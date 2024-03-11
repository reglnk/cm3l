#include <cm3l/Lexer.h>
#include <cm3l/Parser.h>
#include <cm3l/Loadtime/NameResolver.h>
#include <cm3l/Lib/DLList.h>
#include <cm3l/Lib/HashMap.h>

#include <assert.h>
#include <stdio.h> // IWYU pragma: keep
#include <stdlib.h> // IWYU pragma: keep

#define CM3L_PARSER_DEBUG

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
	finalVarDecl,

	finalStatement
}
imFragType;

#ifdef CM3L_PARSER_DEBUG
#define dbgprintf(...) printf("[debug] " __VA_ARGS__)

static inline const char *dbg_fragtypename(imFragType tp)
{
	switch (tp)
	{
		case rawToken: return "rawToken";
		case imtGrouping: return "imtGrouping";
		case imtSequence: return "imtSequence";
		case finalReference: return "finalReference";
		case finalGrouping: return "finalGrouping";
		case finalSequence: return "finalSequence";
		case finalFuncCall: return "finalFuncCall";
		case finalSubscript: return "finalSubscript";
		case finalMemberAccess: return "finalMemberAccess";
		case finalVarDecl: return "finalVarDecl";
		case finalStatement: return "finalStatement";
		default: return "unknown";
	}
}

#else
#define dbgprintf(...)
#endif

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

static inline void *imGetValue(cm3l_ParserContext *ctx, imCodeFragment const *frag);

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

static inline void *imGetValue(cm3l_ParserContext *ctx, imCodeFragment const *frag)
{
	switch (frag->type)
	{
		case rawToken:
			return cm3l_VectorAtM(&ctx->inp->tokens, frag->index, cm3l_Token);
		case imtGrouping:
			return cm3l_VectorAtM(&ctx->imGroups, frag->index, imGroupSeq);
		case imtSequence:
			return cm3l_VectorAtM(&ctx->imSequences, frag->index, imGroupSeq);
		case finalReference:
			return cm3l_VectorAtM(&ctx->outp->references, frag->index, cm3l_StatementReference);
		case finalGrouping:
			return cm3l_VectorAtM(&ctx->outp->groups, frag->index, cm3l_StatementGrouping);
		case finalSequence:
			return cm3l_VectorAtM(&ctx->outp->groups, frag->index, cm3l_StatementSequence);
		case finalVarDecl:
			return cm3l_VectorAtM(&ctx->outp->varDecls, frag->index, cm3l_StatementVarDecl);
		case finalFuncCall:
			return cm3l_VectorAtM(&ctx->outp->groups, frag->index, cm3l_StatementFuncCall);
		case finalSubscript:
			return cm3l_VectorAtM(&ctx->outp->groups, frag->index, cm3l_StatementSubscript);
		case finalMemberAccess:
			return cm3l_VectorAtM(&ctx->outp->groups, frag->index, cm3l_StatementMemberAccess);
		case finalStatement:
			return cm3l_VectorAtM(&ctx->outp->groups, frag->index, cm3l_Statement);
		default: return NULL;
	}
}

static inline void *imGetNodeValue(cm3l_ParserContext *ctx, cm3l_DLNode const *node)
{
	imCodeFragment const *frag = (void const *)node->data;
	return imGetValue(ctx, frag);
}

typedef size_t (*imRangeFn)(
	cm3l_ParserContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last
);

static inline cm3l_Token *imGetRawToken (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *node,
	imCodeFragment *frag
) {
	*frag = *(imCodeFragment *)(void *)node->data;
	if (frag->type == rawToken)
		return imGetValue(ctx, frag);
	return NULL;
}

#ifdef CM3L_PARSER_DEBUG

static inline void dbgprintnode(cm3l_ParserContext *ctx, cm3l_DLNode *node)
{
	if (node == NULL) {
		printf("(null)\n");
		return;
	}
	imCodeFragment frag;
	cm3l_Token *tk = imGetRawToken(ctx, node, &frag);

	if (tk != NULL) {
		printf("token (%zu: %zu: '", tk->line, tk->symbol);
		for (const char *p = tk->beg; p != tk->end; ++p)
			putchar(*p);
		printf("')\n");
		return;
	}
	printf("(type: %s, index: %zu)\n", dbg_fragtypename(frag.type), frag.index);
}
#endif

size_t processRange (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
);

// first stage: process the :: operator
size_t processScopeRes(cm3l_ParserContext *ctx)
{
	for (cm3l_DLNode *node = ctx->fragments.first; node != NULL; )
	{
		// printf("[debug] nodes:\n");
		// for (cm3l_DLNode *nd = ctx->fragments.first; nd != NULL; nd = nd->next)
		// {
		// 	if (nd == node)
		// 		printf("(current) ");
		// 	dbgprintnode(ctx, nd);
		// }
		// printf("=====================\n");
		imCodeFragment frag = *(imCodeFragment *)(void *)node->data;
		if (frag.type == rawToken)
		{
			cm3l_Token *tok = imGetValue(ctx, &frag);
			if (tok->data == Tdt_Oper_ScopeRes)
			{
				if (node->next == NULL) {
					printf("%zu: %zu: unclosed scope resolution operator\n", tok->line, tok->symbol);
					return 1;
				}

				cm3l_DLNode *prev = node->prev, *next = node->next;
				imNamePart name = {.parts = cm3l_VectorCreate(), .isAbsolute = 0};

				if (prev == NULL) name.isAbsolute = 1;
				else
				{
					imCodeFragment fprev = *(imCodeFragment *)(void *)prev->data;
					if (fprev.type != rawToken) {
						printf("%zu: %zu: bad scope resolution\n", tok->line, tok->symbol);
						dbgprintf("hint: prev node is %s\n", dbg_fragtypename(fprev.type));
						return 1;
					}

					cm3l_Token *prevtk = imGetValue(ctx, &fprev);
					if (prevtk->type != Ttp_Var) {
						printf("%zu: %zu: identifier expected\n", prevtk->line, prevtk->symbol);
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
					next = node->next;
					imCodeFragment frag = *(imCodeFragment *)(void *)node->data;
					cm3l_Token *tk = imGetValue(ctx, &frag);
					if (tk->type != Ttp_Var) {
						printf("%zu: %zu: identifier expected\n", tk->line, tk->symbol);
						return 1;
					}
					cm3l_CodeSelectorPtr csel = {.begin = tk->beg, .end = tk->end};
					cm3l_VectorPushBackV(&name.parts, csel);
					cm3l_DLListDeleteNode(&ctx->fragments, node);

					node = next;
					if (node == NULL)
						break;

					frag = *(imCodeFragment *)(void *)node->data;
					if (frag.type != rawToken)
						break;

					tk = imGetValue(ctx, &frag);
					if (tk->data != Tdt_Oper_ScopeRes)
						break;

					next = node->next;
					cm3l_DLListDeleteNode(&ctx->fragments, node);
					node = next;
					if (node == NULL) {
						printf("%zu: %zu: unclosed namespace resolution operator\n", tk->line, tk->symbol);
						return 1;
					}
				}

				// now make the reference out of it
				cm3l_StatementReference refst = {
					.id = CM3L_RESERVED,
					.flags = cm3l_SttRefNone
				};
				if (name.isAbsolute)
					refst.flags |= cm3l_SttRefIsAbsolute;

				if (name.parts.length > 1)
				{
					refst.flags |= cm3l_SttRefIsNested;
					refst.data.nested.length = name.parts.length;
					refst.data.nested.parts = malloc (
						sizeof(cm3l_CodeSelectorPtr) * name.parts.length
					);
					memcpy (refst.data.nested.parts, name.parts.data, (
						sizeof(cm3l_CodeSelectorPtr) * name.parts.length
					));
				}
				else refst.data.regular = *cm3l_VectorAtM(&name.parts, 0, cm3l_CodeSelectorPtr);
				cm3l_VectorDestroy(&name.parts);

				cm3l_Vector *vref = &ctx->outp->references;
				cm3l_VectorPushBackV(vref, refst);

				imCodeFragment *frag = (imCodeFragment *)(void *)prev->data;
				frag->type = finalReference;
				frag->index = vref->length - 1;
			}
			else node = node->next;
		}
		else node = node->next;
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
static size_t makeImtGrouping (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *begin,
	cm3l_DLNode *end
) {
	cm3l_DLNode *sel_begin = begin;
	cm3l_DLNode *access = begin->prev;
	size_t partcount = 0;

	for (cm3l_DLNode *node = begin; node != end; node = node->next)
	{
		imCodeFragment tokf;
		cm3l_Token *tok = imGetRawToken(ctx, node, &tokf);
		if (tok == NULL)
			continue;

		const int isComplete = tok->data == Tdt_Oper_Comma || node->next == end;
		if (!isComplete)
			continue;

		if (tok->data == Tdt_Oper_Comma)
		{
			// remove the node with comma
			cm3l_DLNode *prev = node->prev;
			if (node == begin)
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

// =============== transform routines

// returns the index of new grouping
static size_t finalizeImtGrouping (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *grp
) {
	cm3l_Vector *vgroup = &ctx->outp->groups;
	cm3l_Vector *vimgrp = &ctx->imGroups;
	imCodeFragment *frag = (void *)grp->data;
	assert(frag->type == imtGrouping);

	// pick the vector
	// move it to real grouping statement
	// set the old vector to 0
	// done
	imGroupSeq *container = cm3l_VectorAtM(vimgrp, frag->index, imGroupSeq);

	// wrap the statement into general Statement structure
	cm3l_StatementGrouping finalGroup = {
		.members = container->parts
	};
	container->parts = cm3l_VectorCreate();
	cm3l_VectorPushBackV(vgroup, finalGroup);
	return vgroup->length - 1;
}

typedef struct
{
	cm3l_StatementType type;
	size_t container_offset;
}
imFinalData;

static inline imFinalData convertType(imFragType tp) {
switch (tp)
{
	case finalFuncCall:
		return (imFinalData) {Stt_FuncCall, offsetof(cm3l_ParserData, fnCalls)};
	case finalGrouping:
		return (imFinalData) {Stt_Grouping, offsetof(cm3l_ParserData, groups)};
	case finalReference:
		return (imFinalData) {Stt_Reference, offsetof(cm3l_ParserData, references)};
	case finalMemberAccess:
		return (imFinalData) {Stt_MemberAccess, offsetof(cm3l_ParserData, memberAccOpers)};
	case finalSequence:
		return (imFinalData) {Stt_Sequence, offsetof(cm3l_ParserData, sequences)};
	case finalSubscript:
		return (imFinalData) {Stt_Subscript, offsetof(cm3l_ParserData, subscriptOpers)};
	case finalVarDecl:
		return (imFinalData) {Stt_VarDecl, offsetof(cm3l_ParserData, varDecls)};
	default:
		return (imFinalData) {Stt_Undefined, 0};
}}

// must be one of underlying final forms
static size_t finalizeStatement (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *st
) {
	cm3l_Vector *vstat = &ctx->outp->statements;
	imCodeFragment *frag = (void *)st->data;

	if (frag->type == finalStatement)
		return frag->index;

	imFinalData f = convertType(frag->type);

	// wrap the statement into general Statement structure
	cm3l_Statement finalStat = {
		.index = frag->index,
		.type = f.type,
		.container = ctx->outp + f.container_offset
	};
	cm3l_VectorPushBackV(vstat, finalStat);
	return vstat->length - 1;
}

// ===============

// process the contents of {} braces
// the resulting sequence may contain only final statements
// (braces aren't necessary, for example: `some statement; another = 1`)
//
// some examples of the fragments state before and after call:
//
// before: ['{']<-->[first]<-->[reference]<-->[';']<-->[smth]<-->[';']<-->[last]<-->['}']
// after: ['{']<-->[imtGroupSeq]<-->['}']
//
// before: ['{']<-->[first]<-->['=']<-->[last]<-->['}']
// after: ['{']<-->[imtGroupSeq]<-->['}']
size_t makeImtSequence (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *begin,
	cm3l_DLNode *end
) {
	cm3l_DLList *frags = &ctx->fragments;
	cm3l_DLNode *sel_begin = begin;
	begin = begin->prev;
	end = end->next;
	size_t partcount = 0;

	for (cm3l_DLNode *node = sel_begin; node != end; node = node->next)
	{
		imCodeFragment tokf;
		cm3l_Token *tok = imGetRawToken(ctx, node, &tokf);
		const char isSc = tok != NULL && tok->data == Tdt_Control_Semicolon;

		const int isComplete = isSc || node->next == end;
		if (!isComplete)
			continue;

		if (isSc)
		{
			// remove the node with semicolon
			cm3l_DLNode *prev = node->prev;
			if (prev == begin) {
				printf("empty statement before semicolon\n");
				return 1;
			}
			cm3l_DLListDeleteNode(&ctx->fragments, node);
			node = prev;
		}
		cm3l_DLNode *sel_end = node;

		size_t result = processRange(ctx, sel_begin, sel_end);
		if (result)
			return result;

		++partcount;
		// assert(node->prev != NULL);
		sel_begin = node->next;
	}

	// get access to first node of ones processed with processRange
	cm3l_DLNode *iter = cm3l_DLListNF(frags, begin);
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

	imCodeFragment seqf = {
		.type = imtSequence,
		.index = ctx->imSequences.length
	};
	cm3l_VectorPushBackV(&ctx->imSequences, seq);
	cm3l_DLNode *seqnode = cm3l_DLNodeNewV(seqf);
	cm3l_DLListInsertRc(&ctx->fragments, seqnode, begin);
	return 0;
}

static cm3l_DLNode *findClosingBracket (
	cm3l_ParserContext *ctx,
	cm3l_TokenData openingBrk,
	cm3l_DLNode *node,
	cm3l_DLNode *end
) {
	cm3l_TokenData closingBrk;
	switch (openingBrk)
	{
		case Tdt_Control_ParBeg:
			closingBrk = Tdt_Control_ParEnd;
			break;
		case Tdt_Control_SbrkOpen:
			closingBrk = Tdt_Control_SbrkClose;
			break;
		case Tdt_Control_ScopeBeg:
			closingBrk = Tdt_Control_ScopeEnd;
			break;
		default:
			return NULL;
	}

	cm3l_DLNode *iter = node;
	for (unsigned brkLevel = 1; iter != end; iter = iter->next)
	{
		imCodeFragment chkf;
		cm3l_Token *chk = imGetRawToken(ctx, iter, &chkf);
		if (chk == NULL)
			continue;

		if /**/ (chk->data == closingBrk) ++brkLevel;
		else if (chk->data == openingBrk) --brkLevel;
		if (brkLevel)
			continue;

		return iter;
	}
	return NULL;
}

// find subsequent identfiers and make VarDecl statements from that
size_t processStage1 (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
) {
	cm3l_DLNode *node = first;
	cm3l_DLNode *end = last->next;
	assert(node != end);

	cm3l_DLNode *prev = node;
	node = node->next;
	for (; node != end; (prev = node) && (node = node->next))
	{
		imCodeFragment *pfrag = (void *)prev->data;
		if (pfrag->type == finalReference)
			finalizeStatement(ctx, prev);
		else if (pfrag->type != finalStatement)
			continue;

		imCodeFragment *frag = (void *)node->data;
		if (frag->type != finalReference)
			continue;

		// replace 2 nodes with variable definition
		cm3l_Vector *vdecl = &ctx->outp->varDecls;
		cm3l_StatementVarDecl vdst = {.ref = frag->index, .type = pfrag->index};
		cm3l_VectorPushBackV(vdecl, vdst);

		cm3l_DLListDeleteNode(&ctx->fragments, prev);
		frag->type = finalVarDecl;
		frag->index = vdecl->length - 1;
	}
	return 0;
}

// corresponds to stage 2 at
// https://en.cppreference.com/w/cpp/language/operator_precedence
size_t processStage2 (
	cm3l_ParserContext *ctx,
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
	assert(node != end);

	cm3l_DLNode *next = node->next;
	for (; next != end; node = next)
	{
		next = node->next;

		// printf("[debug] nodes:\n");
		// for (cm3l_DLNode *nd = first; nd != last; nd = nd->next)
		// {
		// 	if (nd == node)
		// 		printf("(current) ");
		// 	dbgprintnode(ctx, nd);
		// }
		// printf("=====================\n");

		imCodeFragment *frag = (void *)node->data;
		switch (frag->type)
		{
			case finalReference:
				break;
			default:
				continue;
		}

		imCodeFragment nextfr;
		cm3l_Token *nexttk = imGetRawToken(ctx, next, &nextfr);
		if (nexttk == NULL)
			continue;

		switch (nexttk->data)
		{
			case Tdt_Control_ParBeg:
			case Tdt_Control_SbrkOpen:
			case Tdt_Oper_Dot:
				break;
			default:
				continue;
		}

		// ['foo'] ['('] ['7'] [')'] ['.'] ['bar'] ['='] ['8']

		// if '('
		// parse to
		// ['fnCall'] ['.'] ['bar'] ['='] ['8']

		if (nexttk->data == Tdt_Control_ParBeg || nexttk->data == Tdt_Control_SbrkOpen)
		{
			cm3l_DLNode *closing = findClosingBracket(ctx, nexttk->data, next->next, end);
			if (closing == NULL) {
				printf("closing bracket not found\n");
				return 1;
			}
			cm3l_DLListDeleteNode(&ctx->fragments, next);
			cm3l_DLListDeleteNode(&ctx->fragments, closing);

			// regardless of whether the brackets are () or [], their contents
			// will be parsed as comma-separated arguments
			// ref(foo, bar, 0); ref[x, y]
			size_t result = makeImtGrouping(ctx, next->next, closing);
			if (result)
				return result;

			size_t stId = finalizeStatement(ctx, node);
			size_t grpId = finalizeImtGrouping(ctx, next->next);

			// @todo ensure the pointer (node) is valid

			if (nexttk->data == Tdt_Control_ParBeg)
			{
				cm3l_StatementFuncCall fncall = {.fn = stId, .args = grpId};
				cm3l_VectorPushBackV(&ctx->outp->fnCalls, fncall);
				frag->type = finalFuncCall;
				frag->index = ctx->outp->fnCalls.length - 1;
			}
			else
			{
				cm3l_StatementSubscript subscr = {.obj = stId, .args = grpId};
				cm3l_VectorPushBackV(&ctx->outp->subscriptOpers, subscr);
				frag->type = finalSubscript;
				frag->index = ctx->outp->fnCalls.length - 1;
			}
		}
		else // if (nexttk->data == Tdt_Oper_Dot)
		{
			// else join that into 'finalMemberAccess'

			cm3l_DLNode *nnext = next->next;
			if (nnext == NULL) {
				printf("error: no field specified\n");
				return 1;
			}
			imCodeFragment nnextfr;
			cm3l_Token *nnexttk = imGetRawToken(ctx, nnext, &nnextfr);
			if (nnexttk->type != Ttp_Var) {
				printf("error: wrong token\n");
				return 1;
			}

			// delete the node with dot operator
			cm3l_DLListDeleteNode(&ctx->fragments, next);

			size_t stId = finalizeStatement(ctx, node);
			cm3l_CodeSelectorPtr csel = {.begin = nnexttk->beg, .end = nnexttk->end};
			cm3l_StatementMemberAccess macc = {.obj = stId, .field = csel, .id = 0};
		}
	}
	return 0;// @todo report error
}

size_t processStage16 (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
) {
	// operators:
	// = += -= *= /= %= ^= |= &= <<= >>=
	// later: ?:

	// token flow:
	// [smth] ['=' or '+=' or ...] [smth] --> [AssignOper]

	cm3l_DLNode *node = first;
	cm3l_DLNode *end = last->next;
	assert(node != end);

	cm3l_DLNode *next = node->next;
	cm3l_DLNode *prev = node->prev;
	for (;
		end != (next = node->next);
		prev = node,
		node = next
	) {
		imCodeFragment frag;
		cm3l_Token *tk = imGetRawToken(ctx, node, &frag);
		if (tk == NULL)
			continue;

		int num = cm3l_getBinaryOperNumA(tk->data);
		if (num == -1)
			continue;

		cm3l_StatementBinaryOper binopst = {
			.type = tk->data,
			.first = finalizeStatement(ctx, prev),
			.second = finalizeStatement(ctx, next)
		};
		cm3l_DLListDeleteNode(&ctx->fragments, prev);
		cm3l_DLNode *rmv = next;
		next = next->next;
		cm3l_DLListDeleteNode(&ctx->fragments, rmv);
		cm3l_VectorPushBackV(&ctx->outp->binOpers, binopst);
	}
	return 0;
}

// remove all odd semicolons and leave final statements only
static void finalizeAll(cm3l_ParserContext *ctx)
{
	for (
		cm3l_DLNode *iter = ctx->fragments.first;
		iter != NULL;
	) {
		imCodeFragment *frag = (void *)iter->data;
		if (frag->type == rawToken)
		{
			cm3l_Token *tk = imGetValue(ctx, frag);
			assert(tk->data == Tdt_Control_Semicolon);
			cm3l_DLNode *prev = iter;
			iter = iter->next;
			cm3l_DLListDeleteNode(&ctx->fragments, prev);
		}
		else {
			// error in converting type
			size_t stId = finalizeStatement(ctx, iter);
			frag->type = finalStatement;
			frag->index = stId;
			iter = iter->next;
		}
	}
}

// input: raw tokens with references
// output: final statements only
size_t processRange (
	cm3l_ParserContext *ctx,
	cm3l_DLNode *first,
	cm3l_DLNode *last // inclusive end marker
) {
	// @todo maybe remake everything to use these non-inclusive markers
	cm3l_DLNode *access_before = first->prev;
	cm3l_DLNode *access_after = last->next;

	size_t errors;
	errors = processStage1(ctx, first, last);
	if (errors)
		return errors;

	// the first and last pointers may have been invalidated. Restore their actual values
	// @todo add method to DLList to shorten this
	first = access_before == NULL ? ctx->fragments.first : access_before->next;
	last = access_after == NULL ? ctx->fragments.last : access_after->prev;
	access_before = first->prev;
	access_after = last->next;
	errors = processStage2(ctx, first, last);

	first = access_before == NULL ? ctx->fragments.first : access_before->next;
	last = access_after == NULL ? ctx->fragments.last : access_after->prev;
	// access_before = first->prev;
	// access_after = last->next;
	errors = processStage16(ctx, first, last);
	return errors;
}

unsigned cm3l_ParserProcess(cm3l_LexerData const *inp, cm3l_ParserData *outp)
{
	assert(!inp->syntaxErrors);

	cm3l_ParserContext ctx = {
		.inp = inp,
		.outp = outp,
		.errcount = 0,
		.nesting = 0,
		.vnameRes = cm3l_VectorCreate(),
		.imGroups = cm3l_VectorCreate(),
		.imSequences = cm3l_VectorCreate(),
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

	errors = processRange(&ctx, ctx.fragments.first, ctx.fragments.last);
	if (errors)
		return errors;

	finalizeAll(&ctx);

	errors = makeImtSequence(&ctx, ctx.fragments.first, ctx.fragments.last);
	if (errors)
		return errors;

	assert(ctx.fragments.length == 1);
	imCodeFragment *frag = (void *)ctx.fragments.first->data;
	assert(frag->type == imtSequence);

	// size_t *parts = seq->parts.data;
	// for (size_t i = 0; i != seq->parts.length; ++i)
	// 	cm3l_VectorPushBackV(&outp->toplevel, parts[i]);
	// cm3l_VectorDestroy(&seq->parts);

	// optimization:
	cm3l_VectorDestroy(&outp->toplevel);
	imGroupSeq *seq = cm3l_VectorAtM(&ctx.imSequences, frag->index, imGroupSeq);
	outp->toplevel = seq->parts;

	// error: the outp->toplevel is misreading
	// either its not pointing to real 'final statements', or the intermediate types are not converted, or smth

	cm3l_HashMapEv *m = ctx.vnameRes.data;
	for (size_t i = 0; i != ctx.vnameRes.length; ++i)
		cm3l_HashMapEvDestroy(m + i);

	cm3l_VectorDestroy(&ctx.vnameRes);
	cm3l_VectorDestroy(&ctx.imGroups);
	cm3l_VectorDestroy(&ctx.imSequences);
	cm3l_DLListDestroy(&ctx.fragments);
	return 0;
}
