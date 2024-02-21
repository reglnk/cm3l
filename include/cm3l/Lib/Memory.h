#ifndef CM3L_MEMORY_H
#define CM3L_MEMORY_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *cm3l_Alloc(size_t s)
{
	void *mem = malloc(s);
	if (!mem) exit(3);
	return mem;
}

#define cm3l_New(type) ((type *)cm3l_Alloc(sizeof(type)))

static inline void *cm3l_Realloc(void *mem, size_t s)
{
	if (!s) {
		free(mem);
		return NULL;
	}
	
	mem = realloc(mem, s);
	if (mem == NULL)
		exit(3);
	
	return mem;
}

#ifdef __cplusplus
}
#endif

#endif
