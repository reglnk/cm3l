#ifndef CM3L_COMPOSER_H
#define CM3L_COMPOSER_H

#include <cm3l/Lib/Vector.h>
#include <cm3l/Lib/HashMapEv.h>
#include <cm3l/Lib/DLList.h>
#include <cm3l/Lexer.h>

#define CM3L_NONE ((size_t)-1)
#define CM3L_RESERVED ((size_t)-2)

#ifdef __cplusplus
extern "C" {
#endif

// annotations
// <st> -- a slot for a statement
// <st:val> -- one that gives value
// <st:ref> -- one that gives reference
// <id> -- would be identified by Lexer as Ttp_Var

typedef enum
{
	Stt_Undefined = 0, // -- no value (reserved)

	Stt_Reference, // <id>; -- gives reference

	Stt_BasicOper, // break, continue, ...
	Stt_UnaryOper, // return, ++, --
	Stt_AssignOper, // <st:ref> [= += -= *= /= ...] <st:val>; -- gives reference
	Stt_ValueOper, // <st:val> [+ - * / ...] <st:val>; -- gives value

	Stt_ForLoop, // for (<st>; <st:val>; <st>) <st> -- no value
	Stt_Branch, // if (<st:val>) { <st>; ... } [else <st:branch>] -- no value

	Stt_VarDecl, // <st:val> <id>; -- gives reference

	Stt_Function, // <st:val> <id> (<st:val> <id>, ...) <st> -- no value
	Stt_InlineFunction, // <st:val> (<st:val> <id>, ...) <st> -- gives value
	Stt_FuncCall, // <st:val> (<st:val>, ...); -- gives value

	Stt_Grouping, // (<st>, ...); -- gives reference or value
	Stt_Sequence, // { <st>; ... } -- gives value

	Stt_Subscript, // <st:val>[<st:val>] -- gives reference
	Stt_MemberAccess // <st:val>.<st:ref> -- gives reference
}
cm3l_StatementType;

typedef struct
{
	cm3l_StatementType type;
	void *container;
	size_t index;
}
cm3l_Statement;

typedef struct cm3l_StatementReference cm3l_StatementReference;

typedef struct cm3l_StatementBasicOper cm3l_StatementBasicOper;
typedef struct cm3l_StatementUnaryOper cm3l_StatementUnaryOper;

// AssignOper, ValueOper
typedef struct cm3l_StatementBinaryOper cm3l_StatementBinaryOper;

typedef struct cm3l_StatementForLoop cm3l_StatementForLoop;
typedef struct cm3l_StatementBranch cm3l_StatementBranch;

typedef struct cm3l_StatementVarDecl cm3l_StatementVarDecl;
typedef struct cm3l_StatementFunction cm3l_StatementFunction;
typedef struct cm3l_StatementInlineFunction cm3l_StatementInlineFunction;
typedef struct cm3l_StatementFuncCall cm3l_StatementFuncCall;

typedef struct cm3l_StatementGrouping cm3l_StatementGrouping;
typedef struct cm3l_StatementSequence cm3l_StatementSequence;

typedef enum
{
	cm3l_SttRefNone = 0,
	cm3l_SttRefIsNested = 1,
	cm3l_SttRefIsAbsolute = 2, // written as ::name
	cm3l_SttRefIsLiteral = 4
}
cm3l_SttRefFlags;
/*
	* |  globalfn -- cm3l_SttRefNone
	* |  ::globalfn -- cm3l_SttRefIsAbsolute
	* |  ::ns::globalfn -- cm3l_SttRefIsNested | cm3l_SttRefIsAbsolute
	* |  ns::globalfn -- cm3l_SttRefIsNested
	*/

typedef struct {
	const char *begin;
	const char *end;
} cm3l_CodeSelectorPtr;

struct cm3l_StatementReference
{
	union {
		cm3l_CodeSelectorPtr regular;
		struct {
			cm3l_CodeSelectorPtr *parts;
			size_t length;
		} complex;
	}
	data;
	cm3l_SttRefFlags flags;

	// displays offset from initial function stack pointer
	size_t id;
};

struct cm3l_StatementBasicOper
{
	cm3l_TokenData type;
};

struct cm3l_StatementUnaryOper
{
	cm3l_TokenData type;
	size_t st; // index of statement
};

struct cm3l_StatementBinaryOper
{
	cm3l_TokenData type;
	size_t first; // stat *
	size_t second; // stat *
};

struct cm3l_StatementForLoop
{
	size_t init; // stat *
	size_t cond; // stat *
	size_t cont; // stat *

	size_t body; // stat *
};

struct cm3l_StatementBranch
{
	size_t cond; // stat *
	size_t elseBr; // stat *

	size_t body; // stat *
};

struct cm3l_StatementVarDecl
{
	size_t type; // statement *
	size_t ref; // reference *
};

struct cm3l_StatementFunction
{
	size_t type; // statement *
	size_t ref; // reference *

	size_t args; // grouping *
	size_t body; // statement *
};

struct cm3l_StatementInlineFunction
{
	size_t args; // grouping *
	size_t body; // statement *
};

struct cm3l_StatementFuncCall
{
	size_t fn; // stat *
	size_t args; // seq *
};

struct cm3l_StatementGrouping
{
	cm3l_Vector members; // of cm3l_Statement *
};

struct cm3l_StatementSequence
{
	cm3l_Vector statements; // of cm3l_Statement *
};

// ==========================================

typedef struct
{
	cm3l_Vector statements; // cm3l_Statement
	cm3l_Vector toplevel; // cm3l_Statement *

	cm3l_Vector references; // cm3l_StatementReference

	cm3l_Vector basicOpers; // cm3l_StatementBasicOper
	cm3l_Vector unaryOpers; // cm3l_StatementUnaryOper
	cm3l_Vector binOpers; // cm3l_StatementBinaryOper

	cm3l_Vector forLoops; // cm3l_StatementForLoop
	cm3l_Vector branches; // cm3l_StatementBranch
	cm3l_Vector varDecls; // cm3l_StatementVarDecl
	cm3l_Vector functions; // cm3l_StatementFunction
	cm3l_Vector inlineFns; // cm3l_StatementInlineFunction
	cm3l_Vector fnCalls; // cm3l_StatementFuncCall

	cm3l_Vector groups; // cm3l_StatementGrouping
	cm3l_Vector sequences; // cm3l_StatementSequence
}
cm3l_ComposerData;

static inline cm3l_ComposerData cm3l_ComposerDataCreate(void)
{
	cm3l_ComposerData data =
	{
		.statements = cm3l_VectorCreate(),
		.toplevel = cm3l_VectorCreate(),

		.references = cm3l_VectorCreate(),

		.basicOpers = cm3l_VectorCreate(),
		.binOpers = cm3l_VectorCreate(),

		.forLoops = cm3l_VectorCreate(),
		.branches = cm3l_VectorCreate(),
		.varDecls = cm3l_VectorCreate(),

		.functions = cm3l_VectorCreate(),
		.inlineFns = cm3l_VectorCreate(),
		.fnCalls = cm3l_VectorCreate(),

		.groups = cm3l_VectorCreate(),
		.sequences = cm3l_VectorCreate()
	};
	return data;
}

typedef struct
{
	cm3l_LexerData const *inp;
	cm3l_ComposerData *outp;
	unsigned int errcount;
	unsigned int nesting;

	// for internal Composer.c usage
	cm3l_Vector /* cm3l_NameResolver */ vnameRes;

	// cm3l_Vector /* imNamePart */ imNameParts;
	cm3l_Vector /* imGrouping */ imGroups;
	cm3l_Vector /* imSequence */ imSequences;
	cm3l_DLList /* imCodeFragment */ fragments;
}
cm3l_ComposerContext;

int cm3l_IsAssignOper(cm3l_TokenData data);

int cm3l_IsValueOper(cm3l_TokenData data);

int cm3l_IsValueStatement(cm3l_StatementType tp);

unsigned cm3l_Compose(cm3l_LexerData const *inp, cm3l_ComposerData *outp);

#ifdef __cplusplus
}
#endif

#endif
