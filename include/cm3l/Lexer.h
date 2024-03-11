#ifndef COMPOSE_LEXER_H
#define COMPOSE_LEXER_H

#include <cm3l/Lib/Vector.h>

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CM3L_NUM_BINARY_OPERATORS_A 9
#define CM3L_NUM_BINARY_OPERATORS_V 17
#define CM3L_NUM_OPERATORS \
	(CM3L_NUM_BINARY_OPERATORS_A + CM3L_NUM_BINARY_OPERATORS_V + 1) // +1 for ::

#define CM3L_NUM_CONTROLS 15

#define CM3L_OK 0
#define CM3L_ERROR 1

typedef enum cm3l_TokenType
{
	Ttp_Undefined = 0x0,
	Ttp_Oper      = 0x1,
	Ttp_Control   = 0x2,
	Ttp_Var       = 0x4,
	Ttp_Literal   = 0x8
}
cm3l_TokenType;

typedef enum
{
	Tdt_Undefined = 0,

	Tdt_Oper_Assign, // =
	Tdt_Oper_AddA, // +=
	Tdt_Oper_SubA, // -=
	Tdt_Oper_MulA, // *=
	Tdt_Oper_DivA, // /=
	Tdt_Oper_RemA, // %=
	Tdt_Oper_ShlA, // <<=
	Tdt_Oper_ShrA, // >>=
	Tdt_Oper_XorA, // ^=

	Tdt_Oper_IsEqual, // ==
	Tdt_Oper_IsNEqual, // !=
	Tdt_Oper_IsGreater, // >
	Tdt_Oper_IsGreaterEqual, // >=
	Tdt_Oper_IsLess, // <
	Tdt_Oper_IsLessEqual, // <=

	Tdt_Oper_Add, // +
	Tdt_Oper_Sub, // -
	Tdt_Oper_Mul, // *
	Tdt_Oper_Div, // /
	Tdt_Oper_Rem, // %
	Tdt_Oper_Shl, // <<
	Tdt_Oper_Shr, // >>
	Tdt_Oper_Xor, // ^

	Tdt_Oper_Dot, // .
	Tdt_Oper_Colon, // :
	Tdt_Oper_Comma, // ,

	Tdt_Oper_ScopeRes, // ::

	Tdt_Control_Return, // return
	Tdt_Control_ScopeBeg, // {
	Tdt_Control_ScopeEnd, // }
	Tdt_Control_ParBeg, // (
	Tdt_Control_ParEnd, // )
	Tdt_Control_SbrkOpen, // [
	Tdt_Control_SbrkClose, // ]
	Tdt_Control_If, // if
	Tdt_Control_Else, // else
	Tdt_Control_Break, // break
	Tdt_Control_Continue, // continue
	Tdt_Control_For, // for
	Tdt_Control_Semicolon, // ;
	Tdt_Control_Require, // require
	Tdt_Control_Function, // function

	/* Tdt_Var_Value, */
	/* Tdt_Var_Reference, */

	Tdt_Literal_String,
	Tdt_Literal_Int,
	Tdt_Literal_Float
}
cm3l_TokenData;

static inline int32_t cm3l_getBinaryOperNumA(cm3l_TokenData oper)
{
	switch (oper)
	{
		case Tdt_Oper_Assign: return 0;
		case Tdt_Oper_AddA: return 1;
		case Tdt_Oper_SubA: return 2;
		case Tdt_Oper_MulA: return 3;
		case Tdt_Oper_DivA: return 4;
		case Tdt_Oper_RemA: return 5;
		case Tdt_Oper_ShlA: return 6;
		case Tdt_Oper_ShrA: return 7;
		case Tdt_Oper_XorA: return 8;
		default:
			return -1;
	}
}

static inline int32_t cm3l_getBinaryOperNumV(cm3l_TokenData oper)
{
	switch (oper)
	{
		case Tdt_Oper_IsEqual: return 0;
		case Tdt_Oper_IsNEqual: return 1;
		case Tdt_Oper_IsGreater: return 2;
		case Tdt_Oper_IsGreaterEqual: return 3;
		case Tdt_Oper_IsLess: return 4;
		case Tdt_Oper_IsLessEqual: return 5;

		case Tdt_Oper_Add: return 6;
		case Tdt_Oper_Sub: return 7;
		case Tdt_Oper_Mul: return 8;
		case Tdt_Oper_Div: return 9;
		case Tdt_Oper_Rem: return 10;
		case Tdt_Oper_Shl: return 11;
		case Tdt_Oper_Shr: return 12;
		case Tdt_Oper_Xor: return 13;

		case Tdt_Oper_Dot: return 14;
		case Tdt_Oper_Colon: return 15;
		case Tdt_Oper_Comma: return 16;

		default:
			return -1;
	}
}

typedef struct
{
	cm3l_TokenType type;
	cm3l_TokenData data;

	size_t line;
	size_t symbol;

	const char *beg;
	const char *end;
}
cm3l_Token;

typedef struct
{
	char *filename;
	char *buffer;
	size_t bufsize;
	cm3l_Vector tokens;

	// @deprecated (TODO: delete from here)
	int syntaxErrors;
}
cm3l_LexerData;

static inline cm3l_LexerData cm3l_LexerDataCreate()
{
	cm3l_LexerData lex = { .filename = NULL, .buffer = NULL, .bufsize = 0u, .tokens = cm3l_VectorCreate(), .syntaxErrors = 0 };
	return lex;
}

// @unused
static inline void cm3l_LexerDataDestroy(cm3l_LexerData *lex)
{
	if (lex->filename != NULL)
		free(lex->filename);
	if (lex->buffer != NULL)
		free(lex->buffer);
	cm3l_VectorDestroy(&lex->tokens);
}

/* @deprecated
cm3l_Token cm3l_RecognizeToken(const char *beg, const char *end); */

const char *cm3l_OperString(cm3l_TokenData data);

const char *cm3l_ControlString(cm3l_TokenData data);

// retrieves tokens from lex->buffer. If buffer is NULL, reads buffer from filename.
int cm3l_LexerProcess(cm3l_LexerData *lex, const char *filename);

#ifdef __cplusplus
}
#endif

#endif
