#include <cm3l/Lib/HashMap.h>

// after call of the following function, iterator changes so that:
// 1. it points to next valid key-value pair, or
// 2. switches to invalid state (if it was pointing to the last pair or there're none of them)
// the invalid state is state of iterator which has { .node == NULL }

void cm3l_HashMapNext(cm3l_HashMapIterator *iter)
{
	cm3l_SLNode *node = iter->node;
	if (node != NULL) {
		if (node->next != NULL) {
			iter->node = node->next;
			return;
		}
		++iter->index;
	}
	else {
		if (!iter->map->data.length)
			return;
		iter->index = 0;
	}

	while (iter->index < iter->map->data.length)
	{
		cm3l_SLList *lst = cm3l_VectorAtM(&iter->map->data, iter->index, cm3l_SLList);
		if (lst->length)
		{
			iter->node = lst->first;
			return;
		}
		++iter->index;
	}
	iter->node = NULL;
}

void cm3l_HashMapExpand(cm3l_HashMap *o)
{
	const size_t prevlen = o->data.length;

	const size_t newlen = prevlen ? prevlen * 2 : 1;
	cm3l_VectorResizeExactM(&o->data, newlen, cm3l_SLList);
	for (size_t i = prevlen; i != newlen; ++i)
		*cm3l_VectorAtM(&o->data, i, cm3l_SLList) = cm3l_SLListCreate();

	// get significant bits of hash (for expanded map)
	size_t sig = 0u;
	for (size_t n = prevlen; n; n >>= 1u)
	{
		sig <<= 1u;
		sig ^= 1u;
	}

	// the bitmask determines whether element should be moved to the
	// new half of map or not
	size_t bitsel = sig ^ (sig >> 1u);

	cm3l_SLList *p = o->data.data;
	for (size_t i = 0; i != prevlen; ++i)
	{
		cm3l_SLNode *node = p[i].first;
		cm3l_SLNode *prev = NULL;
		size_t index = 0;

		while (node != NULL)
		{
			cm3l_MapEntryKey entk;
			memcpy(&entk, node->data, sizeof(entk));

			// SLListDetach will set .next of current *node to NULL
			cm3l_SLNode *next = node->next;

			void *keyaddr = CM3L_KEYADDR_SELECT(entk);
			size_t elhash = o->hash_fn(keyaddr, entk.size) & sig;
			if (elhash & bitsel)
			{
				cm3l_SLListDetach(p + i, index);
				cm3l_SLListPushNode(p + i + bitsel, node);
			}
			else ++index;
			node = next;
		}
	}
}

void cm3l_HashMapShrink(cm3l_HashMap *o)
{
	const size_t prevlen = o->data.length;
	assert(prevlen > 1);
	const size_t newlen = prevlen >> 1;

	cm3l_SLList *p = o->data.data;
	for (size_t i = newlen; i != prevlen; ++i)
	{
		cm3l_SLList *bucket = p + i;
		cm3l_SLNode *node;
		while ((node = p[i].first) != NULL)
		{
			cm3l_SLListDetach(bucket, 0);
			cm3l_SLListPushNode(bucket - newlen, node);
		}
	}
	cm3l_VectorResizeExactM(&o->data, newlen, cm3l_SLList);
}

cm3l_SLList *cm3l_HashMapAccessBucket(cm3l_HashMap *o, const void *key, const size_t keysize)
{
	// get significant bits of hash
	size_t sig = 0u;
	for (size_t n = o->data.length; n; n >>= 1u)
	{
		sig <<= 1u;
		sig ^= 1u;
	}
	sig >>= 1u;

	size_t hash = o->hash_fn(key, keysize) & sig;
	return cm3l_VectorAtM(&o->data, hash, cm3l_SLList);
}

cm3l_MapEntryKey *cm3l_HashMapAccessHK(cm3l_HashMap *o, const void *key, const size_t keysize)
{
	if (!o->data.length)
		return NULL;

	cm3l_SLList *lst = cm3l_HashMapAccessBucket(o, key, keysize);
	cm3l_SLNode *node = lst->first;

	while (node != NULL)
	{
		cm3l_MapEntryKey *entk = (void *)node->data;
		void *keyaddr = CM3L_KEYADDR_SELECT(*entk);
		if (entk->size == keysize && !memcmp(keyaddr, key, keysize))
			return entk;

		node = node->next;
	}
	return NULL;
}

void cm3l_HashMapInsert(cm3l_HashMap *o, cm3l_MapEntry e)
{
	if (o->count >= o->data.length)
		cm3l_HashMapExpand(o);

	void *keyaddr = CM3L_KEYADDR_SELECT(e.key);
	cm3l_SLList *lst = cm3l_HashMapAccessBucket(o, keyaddr, e.key.size);

	// iterate over lst to check if there's already such key
	cm3l_SLNode *node = lst->first;
	while (node != NULL)
	{
		cm3l_MapEntry *ent = (cm3l_MapEntry *)(void *)&node->data;
		if (!cm3l_MapKeyCompare(&ent->key, &e.key))
		{
			// full replacement
			cm3l_MapEntryDestroy(ent);
			memcpy(ent, &e, sizeof(cm3l_MapEntry));
			return;
		}
		node = node->next;
	}

	// insert new node
	cm3l_SLListPush(lst, &e, sizeof(e));
	++o->count;
}

size_t cm3l_HashMapEraseK(cm3l_HashMap *o, const void *key, const size_t keysize)
{
	cm3l_SLList *lst = cm3l_HashMapAccessBucket(o, key, keysize);

	cm3l_SLNode *node = lst->first;
	size_t index = 0;
	while (node != NULL)
	{
		cm3l_MapEntry *ent = (cm3l_MapEntry *)(void *)&node->data;
		if (!cm3l_MapKeyCompareK(key, keysize, &ent->key))
		{
			// free node & remove
			cm3l_MapEntryDestroy(ent);
			cm3l_SLListDelete(lst, index);

			// #todo: implement cm3l_HashMapShrink and call here if needed
			--o->count;
			return 1;
		}
		node = node->next;
		++index;
	}
	return 0;
}

void cm3l_HashMapClear(cm3l_HashMap *o)
{
	for (size_t i = 0; i != o->data.length; ++i)
	{
		cm3l_SLList *lst = cm3l_VectorAtM(&o->data, i, cm3l_SLList);
		while (lst->length) {
			cm3l_MapEntry *ent = (cm3l_MapEntry *)(void *)&lst->first->data;
			cm3l_MapEntryDestroy(ent);
			cm3l_SLListPop(lst);
			--o->count;
		}
	}
	assert(!o->count);
}

void cm3l_HashMapDestroy(cm3l_HashMap *o)
{
	cm3l_HashMapClear(o);
	cm3l_VectorDestroy(&o->data);
}

size_t cm3l_HashMapCountCollisions(cm3l_HashMap const *o)
{
	size_t count = 0;
	for (size_t i = 0; i != o->data.length; ++i) {
		cm3l_SLList *lst = cm3l_VectorAtM(&o->data, i, cm3l_SLList);
		if (lst->length > 1) {
			count += lst->length - 1;
		}
	}
	return count;
}
