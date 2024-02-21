#ifndef COMPOSE_DISTRIBUTED_HASH_MAP_H
#define COMPOSE_DISTRIBUTED_HASH_MAP_H

#include <cm3l/Lib/Memory.h>
#include <cm3l/Lib/SLList.h>
#include <cm3l/Lib/Hash.h>
#include <cm3l/Lib/MapCommon.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	void *branches[256];
}
cm3l_DHTMapNode;

static inline cm3l_DHTMapNode *cm3l_DHTMapNodeNew(void)
{
	cm3l_DHTMapNode *node = cm3l_New(cm3l_DHTMapNode);
	for (unsigned i = 0u; i != 256u; ++i)
		node->branches[i] = NULL;
	return node;
}

typedef struct
{
	cm3l_DHTMapNode *root;
	cm3l_HashFunc hash_fn;
}
cm3l_DHTMap;

static inline cm3l_DHTMap cm3l_DHTMapCreate(cm3l_HashFunc fn)
{
	cm3l_DHTMap m = { .root = NULL, .hash_fn = fn };
	return m;
}

// TODO make deletion by iterator without need to access yet 1 time

// check validity: .node != NULL
typedef struct
{
	cm3l_DHTMap *map;
	cm3l_DHTMapNode *path[sizeof(size_t)];
	uint8_t npath[sizeof(size_t)];
	cm3l_SLNode *node;
}
cm3l_DHTMapIterator;

static inline void cm3l_DHTMapIteratorInit(cm3l_DHTMapIterator *self, cm3l_DHTMap *m)
{
	self->map = m;
	for (unsigned i = 0; i != sizeof(size_t); ++i)
		self->path[i] = NULL;
	self->node = NULL;
}

static inline cm3l_DHTMapIterator cm3l_DHTMapIteratorCreate(cm3l_DHTMap *m)
{
	cm3l_DHTMapIterator iter;
	cm3l_DHTMapIteratorInit(&iter, m);
	return iter;
}

/* void cm3l_DHTMapTest(cm3l_DHTMapIterator *iter); */

void cm3l_DHTMapNext(cm3l_DHTMapIterator *iter);

void cm3l_DHTMapInsert(cm3l_DHTMap *o, cm3l_MapEntry e);

cm3l_MapEntry *cm3l_DHTMapAccess(cm3l_DHTMap *o, const void *key, const size_t keysize);

static inline cm3l_MapEntry *cm3l_DHTMapIterAccess(cm3l_DHTMapIterator const *iter)
{
	return (void *)iter->node->data;
}

void cm3l_DHTMapErase(cm3l_DHTMap *o, const void *key, const size_t keysize);

void cm3l_DHTMapDestroy(cm3l_DHTMap *o);

#ifdef __cplusplus
}
#endif

#endif
