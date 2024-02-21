#ifndef CM3L_LIB_HASH_H
#define CM3L_LIB_HASH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*cm3l_HashFunc)(const void *mem, size_t size);

size_t cm3l_HashFuncS(const void *mem, size_t size);

size_t cm3l_HashFuncS64(const void *mem, size_t size);

size_t cm3l_HashFuncS32(const void *mem, size_t size);

size_t cm3l_HashFuncS16(const void *mem, size_t size);

size_t cm3l_HashFuncS8(const void *mem, size_t size);

#if __SIZEOF_SIZE_T__ == 8
#  define cm3l_HashFuncSsize cm3l_HashFuncS64
#elif __SIZEOF_SIZE_T__ == 4
#  define cm3l_HashFuncSsize cm3l_HashFuncS32
#else
#  define cm3l_HashFuncSsize cm3l_HashFuncS16
#endif

#ifdef __cplusplus
}
#endif

#endif
