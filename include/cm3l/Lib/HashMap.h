#ifndef CM3L_LIB_HASH_MAP_H
#define CM3L_LIB_HASH_MAP_H

#include <cm3l/Lib/Memory.h>
#include <cm3l/Lib/SLList.h>
#include <cm3l/Lib/Vector.h>
#include <cm3l/Lib/Hash.h>
#include <cm3l/Lib/MapCommon.h>

#include <stdlib.h>
#include <stdint.h> // IWYU pragma: keep
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	cm3l_Vector data;
	cm3l_HashFunc hash_fn;
	size_t count;
}
cm3l_HashMap;

static inline cm3l_HashMap cm3l_HashMapCreate(cm3l_HashFunc fn)
{
	cm3l_HashMap m = {.data = cm3l_VectorCreate(), .hash_fn = fn, .count = 0u};
	return m;
}

static inline void cm3l_HashMapInit(cm3l_HashMap *self, cm3l_HashFunc fn) {
	*self = (cm3l_HashMap) {.data = cm3l_VectorCreate(), .hash_fn = fn, .count = 0u};
}

// check validity: .node != NULL
typedef struct
{
	cm3l_HashMap *map;
	size_t index;
	cm3l_SLNode *node;
}
cm3l_HashMapIterator;

static inline cm3l_HashMapIterator cm3l_HashMapIteratorCreate(cm3l_HashMap *m)
{
	cm3l_HashMapIterator iter = { .map = m, .index = 0u, .node = NULL };
	return iter;
}

static inline cm3l_MapEntry *cm3l_HashMapIterAccess(cm3l_HashMapIterator const *iter) {
	return (cm3l_MapEntry *)(void *)iter->node->data;
}

void cm3l_HashMapNext(cm3l_HashMapIterator *iter);

void cm3l_HashMapExpand(cm3l_HashMap *o);

cm3l_SLList *cm3l_HashMapAccessBucket(cm3l_HashMap *o, const void *key, const size_t keysize);

// the allocated data from map entry cannot be reused after insertion by that function
void cm3l_HashMapInsert(cm3l_HashMap *o, cm3l_MapEntry e);

static inline void cm3l_HashMapInsertKV (
	cm3l_HashMap *o,
	const void *k, const size_t ks,
	const void *v, const size_t vs
) {
	cm3l_HashMapInsert(o, cm3l_MapEntryCreateK(k, ks, v, vs));
}

// the returned pointer can be casted to cm3l_MapEntry* or other type if stored so
cm3l_MapEntryKey *cm3l_HashMapAccessHK(cm3l_HashMap *o, const void *key, const size_t keysize);

static inline cm3l_MapEntry *cm3l_HashMapAccessK(cm3l_HashMap *o, const void *key, const size_t keysize) {
	return (cm3l_MapEntry *)cm3l_HashMapAccessHK(o, key, keysize);
}

size_t cm3l_HashMapEraseK(cm3l_HashMap *o, const void *key, const size_t keysize);

static inline size_t cm3l_HashMapErase(cm3l_HashMap *o, cm3l_MapEntryKey key) {
	void *keyaddr = CM3L_KEYADDR_SELECT(key);
	return cm3l_HashMapEraseK(o, keyaddr, key.size);
}

// clears all elements but keeps all buckets
void cm3l_HashMapClear(cm3l_HashMap *o);

void cm3l_HashMapDestroy(cm3l_HashMap *o);

size_t cm3l_HashMapCountCollisions(cm3l_HashMap const *o);

#ifdef __cplusplus
}
#endif

#endif
