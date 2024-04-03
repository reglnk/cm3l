#ifndef CM3L_LOADTIME_NAME_RESOLVER
#define CM3L_LOADTIME_NAME_RESOLVER

#include <cm3l/Parser/Context.h>

#ifdef __cplusplus
extern "C" {
#endif

// returns the length of resulting string. If buf is NULL, the string is not written.
size_t cm3l_RefStatToLinear(cm3l_StatementReference const *refst, char *buf);

// links the reference to an existing variable or creates a new one
// if reference allocation fails, returns non-zero value
// @param refType
// 0: declaration
// 1: link to existing variable
int cm3l_ResolveReference (
	cm3l_ParserContext *ctx,
	size_t refId,
	int refType
);

#ifdef __cplusplus
}
#endif

#endif
