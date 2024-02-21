#include <cm3l/Lexer.h>
#include <cm3l/Composer.h>
#include <cm3l/Loadtime/NameResolver.h>
#include <cm3l/Lib/DLList.h>
#include <cm3l/Lib/HashMap.h>

#include <assert.h>
#include <stdio.h> // IWYU pragma: keep
#include <stdlib.h> // IWYU pragma: keep

typedef enum
{
	rawToken,

	// All fragments of intermediate types (called imt...) are converted to ones of
	// final types (incase there're no errors)

	// The difference from finalGrouping option is that
	// it marks the sequence of tokens as something that may be separated with commas
	// or not (therefore without impact on the inner statement in 2nd case),
	// while the final form behaves as a data structure containing multiple elements
	// (even if there's a single one).
	// so, it's crucial for some scenarios, including:
	// 1. C-style casts, which accept a single statement,
	//    not an array: (type)value;
	// 2. calculations: (foo + bar) * ff;
	imtGrouping,

	// the following is the same as imtGrouping but applicable to finalSequence
	// and different separators with braces
	imtSequence,

	// If a fragment has final type, that means it's stored in output structure
	// and is syntactically complete, but final statements might be joined into
	// another high-level statements, while not deleting them (unlike intermediate statements).
	finalReference,
	finalGrouping, // @deprecated ??
	finalSequence, // @deprecated ??

	finalFuncCall,
	finalSubscript,
	finalMemberAccess,

	finalStatement
}
imFragType;

typedef struct
{
	// an offset indices pointing to actual cm3l_Token's
	cm3l_Vector /* cm3l_CodeSelectorPtr */ parts;
	int isAbsolute;
}
imNamePart;

typedef struct
{
	// members of the vector are pointing to finalStatement's
	cm3l_Vector /* size_t */ parts;
}
imGroupSeq;

typedef struct
{
	imFragType type;
	size_t index;
}
imCodeFragment;

static inline void *imGetValue(cm3l_ComposerContext *ctx, imCodeFragment const *frag);

static const char *builtins[] = {
	"char",
	"byte",
	"int",
	"uint",
	"long",
	"ulong",
	"float",
	"double",
	"type"
};

static size_t builtins_size[] = {
	5, 5, 4, 5, 5, 6, 6, 7, 5
};

static inline void *imGetValue(cm3l_ComposerContext *ctx, imCodeFragment const *frag)
{
	switch (frag->type)
	{
		case rawToken:
			return cm3l_VectorAtM(&ctx->inp->tokens, frag->index, cm3l_Token);
		case finalReference:
			return cm3l_VectorAtM(&ctx->outp->references, frag->index, cm3l_StatementReference);
		default: return NULL;
	}
}

static inline void *imGetNodeValue(cm3l_ComposerContext *ctx, cm3l_DLNode const *node)
{
	imCodeFragment const *frag = (void const *)node->data;
	return imGetValue(ctx, frag);
}

typedef size_t (*imRangeFn)(
	cm3l_ComposerContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last
);

static inline cm3l_Token *imGetRawToken (
	cm3l_ComposerContext *ctx,
	cm3l_DLNode *node,
	imCodeFragment *frag
) {
	*frag = *(imCodeFragment *)(void *)node->data;
	if (frag->type == rawToken)
		return imGetValue(ctx, frag);
	return NULL;
}

size_t processRange (
	cm3l_ComposerContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
);

// first stage: process the :: operator
size_t processScopeRes(cm3l_ComposerContext *ctx)
{
	cm3l_DLNode *node = ctx->fragments.first;
	while (node != NULL)
	{
		imCodeFragment frag = *(imCodeFragment *)(void *)node->data;
		if (frag.type == rawToken)
		{
			cm3l_Token *tok = imGetValue(ctx, &frag);
			if (tok->data == Tdt_Oper_ScopeRes)
			{
				if (node->next == NULL) {
					printf("bad scope resolution\n");
					return 1;
				}

				cm3l_DLNode *prev = node->prev, *next = node->next;
				imNamePart name = {.parts = cm3l_VectorCreate(), .isAbsolute = 0};

				if (prev == NULL) name.isAbsolute = 1;
				else
				{
					imCodeFragment fprev = *(imCodeFragment *)(void *)prev->data;

					if (fprev.type != rawToken) {
						printf("bad scope resolution\n");
						return 1;
					}
					cm3l_Token *prevtk = imGetValue(ctx, &fprev);
					if (prevtk->type != Ttp_Var) {
						printf("bad scope resolution\n");
						return 1;
					}

					cm3l_CodeSelectorPtr csel = {.begin = prevtk->beg, .end = prevtk->end};
					cm3l_VectorPushBackV(&name.parts, csel);

					cm3l_DLListDeleteNode(&ctx->fragments, prev);
				}

				// the order of checks looks like
				// ? :: foo :: bar :: spam
				//    1
				// 2
				//       3
				//           4
				//              5
				//                  6
				//                      7

				// the (prev) node will store the parsed reference
				prev = node; node = next;
				for (;;)
				{
					imCodeFragment frag = *(imCodeFragment *)(void *)node->data;
					cm3l_Token *tk = imGetValue(ctx, &frag);
					if (tk->type != Ttp_Var) {
						printf("bad scope resolution\n");
						return 1;
					}
					cm3l_CodeSelectorPtr csel = {.begin = tk->beg, .end = tk->end};
					cm3l_VectorPushBackV(&name.parts, csel);

					node = node->next;
					if (node == NULL)
						break;

					frag = *(imCodeFragment *)(void *)node->data;
					if (frag.type != rawToken)
						break;

					tk = imGetValue(ctx, &frag);
					if (tk->data != Tdt_Oper_ScopeRes)
						break;

					if (node->next == NULL) {
						printf("bad scope resolution\n");
						return 1;
					}
					node = node->next;
				}

				// now make the reference out of it
				cm3l_StatementReference refst = {.flags = cm3l_SttRefNone};
				if (name.isAbsolute)
					refst.flags |= cm3l_SttRefIsAbsolute;

				if (name.parts.length > 1)
				{
					refst.flags |= cm3l_SttRefIsNested;
					refst.data.complex.length = name.parts.length;
					refst.data.complex.parts = malloc (
						sizeof(cm3l_CodeSelectorPtr) * name.parts.length
					);
					memcpy (refst.data.complex.parts, name.parts.data, (
						sizeof(cm3l_CodeSelectorPtr) * name.parts.length
					));
				}
				else refst.data.regular = *cm3l_VectorAtM(&name.parts, 0, cm3l_CodeSelectorPtr);
				cm3l_VectorDestroy(&name.parts);

				cm3l_Vector *vref = &ctx->outp->references;
				cm3l_VectorPushBackV(vref, refst);

				imCodeFragment *frag = (imCodeFragment *)(void *)node->data;
				frag->type = finalReference;
				frag->index = vref->length - 1;
			}
		}
	}
	return 0;
}

// not a some definite stage: process the contents of rounded brackets
// (brackets aren't necessary, for example: `a, b = b, a`)
//
// some examples of the fragments state before and after call:
//
// before: ['(']<-->[first]<-->[reference]<-->[',']<-->[smth]<-->[',']<-->[last]<-->[')']
// after: ['(']<-->[finalGrouping]<-->[')']
//
// before: ['(']<-->[first]<-->['+']<-->[last]<-->[')']
// after: ['(']<-->[imtGrouping]<-->[')']
//
// before: ['(']<-->[first == last]<-->[')']
// after: ['(']<-->[imtGrouping]<-->[')']

// @todo move commas parsing to processRange() because:
// 1. it should be able to detect such things and parse
//    (especially the semicolons as statement separators)
// 2. the comma is an operator so it anyway gets parsed
//    (so, gets parsed into a form that depends on the decision in [ctx. 1])
// 3. it's not known whether brackets contain grouping or sequence
// or not move because:
// 1. this function outputs just imtGrouping but processRange does the whole finalStatement
//    (or not?? @todo decide what processRange actually does [ctx. 1])
// 2. detecting is more complicated than just
//    parsing some code while keeping in mind this code is a content of rounded brackets

// [ctx.1] parsing to less high-level structures than finalStatement
// pros:
// 1. perf obviously
// 2. once added to array of statements, the one cannot be deleted back and restructured
// cons:
// 1. more branches

// @deprecated ??
size_t makeImtGrouping (
	cm3l_ComposerContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
) {
	cm3l_DLNode *sel_begin = first;
	cm3l_DLNode *access = first->prev;
	size_t partcount = 0;

	for (cm3l_DLNode *node = first; node != last->next; node = node->next)
	{
		imCodeFragment tokf;
		cm3l_Token *tok = imGetRawToken(ctx, node, &tokf);
		if (tok == NULL)
			continue;

		const int isComplete = tok->data == Tdt_Oper_Comma || node == last;
		if (!isComplete)
			continue;

		if (tok->data == Tdt_Oper_Comma)
		{
			// remove the node with comma
			cm3l_DLNode *prev = node->prev;
			if (node == first)
			{
				printf("empty statement before comma\n");
				return 1;
			}
			else { assert(prev != NULL); }

			cm3l_DLListDeleteNode(&ctx->fragments, node);
			node = prev;
		}
		cm3l_DLNode *sel_end = node;

		size_t result = processRange(ctx, sel_begin, sel_end);
		if (result)
			return result;

		++partcount;
		assert(node->prev != NULL);
		sel_begin = node->next;
	}

	// get access to first node of ones processed with processRange
	cm3l_DLNode *iter = access != NULL ? access->next : ctx->fragments.first;
	assert(iter != NULL);

	// create a new imGroupSeq statement
	imGroupSeq group = {.parts = cm3l_VectorCreate()};
	while (partcount--)
	{
		imCodeFragment *frag = (void *)iter->data;
		assert(frag->type == finalStatement);
		cm3l_VectorPushBackV(&group.parts, frag->index);

		cm3l_DLNode *prev = iter; iter = iter->next;
		cm3l_DLListDeleteNode(&ctx->fragments, prev);
	}

	cm3l_DLNode *grpnode = cm3l_DLNodeNewV(group);
	cm3l_DLListInsertRc(&ctx->fragments, grpnode, access);
	return 0;
}

// @deprecated ??
// process the contents of {} braces
// (braces aren't necessary, for example: `some statement; another = 1`)
//
// some examples of the fragments state before and after call:
//
// before: ['{']<-->[first]<-->[reference]<-->[';']<-->[smth]<-->[';']<-->[last]<-->['}']
// after: ['{']<-->[finalSequence]<-->['}']
//
// before: ['{']<-->[first]<-->['=']<-->[last]<-->['}']
// after: ['{']<-->[finalStatement]<-->['}']
size_t makeImtSequence (
	cm3l_ComposerContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
) {
	cm3l_DLNode *sel_begin = first;
	cm3l_DLNode *access = first->prev;
	size_t partcount = 0;

	for (cm3l_DLNode *node = first; node != last->next; node = node->next)
	{
		imCodeFragment tokf;
		cm3l_Token *tok = imGetRawToken(ctx, node, &tokf);
		if (tok == NULL)
			continue;

		const int isComplete = tok->data == Tdt_Control_Semicolon || node == last;
		if (!isComplete)
			continue;

		if (tok->data == Tdt_Control_Semicolon)
		{
			// remove the node with semicolon
			cm3l_DLNode *prev = node->prev;
			if (node == first)
			{
				printf("empty statement before semicolon\n");
				return 1;
			}
			else { assert(prev != NULL); }

			cm3l_DLListDeleteNode(&ctx->fragments, node);
			node = prev;
		}
		cm3l_DLNode *sel_end = node;

		size_t result = processRange(ctx, sel_begin, sel_end);
		if (result)
			return result;

		++partcount;
		assert(node->prev != NULL);
		sel_begin = node->next;
	}

	// get access to first node of ones processed with processRange
	cm3l_DLNode *iter = access != NULL ? access->next : ctx->fragments.first;
	assert(iter != NULL);

	// create a new imGroupSeq statement
	imGroupSeq seq = {.parts = cm3l_VectorCreate()};
	while (partcount--)
	{
		imCodeFragment *frag = (void *)iter->data;
		assert(frag->type == finalStatement);
		cm3l_VectorPushBackV(&seq.parts, frag->index);

		cm3l_DLNode *prev = iter; iter = iter->next;
		cm3l_DLListDeleteNode(&ctx->fragments, prev);
	}

	cm3l_DLNode *seqnode = cm3l_DLNodeNewV(seq);
	cm3l_DLListInsertRc(&ctx->fragments, seqnode, access);
	return 0;
}

// @deprecated ??
// (not ??) second stage: process all brackets () {}, later []
size_t processAllBrackets (
	cm3l_ComposerContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
) {
	cm3l_DLNode *node;
	for (node = first;; )
	{
		imCodeFragment tokf;
		cm3l_Token *tok = imGetRawToken(ctx, node, &tokf);
		if (tok == NULL)
			continue;

		cm3l_TokenData openingBrk, closingBrk;
		imRangeFn procRangeFn;
		switch (tok->data)
		{
			case Tdt_Control_ParBeg:
				openingBrk = Tdt_Control_ParBeg;
				closingBrk = Tdt_Control_ParEnd;
				procRangeFn = makeImtGrouping;
				break;
			case Tdt_Control_ScopeBeg:
				openingBrk = Tdt_Control_ScopeBeg;
				closingBrk = Tdt_Control_ScopeEnd;
				procRangeFn = makeImtSequence;
				break;
			default:
				continue;
		}

		cm3l_DLNode *iter = node;
		for (unsigned brkLevel = 1;; )
		{
			iter = iter->next;
			if (iter == last)
				break;

			imCodeFragment chkf;
			cm3l_Token *chk = imGetRawToken(ctx, iter, &chkf);
			if (chk == NULL)
				continue;

			if /**/ (chk->data == closingBrk) ++brkLevel;
			else if (chk->data == openingBrk) --brkLevel;
			if (brkLevel)
				continue;

			// the whole sequence from '(' to ')' will be replaced with complete statement
			size_t res = procRangeFn(ctx, node->next, iter->prev);
			if (res) return res;

			// remove brackets
			cm3l_DLListDeleteNode(&ctx->fragments, node); node = iter;
			cm3l_DLListDeleteNode(&ctx->fragments, iter);
			break;
		}
		node = node->next;
		if (node == last)
			break;
	}
	return 0;
}

// https://en.cppreference.com/w/cpp/language/operator_precedence
size_t processStage2 (
	cm3l_ComposerContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
) {
	// postfix inc/dec (later)
	// function call
	// subscript
	// member access

	// token flow:
	// [smth] ['('] ... [')'] --> [funcCall]
	// [smth] ['['] ... [']'] --> [subscript]
	// [smth] ['.'] ... --> [membAccess]

	cm3l_DLNode *node = first;
	cm3l_DLNode *end = last->next;
	for (; node != end; node = node->next)
	{
		imCodeFragment *frag = (void *)node->data;
		switch (frag->type)
		{
			case finalReference:
				break;
			default:
				continue;
		}

		cm3l_DLNode *next = node->next;
		if (next == NULL)
			break;

		imCodeFragment nextfr;
		cm3l_Token *nexttk = imGetRawToken(ctx, next, &nextfr);
		switch (nexttk->data)
		{
			case Tdt_Control_ParBeg:
				break;
			case Tdt_Control_SbrkOpen:
				break;
			default:
				continue;
		}

		// ['foo'] ['('] ['7'] [')'] ['.'] ['bar'] ['='] ['8']

		// if '('
		// parse to
		// ['fnCall'] ['.'] ['bar'] ['='] ['8']
	}
	return 0;
}

size_t processRange (
	cm3l_ComposerContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
) {
	size_t errors = processAllBrackets(ctx, first, last);
	return errors;
}

unsigned cm3l_Compose(cm3l_LexerData const *inp, cm3l_ComposerData *outp)
{
	assert(!inp->syntaxErrors);

	cm3l_ComposerContext ctx = {
		.inp = inp,
		.outp = outp,
		.errcount = 0,
		.nesting = 0,
		.vnameRes = cm3l_VectorCreate(),
		.fragments = cm3l_DLListCreate()
	};

	// create the global scope and assign some builtin values
	{
		cm3l_NameResolver nres = {
			.scope = cm3l_HashMapCreate(cm3l_HashFuncS),
			.nextId = 0
		};
		for (size_t i = 0; i != sizeof(builtins) / sizeof(builtins[0]); ++i) {
			size_t value = CM3L_RESERVED;
			cm3l_HashMapEvInsertKV(&nres.scope, builtins[i], builtins_size[i], &value, sizeof(size_t));
		}
		cm3l_VectorPushBackV(&ctx.vnameRes, nres);
	}

	// fill the list with all tokens converted to their intermediate representations
	for (size_t i = 0; i != inp->tokens.length; ++i) {
		imCodeFragment frag = {.type = rawToken, .index = i};
		cm3l_DLListPushBackV(&ctx.fragments, frag);
	}

	size_t errors = processScopeRes(&ctx);
	if (errors)
		return errors;

	errors = makeImtSequence(&ctx, ctx.fragments.first, ctx.fragments.last);
	if (errors)
		return errors;

	assert(ctx.fragments.length == 1);
	imGroupSeq *seq = (void *)ctx.fragments.first->data;

	// size_t *parts = seq->parts.data;
	// for (size_t i = 0; i != seq->parts.length; ++i)
	// 	cm3l_VectorPushBackV(&outp->toplevel, parts[i]);
	// cm3l_VectorDestroy(&seq->parts);

	// optimization:
	cm3l_VectorDestroy(&outp->toplevel);
	outp->toplevel = seq->parts;

	cm3l_DLListDeleteNode(&ctx.fragments, ctx.fragments.first);
	return 0;
}
