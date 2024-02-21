#include <cm3l/Lib/HashMapEv.h>

// note that significant part of code for HashMapEv is shared and located in HashMap.c

void cm3l_HashMapEvInsert (
	cm3l_HashMap *o,
	cm3l_MapEntryKey key,
	const void *v, const size_t vs
) {
	if (o->count >= o->data.length)
		cm3l_HashMapExpand(o);
	
	void *keyaddr = CM3L_KEYADDR_SELECT(key);
	cm3l_SLList *lst = cm3l_HashMapAccessBucket(o, keyaddr, key.size);
	
	// iterate over lst to check if there's already such key
	cm3l_SLNode *node = lst->first;
	while (node != NULL)
	{
		cm3l_MapEntryEv *ent = (cm3l_MapEntryEv *)(void *)&node->data;
		if (!cm3l_MapKeyCompare(&key, &ent->key))
		{
			// full replacement
			cm3l_MapEntryEvDestroy(ent);
			memcpy(ent, &key, sizeof(cm3l_MapEntryKey));
			memcpy(&ent->value, v, vs);			
			return;
		}
		node = node->next;
	}
	
	// insert new node
	node = cm3l_Alloc(sizeof(cm3l_SLNode) + sizeof(cm3l_MapEntryEv) + vs);
	cm3l_MapEntryEv *ent = (cm3l_MapEntryEv *)(void *)&node->data;
	
	memcpy(ent, &key, sizeof(cm3l_MapEntryKey));
	memcpy(&ent->value, v, vs);
	
	cm3l_SLListPushNode(lst, node);
	++o->count;
}

size_t cm3l_HashMapEvEraseK(cm3l_HashMap *o, const void *key, const size_t keysize)
{
	cm3l_SLList *lst = cm3l_HashMapAccessBucket(o, key, keysize);
	
	cm3l_SLNode *node = lst->first;
	size_t index = 0;
	while (node != NULL)
	{
		cm3l_MapEntryEv *ent = (cm3l_MapEntryEv *)(void *)&node->data;
		if (!cm3l_MapKeyCompareK(key, keysize, &ent->key))
		{
			// free node & remove
			cm3l_MapEntryEvDestroy(ent);
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

void cm3l_HashMapEvClear(cm3l_HashMap *o)
{
	for (size_t i = 0; i != o->data.length; ++i)
	{
		cm3l_SLList *lst = cm3l_VectorAtM(&o->data, i, cm3l_SLList);
		while (lst->length) {
			cm3l_MapEntryEv *ent = (cm3l_MapEntryEv *)(void *)&lst->first->data;
			cm3l_MapEntryEvDestroy(ent);
			cm3l_SLListPop(lst);
			--o->count;
		}
	}
	assert(!o->count);
}

void cm3l_HashMapEvDestroy(cm3l_HashMap *o)
{
	cm3l_HashMapEvClear(o);
	cm3l_VectorDestroy(&o->data);
}
