#ifndef CM3L_LIB_MAP_COMMON_H
#define CM3L_LIB_MAP_COMMON_H

#include <cm3l/Lib/Memory.h>

#include <stddef.h>
#include <stdint.h> // IWYU pragma: keep
#include <malloc.h>
#include <string.h>

#define CM3L_MAPKEY_COMPACT 8

typedef struct
{
	size_t size;
	
	// if size <= 8 then key is inline (stored in compact),
	// otherwise it's stored in memory which large points to.
	//
	// @def: allocated entry key is cm3l_MapEntryKey with size > 8
	// @def: unallocated entry key is the same with size <= 8
	union {
		void *ptr;
		uint64_t compact;
	} data;
}
cm3l_MapEntryKey;

#define CM3L_KEYADDR_SELECT(p) \
	(p).size > CM3L_MAPKEY_COMPACT ? (p).data.ptr : &(p).data.compact

static inline int cm3l_MapKeyCompare (
	cm3l_MapEntryKey const *k0,
	cm3l_MapEntryKey const *k1
) {
	if (k0->size != k1->size)
		return 1;
	
	const void *k0addr = CM3L_KEYADDR_SELECT(*k0);
	const void *k1addr = CM3L_KEYADDR_SELECT(*k1);
	return memcmp(k0addr, k1addr, k0->size);
}

static inline int cm3l_MapKeyCompareK (
	const void *k, const size_t ks,
	cm3l_MapEntryKey const *k1
) {
	if (ks != k1->size)
		return 1;
	
	const void *k1addr = CM3L_KEYADDR_SELECT(*k1);
	return memcmp(k, k1addr, ks);
}

typedef struct
{
	cm3l_MapEntryKey key;
	size_t valsize;
	void *value;
}
cm3l_MapEntry;

typedef struct
{
	cm3l_MapEntryKey key;
	uint8_t value[];
}
cm3l_MapEntryEv;

static inline cm3l_MapEntryKey cm3l_MapEntryKeyCreate (
	const void *k, const size_t ks
) {
	cm3l_MapEntryKey key = { .size = ks };
	void *keyaddr;
	
	if (ks > CM3L_MAPKEY_COMPACT)
		keyaddr = key.data.ptr = cm3l_Alloc(ks);
	else keyaddr = &key.data.compact;
	memcpy(keyaddr, k, ks);	
	
	return key;
}

// the key cannot be reused if it's allocated
static inline cm3l_MapEntry cm3l_MapEntryCreate (
	cm3l_MapEntryKey key,
	const void *v, const size_t vs
) {
	cm3l_MapEntry ent = {.key = key, .valsize = vs, .value = malloc(vs)};
	memcpy(ent.value, v, vs);
	return ent;
}

static inline cm3l_MapEntry cm3l_MapEntryCreateK (
	const void *k, const size_t ks,
	const void *v, const size_t vs
) {
	cm3l_MapEntryKey key = cm3l_MapEntryKeyCreate(k, ks);
	return cm3l_MapEntryCreate(key, v, vs);
}

static inline cm3l_MapEntryEv *cm3l_MapEntryEvNew (
	cm3l_MapEntryKey key,
	const void *v, const size_t vs
) {
	cm3l_MapEntryEv *ent = (cm3l_MapEntryEv *)malloc(sizeof(cm3l_MapEntryEv) + vs);
	ent->key = key;
	memcpy(&ent->value, v, vs);
	return ent;
}

static inline void cm3l_MapEntryDestroy(cm3l_MapEntry *e)
{
	if (e->key.size > CM3L_MAPKEY_COMPACT)
		free(e->key.data.ptr);
	free(e->value);
}

static inline void cm3l_MapEntryEvDestroy(cm3l_MapEntryEv *e)
{
	if (e->key.size > CM3L_MAPKEY_COMPACT)
		free(e->key.data.ptr);
}

#endif
