#ifndef CM3L_CONTEXT_H
#define CM3L_CONTEXT_H

#include <cm3l/Lib/IdMap.h>
#include <cm3l/Composer.h>
#include <cm3l/Runtime/Object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cm3l_Context
{
	cm3l_IdMap objects;
}
cm3l_Context;

int cm3l_ContextExecStatement(cm3l_Context *self, cm3l_ComposerData const *code, cm3l_Statement const *st);

int cm3l_ContextExecute(cm3l_Context *self, cm3l_ComposerData const *code);

#ifdef __cplusplus
}
#endif

#endif
