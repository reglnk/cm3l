#ifndef COMPOSE_RUNTIME_H
#define COMPOSE_RUNTIME_H

#include <cm3l/Lib/Vector.h>
#include <cm3l/Lexer.h>
#include <cm3l/Composer.h>

typedef struct
{
	// id of object with type
	uint32_t typeId;
	
	uint32_t size;
	void *data;
}
CmObject;

typedef void (*CmObjectFuncCreate)(CmObject *obj);
typedef void (*CmObjectFuncDestroy)(CmObject *obj);

typedef struct
{
	CmObjectFuncCreate createFn;
	CmObjectFuncDestroy destroyFn;
}
CmObjectType;

typedef struct
{
	CmVector objects; // of uint32_t
	CmVector scopes; // of CmScope
	
	// unsigned numStatementExecuted ? or not
}
CmScope;

typedef struct
{
	IDMap objects; // uint32_t to CmObject
	CmScope global;
}
CmRuntimeData;

int CmExecuteStatement(CmComposerData const *code, CmStatement const *st);

int CmExecute(CmComposerData const *code);

#endif
