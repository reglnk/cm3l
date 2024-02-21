#ifndef CM3L_LIB_HASH_MAP_EV_H
#define CM3L_LIB_HASH_MAP_EV_H

#include <cm3l/Lib/HashMap.h>

// the hash map with embedded values (inherited from HashMap)

#ifdef __cplusplus
extern "C" {
#endif

static inline cm3l_MapEntryEv *cm3l_HashMapEvIterAccess(cm3l_HashMapIterator const *iter) {
	return (cm3l_MapEntryEv *)(void *)iter->node->data;
}

void cm3l_HashMapEvInsert (
	cm3l_HashMap *o,
	cm3l_MapEntryKey key,
	const void *v, const size_t vs
);

static inline void cm3l_HashMapEvInsertKV (
	cm3l_HashMap *o,
	const void *k, const size_t ks,
	const void *v, const size_t vs
) {
	cm3l_HashMapEvInsert(o, cm3l_MapEntryKeyCreate(k, ks), v, vs);
}

static inline cm3l_MapEntryEv *cm3l_HashMapEvAccessK(cm3l_HashMap *o, const void *key, const size_t keysize) {
	return (cm3l_MapEntryEv *)cm3l_HashMapAccessHK(o, key, keysize);
}

// not inherent in just HashMap
static inline void *cm3l_HashMapEvAt(cm3l_HashMap *o, const void *key, const size_t keysize) {
	cm3l_MapEntryEv *ent = (cm3l_MapEntryEv *)cm3l_HashMapAccessHK(o, key, keysize);
	return ent != NULL ? &ent->value : NULL;
}

size_t cm3l_HashMapEvEraseK(cm3l_HashMap *o, const void *key, const size_t keysize);

static inline size_t cm3l_HashMapEvErase(cm3l_HashMap *o, cm3l_MapEntryKey key) {
	void *keyaddr = CM3L_KEYADDR_SELECT(key);
	return cm3l_HashMapEraseK(o, keyaddr, key.size);
}

// clears all elements but keeps all buckets
void cm3l_HashMapEvClear(cm3l_HashMap *o);

void cm3l_HashMapEvDestroy(cm3l_HashMap *o);

#ifdef __cplusplus
}
#endif

#endif
