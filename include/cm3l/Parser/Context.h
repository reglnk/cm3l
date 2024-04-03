#ifndef CM3L_PARSER_INTERNAL_CONTEXT_H
#define CM3L_PARSER_INTERNAL_CONTEXT_H

#include <cm3l/Lexer.h>
#include <cm3l/Parser.h>

typedef struct
{
	cm3l_HashMap /* string, size_t */ scope;
	size_t nextId;
}
cm3l_NameResolver;

typedef struct cm3l_ParserContext
{
	cm3l_LexerData const *inp;
	cm3l_ParserData *outp;
	unsigned int errcount;
	unsigned int nesting;

	cm3l_Vector /* cm3l_NameResolver */ vnameRes;
	cm3l_NameResolver literalRes;

	// cm3l_Vector /* imNamePart */ imNameParts;
	cm3l_Vector /* imGrouping */ imGroups;
	cm3l_Vector /* imSequence */ imSequences;
	cm3l_DLList /* imCodeFragment */ fragments;
}
cm3l_ParserContext;

#endif
