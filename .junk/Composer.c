#include <cm3l/Lexer.h>
#include <cm3l/Composer.h>
#include <cm3l/Loadtime/NameResolver.h>
#include <cm3l/Lib/DLList.h>
#include <cm3l/Lib/HashMap.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// The composer converts token sequence into executable AST.
//
// todo: include some optimizer of AST
// for example, the following sequence:
//|  a = f + q + k + w + o * 2
// (semantically the same as following)
//|  a = (((f + q) + k) + w) + (o * 2)
// without optimizations is converted into such AST:
/*
 * |               oper =
 * |              /     \
 * |           ref a    oper +
 * |                   /     \
 * |              oper +      oper *
 * |             /     \     /     \
 * |         oper +   ref w|ref o   lit 2
 * |        /     \
 * |     oper +   ref k
 * |    /     \
 * |  ref f   ref q
 */
// so it will involve much recursion while executing.
// The solution is to make an array of addition components:
/*
 * |          oper =
 * |         /     \
 * |      ref a    opergroup +
 * |      ____________/    \________
 * |     /    /      /      \       \
 * |  ref f  ref q  ref k   ref w   oper *
 * |                               /     \
 * |                             ref o   lit 2
 */
// todo: output errors when returning NULL (if interpreter is launched without some flag)

static void printToken(cm3l_Token const *tk)
{
	for (const char *s = tk->beg; s != tk->end; ++s)
		putchar(*s);
}

int cm3l_IsAssignOper(cm3l_TokenData data)
{
	switch (data)
	{
		case Tdt_Oper_Assign:
			return 1;
		default:
			return 0;
	}
}

// @todo update
int cm3l_IsValueOper(cm3l_TokenData data)
{
	switch (data)
	{
		case Tdt_Oper_IsEqual:
		case Tdt_Oper_IsNEqual:
		case Tdt_Oper_IsGreater:
		case Tdt_Oper_IsGreaterEqual:
		case Tdt_Oper_IsLess:
		case Tdt_Oper_IsLessEqual:
		case Tdt_Oper_Add:
		case Tdt_Oper_Sub:
		case Tdt_Oper_Mul:
		case Tdt_Oper_Div:
		case Tdt_Oper_Dot:
		case Tdt_Oper_Colon:
			return 1;
		default:
			return 0;
	}
}

int cm3l_IsValueStatement(cm3l_StatementType tp)
{
	switch (tp)
	{
		case Stt_Reference:

		case Stt_BasicOper:
		case Stt_AssignOper:
		case Stt_ValueOper:

		case Stt_VarDecl:

		case Stt_InlineFunction:
		case Stt_FuncCall:

		case Stt_Grouping:
		case Stt_Sequence:
			return 1;

		default:
			return 0;
	}
}

static size_t s_findAdjBracket(cm3l_LexerData const *inp, size_t beg)
{
	cm3l_Token const *data = inp->tokens.data;

	cm3l_TokenData op = data[beg].data;
	cm3l_TokenData cl =
	op == Tdt_Control_ParBeg ? Tdt_Control_ParEnd
	: Tdt_Control_ScopeEnd;

	int lvl = 1;
	for (size_t i = beg + 1; i < inp->tokens.length; ++i)
	{
		if (data[i].data == cl)
			--lvl;
		else if (data[i].data == op)
			++lvl;

		if (!lvl)
			return i;
	}
	return CM3L_NONE;
}

static inline void *selectVector(cm3l_ComposerData *outp, cm3l_StatementType type)
{
	switch (type)
	{
		case Stt_Reference: return &outp->references;
		case Stt_BasicOper: return &outp->basicOpers;
		case Stt_UnaryOper: return &outp->unaryOpers;
		case Stt_AssignOper: return &outp->binOpers;
		case Stt_ValueOper: return &outp->binOpers;

		case Stt_ForLoop: return &outp->forLoops;
		case Stt_Branch: return &outp->branches;

		case Stt_VarDecl: return &outp->varDecls;

		case Stt_Function: return &outp->functions;
		case Stt_InlineFunction: return &outp->inlineFns;
		case Stt_FuncCall: return &outp->fnCalls;

		case Stt_Grouping: return &outp->groups;
		case Stt_Sequence: return &outp->sequences;

		default:
			return NULL;
	};
}

static size_t processStatement (
	cm3l_ComposerContext *ctx,
	size_t beg,
	size_t len,
	size_t *cont
);

static inline void newScope(cm3l_ComposerContext *ctx)
{
	cm3l_NameResolver nres = {
		.scope = cm3l_HashMapCreate(cm3l_HashFuncS),
		.nextId = 0
	};
	cm3l_VectorPushBackM(&ctx->vnameRes, &nres, cm3l_NameResolver);
}

static inline void endScope(cm3l_ComposerContext *ctx)
{
	cm3l_NameResolver *nres = cm3l_VectorBackM(&ctx->vnameRes, cm3l_NameResolver);
	cm3l_HashMapDestroy(&nres->scope);
	cm3l_VectorPopBackM(&ctx->vnameRes, cm3l_NameResolver);
}

static size_t s_stWrapSingle (
	cm3l_ComposerData *outp,
	cm3l_StatementType type,
	size_t index
) {
	cm3l_Vector *statv = &outp->statements;

	cm3l_VectorResizeM(statv, statv->length + 1, cm3l_Statement);
	cm3l_Statement *newst = cm3l_VectorBackM(statv, cm3l_Statement);

	newst->type = type;
	newst->index = index;
	newst->container = selectVector(outp, type);

	return statv->length - 1;
}

// a reference to variable from one token or several tokens, connected with ::
static size_t s_stReference (
	cm3l_ComposerContext *ctx,
	size_t beg,
	size_t end, // CM3L_NONE as default
	size_t *cont // NULL as default
) {
	cm3l_Token const *data = (cm3l_Token const *)ctx->inp->tokens.data;
	size_t tkend = end != CM3L_NONE ? end : ctx->inp->tokens.length;
	cm3l_Vector *statvref = &ctx->outp->references;

	assert(tkend != beg);

	cm3l_StatementReference statref = { .flags = cm3l_SttRefNone, .id = 0u };
	cm3l_Vector nameparts = cm3l_VectorCreate();

	if (data[beg].data == Tdt_Oper_ScopeRes)
	{
		++beg;
		statref.flags |= cm3l_SttRefIsAbsolute;
	}

	size_t i;
	for (i = beg; i < tkend; i += 2)
	{
		if (data[i].type == Ttp_Var)
		{
			cm3l_CodeSelectorPtr csel = { .begin = data[i].beg, .end = data[i].end };
			cm3l_VectorPushBackM(&nameparts, &csel, cm3l_CodeSelectorPtr);
		}
		else if (end != CM3L_NONE) return CM3L_NONE;
		else break;

		if (data[i + 1].data != Tdt_Oper_ScopeRes)
		{
			--i;
			break;
		}
	}
	if (cont) *cont = i;

	if (!nameparts.length)
		return CM3L_NONE;

	if (nameparts.length > 1)
	{
		statref.data.complex.parts = nameparts.data;
		statref.data.complex.length = nameparts.length;
		statref.flags |= cm3l_SttRefIsNested;
	}
	else
	{
		cm3l_CodeSelectorPtr *csel = cm3l_VectorAtM(&nameparts, 0, cm3l_CodeSelectorPtr);
		statref.data.regular.begin = csel->begin;
		statref.data.regular.end = csel->end;
		cm3l_VectorDestroy(&nameparts);
	}
	cm3l_VectorPushBackM(statvref, &statref, cm3l_StatementReference);
	return statvref->length - 1;
}

// a reference to variable from one token or several tokens, connected with ::
// doesn't actually assign ID to a variable
static size_t s_stReferenceFromEnd (
	cm3l_ComposerContext *ctx,
	size_t begr, // reverse beginning (should point to Ttp_Var)
	size_t endr, // CM3L_NONE as default
	size_t *cont // NULL as default
) {
	cm3l_Token const *data = (cm3l_Token const *)ctx->inp->tokens.data;
	cm3l_Vector *statvref = &ctx->outp->references;

	assert(endr != begr);

	cm3l_StatementReference statref = {.flags = cm3l_SttRefNone};
	cm3l_Vector nameparts = cm3l_VectorCreate();

	size_t i;
	int next_symbol = 0;
	int partcount = 0;
	for (i = begr; i != endr; --i)
	{
		if ((!next_symbol && data[i].type == Ttp_Var) ||
			(next_symbol && data[i].data == Tdt_Oper_ScopeRes)
		) {
			next_symbol = !next_symbol;
			++partcount;
		}
		else break;
	}
	if (cont) *cont = i;
	if (!partcount)
		return CM3L_NONE;

	++i;

	if (data[i].data == Tdt_Oper_ScopeRes)
	{
		++i;
		statref.flags |= cm3l_SttRefIsAbsolute;
	}

	for (; i <= begr; i += 2)
	{
		cm3l_CodeSelectorPtr csel = { .begin = data[i].beg, .end = data[i].end };
		cm3l_VectorPushBackM(&nameparts, &csel, cm3l_CodeSelectorPtr);
	}
	--i;

	if (!nameparts.length)
		return CM3L_NONE;

	if (nameparts.length > 1)
	{
		statref.data.complex.parts = nameparts.data;
		statref.data.complex.length = nameparts.length;
		statref.flags |= cm3l_SttRefIsNested;
	}
	else
	{
		cm3l_CodeSelectorPtr *csel = cm3l_VectorAtM(&nameparts, 0, cm3l_CodeSelectorPtr);
		statref.data.regular.begin = csel->begin;
		statref.data.regular.end = csel->end;
		cm3l_VectorDestroy(&nameparts);
	}
	cm3l_VectorPushBackM(statvref, &statref, cm3l_StatementReference);
	return statvref->length - 1;
}

// a reference to literal from one token
static size_t s_stLiteral (
	cm3l_ComposerContext *ctx,
	size_t beg,
	size_t end, // CM3L_NONE as default
	size_t *cont // NULL as default
) {
	cm3l_Token const *data = (cm3l_Token const *)ctx->inp->tokens.data;
	size_t tkend = end != CM3L_NONE ? end : beg + 1;
	cm3l_Vector *statvref = &ctx->outp->references;

	assert(tkend != beg);
	if (cont) *cont = tkend;

	cm3l_StatementReference statref = { .flags = cm3l_SttRefIsLiteral, .id = 0u };
	statref.data.regular.begin = data[beg].beg;
	statref.data.regular.end = data[beg].end;

	cm3l_VectorPushBackM(statvref, &statref, cm3l_StatementReference);
	return statvref->length - 1;
}

// Curly brackets aren't required for something to be a sequence
// (for example, the entire contents of valid cm3l source file is).
// The parsing ends at right curly bracket, specified length or EOF.
static size_t s_stSequence (
	cm3l_ComposerContext *ctx,
	size_t beg,
	size_t end, // CM3L_NONE as default
	size_t *cont // NULL as default
) {
	cm3l_Token const *data = (cm3l_Token const *)ctx->inp->tokens.data;
	size_t tkend = end != CM3L_NONE ? end : ctx->inp->tokens.length;
	cm3l_Vector *statseqv = &ctx->outp->sequences;

	cm3l_StatementSequence seq;
	seq.statements = cm3l_VectorCreate();

	size_t iter = beg;
	newScope(ctx);
	while (iter != tkend)
	{
		if (data[iter].data == Tdt_Control_ScopeEnd)
		{
			++iter;
			break;
		}

		size_t st = processStatement(ctx, iter, tkend, &iter);
		if (st != CM3L_NONE)
			cm3l_VectorPushBackM(&seq.statements, &st, cm3l_Statement *);
	}
	endScope(ctx);

	if (cont) *cont = iter;
	cm3l_VectorPushBackM(statseqv, &seq, cm3l_StatementSequence);
	return statseqv->length - 1;
}

// parse sequence or single statement, depending on
// whether it's in curly brackets or ended with semicolon.
// suits for building if - else constructions, loops, functions etc.
static size_t s_stSequenceOrSingle (
	cm3l_ComposerContext *ctx,
	size_t beg,
	size_t end, // CM3L_NONE as default
	size_t *cont // NULL as default
) {
	cm3l_Token const *data = (cm3l_Token const *)ctx->inp->tokens.data;
	size_t tkend = end != CM3L_NONE ? end : ctx->inp->tokens.length;
	cm3l_Vector *statv = &ctx->outp->statements;
	assert(tkend != beg);

	if (data[beg].data == Tdt_Control_ScopeBeg)
	{
		if (tkend - beg < 2)
			return CM3L_NONE;

		size_t statseq = s_stSequence(ctx, beg + 1, tkend - 1, NULL);
		if (cont) *cont = tkend;
		return s_stWrapSingle(ctx->outp, Stt_Sequence, statseq);
	}
	return processStatement(ctx, beg, tkend, cont);
}

static size_t s_stGrouping (
	cm3l_ComposerContext *ctx,
	size_t beg,
	size_t end, // CM3L_NONE as default
	size_t *cont // NULL as default
) {
	cm3l_Token const *data = (cm3l_Token const *)ctx->inp->tokens.data;
	size_t tkend = end != CM3L_NONE ? end : ctx->inp->tokens.length;
	cm3l_Vector *statgrpv = &ctx->outp->groups;

	if (beg == end)
		return CM3L_NONE;

	size_t stend = tkend;
	if (data[beg].data == Tdt_Control_ParBeg)
	{
		size_t i;
		for (i = ++beg; i != tkend; ++i)
		{
			if (data[i].data == Tdt_Control_ParEnd)
			{
				stend = i;
				break;
			}
		}
		if (i == tkend)
		{
			// @todo report error
			return CM3L_NONE;
		}
		if (cont) *cont = stend + 1;
	}
	else  {
		if (cont) *cont = beg + 1;

		// @todo report error
		// temporary solution. @todo implement s_stGroupingOrSingle
		return CM3L_NONE;
	}

	cm3l_StatementGrouping grp = { .members = cm3l_VectorCreate() };

	for (size_t iter = beg; iter < stend; )
	{
		size_t cur_end;
		for (cur_end = iter; cur_end != stend; ++cur_end)
			if (data[cur_end].data == Tdt_Oper_Comma)
				break;

		size_t st = processStatement(ctx, iter, cur_end, &iter);
		if (st == CM3L_NONE)
		{
			cm3l_VectorDestroy(&grp.members);
			return CM3L_NONE;
		}
		cm3l_VectorPushBackA(&grp.members, &st);

		// jump after comma
		// incase of that ')' is at data[cur_end], the loop will exit
		iter = cur_end + 1;
	}

	cm3l_VectorPushBackM(statgrpv, &grp, cm3l_StatementGrouping);
	return statgrpv->length - 1;
}

/*
 * static cm3l_StatementVarDecl *s_stVariableDeclaration (
 *	ComposerContext *ctx,
 *	size_t beg,
 *	cm3l_Statement *type_st
 * ) {
 *	cm3l_Vector *statv = &outp->statements;
 *	cm3l_Vector *statvvd = &outp->varDecls;
 *
 *	cm3l_StatementVarDecl vd;
 *	vd.type = type_st,
 *	vd.ref = s_stReference(data, outp, beg);
 *
 *	cm3l_VectorPushBackM(statvvd, &vd, cm3l_StatementVarDecl);
 *	return cm3l_VectorBackM(statvvd, cm3l_StatementVarDecl);
 * } */

// parse statement from (beg) to (end).
// @param beg - the beginning of statement
// @param end - the farmost position of statement end (actually the statement may be shorter)
// @param cont - where to place actual end position
// return value - the index of parsed statement in vector of statements inside (ctx).
static size_t processStatement (
	cm3l_ComposerContext *ctx,
	size_t beg,
	size_t end,
	size_t *cont // NULL as default
) {
	cm3l_Token const *data = (cm3l_Token const *)ctx->inp->tokens.data;
	size_t const tkend = end != CM3L_NONE ? end : ctx->inp->tokens.length;
	cm3l_Vector *statv = &ctx->outp->statements;

	if (beg == tkend)
		return CM3L_NONE;

	cm3l_TokenData tdt = data[beg].data;

	// The current statement ends at:
	// 1. either at semicolon, or
	// 2. at the end of beginning statement, if it begins with Ttp_Control

	if (data[beg].type == Ttp_Control)
	{
		if (tdt == Tdt_Control_Semicolon)
		{
			if (cont) *cont = beg + 1;
			return CM3L_NONE;
		}
		if (tdt == Tdt_Control_ScopeBeg)
		{
			size_t seq = s_stSequence(ctx, beg + 1, tkend, cont);
			if (seq == CM3L_NONE)
				return CM3L_NONE;

			return s_stWrapSingle(ctx->outp, Stt_Sequence, seq);
		}
		if (tdt == Tdt_Control_ParBeg)
		{
			size_t grp = s_stGrouping(ctx, beg, tkend, cont);
			if (grp == CM3L_NONE)
				return CM3L_NONE;

			return s_stWrapSingle(ctx->outp, Stt_Grouping, grp);
		}
		if (tdt == Tdt_Control_If)
		{
			cm3l_StatementBranch brr;
			size_t cnt;

			brr.cond = processStatement(ctx, beg + 1, tkend, &cnt);
			if (brr.cond == CM3L_NONE)
				return CM3L_NONE;

			brr.body = s_stSequenceOrSingle(ctx, *cont, tkend - 1, cont);
			// if brr.body == CM3L_NONE, it may be empty statement (single ;)

			if (*cont != tkend && data[*cont].data == Tdt_Control_Else)
				brr.elseBr = processStatement(ctx, *cont + 1, tkend, cont);

			size_t pbrr = ctx->outp->branches.length;
			cm3l_VectorPushBackM(&ctx->outp->branches, &brr, cm3l_StatementBranch);

			return s_stWrapSingle(ctx->outp, Stt_Branch, pbrr);
		}
		if (tdt == Tdt_Control_For)
		{
			if (tkend - beg == 1 || data[beg + 1].data != Tdt_Control_ParBeg)
				return CM3L_NONE;

			size_t sc1 = CM3L_NONE, sc2 = CM3L_NONE;
			size_t cnt;

			newScope(ctx);
			for (cnt = beg + 2; cnt < tkend; ++cnt)
			{
				if (data[cnt].data == Tdt_Control_Semicolon)
				{
					if (sc1 == CM3L_NONE)
						sc1 = cnt;
					else if (sc2 == CM3L_NONE)
						sc2 = cnt;
					else return CM3L_NONE;
				}

				if (data[cnt].data == Tdt_Control_ParEnd)
					break;
			}
			if (cnt == tkend || sc1 == CM3L_NONE || sc2 == CM3L_NONE)
				return CM3L_NONE;


			cm3l_StatementForLoop forl;

			forl.cond = processStatement(ctx, beg + 1, tkend, &cnt);
			if (forl.cond == CM3L_NONE)
				return CM3L_NONE;

			forl.body = s_stSequenceOrSingle(ctx, beg + 1, tkend - 1, &cnt);
			size_t pbrr = ctx->outp->forLoops.length;
			cm3l_VectorPushBackM(&ctx->outp->forLoops, &forl, cm3l_StatementForLoop);

			return s_stWrapSingle(ctx->outp, Stt_ForLoop, pbrr);
		}
		if (tdt == Tdt_Control_Return)
		{
			cm3l_StatementUnaryOper uoper = { .type = Tdt_Control_Return };

			if (beg + 1 != tkend) {
				uoper.st = processStatement(ctx, beg + 1, tkend, cont);
			}
			else {
				uoper.st = CM3L_NONE;
				if (cont) *cont = beg + 1;
			}

			size_t p = ctx->outp->unaryOpers.length;
			cm3l_VectorPushBackM(&ctx->outp->unaryOpers, &uoper, cm3l_StatementUnaryOper);

			return s_stWrapSingle(ctx->outp, Stt_UnaryOper, p);
		}
		if (tdt == Tdt_Control_Continue || tdt == Tdt_Control_Break)
		{
			if (cont) *cont = beg + 1;
			cm3l_StatementBasicOper bsoper = { .type = tdt };

			size_t p = ctx->outp->basicOpers.length;
			cm3l_VectorPushBackM(&ctx->outp->basicOpers, &bsoper, cm3l_StatementBasicOper);

			return s_stWrapSingle(ctx->outp, Stt_BasicOper, p);
		}
		if (tdt == Tdt_Control_Function)
		{
			cm3l_StatementInlineFunction funcst;
			size_t body_pos;

			// @todo this is important, in runtime these scopes must repeat the structure of scopes placed here
			// creating some 'scope' instruction and placing it at begin and end of function body is stupid idea cuz we need isolate args
			newScope(ctx);
			if ((funcst.args = s_stGrouping(ctx, beg + 1, tkend, &body_pos)) == CM3L_NONE)
			{
				// @todo report error
				if (cont) *cont = beg + 1;
				endScope(ctx);
				return CM3L_NONE;
			}

			size_t body_end;
			funcst.body = s_stSequenceOrSingle(ctx, body_pos, tkend, &body_end);
			endScope(ctx);
			if (funcst.body == CM3L_NONE)
			{
				// @todo report error
				if (cont) *cont = body_pos;
				return CM3L_NONE;
			}

			size_t p = ctx->outp->inlineFns.length;
			cm3l_VectorPushBackM(&ctx->outp->inlineFns, &funcst, cm3l_StatementInlineFunction);

			return s_stWrapSingle(ctx->outp, Stt_InlineFunction, p);
		}
	}

	size_t stend;
	for (stend = beg + 1; stend != tkend; ++stend) {
		cm3l_TokenData d = data[stend].data;
		if (d == Tdt_Control_Semicolon)
			break;

		cm3l_TokenData closing = Tdt_Control_Semicolon;
		if (d == Tdt_Control_ParBeg)
			closing = Tdt_Control_ParEnd;
		else if (d == Tdt_Control_ScopeBeg)
			closing = Tdt_Control_ScopeEnd;

		size_t ii;
		if (closing != Tdt_Control_Semicolon) {
			for (ii = stend + 1; ii != tkend; ++ii) {
				if (data[ii].data == closing)
					break;
			}
			if (ii == tkend) {
				// @todo report error
				return CM3L_NONE;
			}
			// stend is incremented later, skipping the closing bracket
			stend = ii;
		}
	}

	if (cont) *cont = stend;

	// ==============================
	// operators stuff begins

	for (size_t i = stend - 1; i > beg; --i)
	{
		if (data[i].type != Ttp_Oper)
			continue;

		cm3l_StatementType sttype;
		switch (data[i].data)
		{
			case Tdt_Oper_Assign:
			case Tdt_Oper_AddA:
			case Tdt_Oper_SubA:
			case Tdt_Oper_MulA:
			case Tdt_Oper_DivA:
			case Tdt_Oper_RemA:
			case Tdt_Oper_ShlA:
			case Tdt_Oper_ShrA:
			case Tdt_Oper_XorA:
				sttype = Stt_AssignOper;
				break;

			case Tdt_Oper_IsEqual:
			case Tdt_Oper_IsNEqual:
			case Tdt_Oper_IsGreater:
			case Tdt_Oper_IsGreaterEqual:
			case Tdt_Oper_IsLess:
			case Tdt_Oper_IsLessEqual:

			case Tdt_Oper_Add:
			case Tdt_Oper_Sub:
			case Tdt_Oper_Mul:
			case Tdt_Oper_Div:
			case Tdt_Oper_Rem:
			case Tdt_Oper_Shl:
			case Tdt_Oper_Shr:
			case Tdt_Oper_Xor:

			case Tdt_Oper_Dot:
			case Tdt_Oper_Colon:
			case Tdt_Oper_Comma:
				sttype = Stt_ValueOper;
				break;

			default:
				continue;
		}
		cm3l_StatementBinaryOper opst = { .type = data[i].data };

		opst.first = processStatement(ctx, beg, i, NULL);
		if (opst.first == CM3L_NONE)
			return CM3L_NONE;

		opst.second = processStatement(ctx, i + 1, stend, NULL);
		if (opst.second == CM3L_NONE)
			return CM3L_NONE;

		cm3l_VectorPushBackM(&ctx->outp->binOpers, &opst, cm3l_StatementBinaryOper);
		return s_stWrapSingle (
			ctx->outp,
			sttype,
			ctx->outp->binOpers.length - 1
		);
	}

	// end of operator stuff
	// ==============================

	if (data[stend - 1].type == Ttp_Var)
	{
		size_t reverse_end;
		size_t ref = s_stReferenceFromEnd(ctx, stend - 1, beg - 1, &reverse_end);
		if (ref == CM3L_NONE)
			goto invalid_st;

		if (reverse_end + 1 == beg) {
			// turned out to be 'rvalue' reference
			// need to link to existing ID
			if (cm3l_ResolveReference(ctx, ref, 1)) {
				printf("%s: %zu: %zu: error: failed to resolve reference\n",
					ctx->inp->filename,
					data[stend - 1].line,
					data[stend - 1].symbol
				);
				return CM3L_NONE;
			}
			return s_stWrapSingle (ctx->outp, Stt_Reference, ref);
		}

		// now it's variable declaration. Allocate a new variable for that
		if (cm3l_ResolveReference(ctx, ref, 0)) {
			printf("%s: %zu: %zu: error: failed to allocate variable\n",
				ctx->inp->filename,
				data[stend - 1].line,
				data[stend - 1].symbol
			);
			return CM3L_NONE;
		}

		size_t tpst_cont;
		size_t type_st = processStatement(ctx, beg, reverse_end + 1, &tpst_cont);

		if (tpst_cont != reverse_end + 1)
		{
			printf("%s: %zu: %zu: warn: extra tokens between type statement and identifier\n",
				ctx->inp->filename,
				data[tpst_cont].line,
				data[tpst_cont].symbol
			);
		}
		if (type_st == CM3L_NONE)
		{
			++ctx->errcount;
			printf("%s: %zu: %zu: error: invalid type statement\n",
				ctx->inp->filename,
				data[beg].line,
				data[beg].symbol
			);
			return CM3L_NONE;
		}

		cm3l_Vector *varDecls = &ctx->outp->varDecls;
		cm3l_StatementVarDecl vdst = { .type = type_st, .ref = ref };

		cm3l_VectorPushBackM(varDecls, &vdst, cm3l_StatementVarDecl);

		return s_stWrapSingle (
			ctx->outp,
			Stt_VarDecl,
			varDecls->length - 1
		);
	}

	if (data[beg].type == Ttp_Literal)
	{
		size_t ref = s_stLiteral(ctx, beg, stend, NULL);
		if (ref == CM3L_NONE)
			goto invalid_st;

		return s_stWrapSingle (ctx->outp, Stt_Reference, ref);
	}

invalid_st:
	++ctx->errcount;
	printf("%s: %zu: %zu: error: invalid statement\n",
		ctx->inp->filename,
		data[beg].line,
		data[beg].symbol
	);
	return CM3L_NONE;
}

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

unsigned cm3l_Compose(cm3l_LexerData const *inp, cm3l_ComposerData *outp)
{
	assert(!inp->syntaxErrors);

	cm3l_ComposerContext ctx = {
		.inp = inp,
		.outp = outp,
		.errcount = 0,
		.nesting = 0,
		.vnameRes = cm3l_VectorCreate()
	};

	cm3l_NameResolver nres = {
		.scope = cm3l_HashMapCreate(cm3l_HashFuncS),
		.nextId = 0
	};

	// assign some builtin values
	for (size_t i = 0; i != sizeof(builtins) / sizeof(builtins[0]); ++i)
	{
		size_t value = CM3L_RESERVED;
		cm3l_HashMapEvInsertKV(&nres.scope, builtins[i], builtins_size[i], &value, sizeof(size_t));
	}
	cm3l_VectorPushBackM(&ctx.vnameRes, &nres, cm3l_NameResolver);

	size_t next = 0;
	while (next != inp->tokens.length)
	{
		size_t st = processStatement(&ctx, next, CM3L_NONE, &next);
		if (st != CM3L_NONE)
		{
			// the (st) statement is guaranteed to take place at the end of vector
			cm3l_VectorPushBackA(&outp->toplevel, &st);
		}
	}
	return ctx.errcount;
}
