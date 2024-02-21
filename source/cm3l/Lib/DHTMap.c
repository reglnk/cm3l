#include <cm3l/Lib/DHTMap.h>
#include <cm3l/Lib/Memory.h>
#include <cm3l/Lib/SLList.h>

#include <stdint.h>
#include <stddef.h>

void cm3l_DHTMapInsert(cm3l_DHTMap *o, cm3l_MapEntry e)
{
	size_t hash = o->hash_fn(e.key, e.keysize);
	void **node = (void **)&o->root;
	
	for (unsigned i = 0; i != sizeof(size_t); ++i)
	{
		if (*node == NULL)
			*node = cm3l_DHTMapNodeNew();
		
		const uint8_t index = (uint8_t)(hash >> (i * 8));
		node = &((*(cm3l_DHTMapNode **)node)->branches[index]);
	}
	if (*node == NULL)
		*node = cm3l_SLListNew();
	
	cm3l_SLList *lst = *node;
	cm3l_SLListPush(lst, &e, sizeof(e));
}

cm3l_MapEntry *cm3l_DHTMapAccess(cm3l_DHTMap *o, const void *key, const size_t keysize)
{
	size_t hash = o->hash_fn(key, keysize);
	void *node = o->root;
	
	for (unsigned i = 0; i != sizeof(size_t); ++i)
	{
		if (node == NULL)
			return NULL;
		
		const uint8_t index = (uint8_t)(hash >> (i * 8));
		node = ((cm3l_DHTMapNode *)node)->branches[index];
	}
	if (node == NULL)
		return NULL;
	
	cm3l_SLList *lst = node;
	cm3l_SLNode *iter = lst->first;
	while (iter != NULL)
	{
		cm3l_MapEntry *ent = (void *)iter->data;
		if (ent->keysize == keysize && !memcmp(ent->key, key, keysize))
			return ent;
		
		iter = iter->next;
	}
	return NULL;
}

void cm3l_DHTMapErase(cm3l_DHTMap *o, const void *key, const size_t keysize)
{
	size_t hash = o->hash_fn(key, keysize);
	void **node = (void **)&o->root;
	void **br[sizeof(size_t)];
	
	for (unsigned i = 0; i != sizeof(size_t); ++i)
	{
		if (*node == NULL)
			return;
		
		br[i] = node;
		const uint8_t index = (uint8_t)(hash >> (i * 8));
		node = &((*(cm3l_DHTMapNode **)node)->branches[index]);
	}
	if (*node == NULL)
		return;
	
	cm3l_SLList *lst = *node;
	cm3l_SLNode *iter = lst->first;
	size_t ind = 0; // @todo optimize

	while (iter != NULL)
	{
		cm3l_MapEntry ent;
		memcpy(&ent, iter->data, sizeof(ent));
		if (ent.keysize == keysize && !memcmp(ent.key, key, keysize))
			cm3l_SLListDelete(lst, ind);
		
		iter = iter->next;
		++ind;
	}

	if (!lst->length)
	{
		cm3l_SLListDestroy(lst);
		*node = NULL;

		unsigned i = 8;
		do {
			--i;
			cm3l_DHTMapNode *n = *br[i];
			for (unsigned a = 0; a != 256; ++a)
				if (n->branches[a])
					break;
			
			free(n);
			br[i] = NULL;
		} while (i);
	}
}

/*
void cm3l_DHTMapTest(cm3l_DHTMapIterator *iter)
{
	for (unsigned u = 0; u < sizeof(size_t); ++u)
	{
		int n = iter->npath[u];
		int l = 0;
		if (u + 1 < sizeof(size_t)) {
			l = iter->path[u]->branches[n] != iter->path[u + 1];
		}
		else {
			cm3l_SLList *lst = iter->path[u]->branches[n];
			cm3l_SLNode *n = lst->first;
			l = 1;
			while (n != NULL) {
				if (n == iter->node) {
					l = 0;
					break;
				}
				n = n->next;
			}
		}
		if (l) printf("ERROR AT %u\n", u);
	}
	printf("TEST OK\n");
}
*/

// if some node exists, it must contain at least 1 subnode of 256
// TODO test
void cm3l_DHTMapNext(cm3l_DHTMapIterator *iter)
{
	if (iter->node == NULL)
	{
		void *node = iter->map->root;
		
		for (unsigned u = 0; u != sizeof(size_t); ++u)
		{
			if (node == NULL) return;
			iter->path[u] = node;
			
			unsigned i;
			for (i = 0; i != 256u; ++i)
			{
				cm3l_DHTMapNode *n = node;
				n = n->branches[i];
				if (n != NULL) {
					node = n;
					iter->npath[u] = i;
					break;
				}
			}
			if (i == 256u) return;
		}
		if (node == NULL) return;

		iter->node = ((cm3l_SLList *)node)->first;
		return;
	}

	if (iter->node->next != NULL)
	{
		iter->node = iter->node->next;
		return;
	}
	
	cm3l_SLList *final = NULL;
	int level = sizeof(size_t) - 1;
	int overwrite = 0;
	while (level >= 0 && level < sizeof(size_t))
	{
		// printf("current level: %i %u\n", level, (unsigned)iter->npath[level]);
		unsigned i;
		for (i = overwrite ? 0u : (unsigned)iter->npath[level] + 1u; i != 256u; ++i)
		{
			void *br = iter->path[level]->branches[i];
			if (br != NULL)
			{
				iter->npath[level] = i;
				if (level == 7)
				{
					// printf("found final %u\n", i);
					final = br;
					break;
				}
				// printf("found branch %u\n", i);
				iter->path[level + 1] = br;
				
				break;
			}
		}
		overwrite = 0;
		if (final != NULL) break;
		if (i != 256u) {
			// dont forget to forget previous inner path
			// cuz the (iter->npath[level + 1]) always exists
			// but, for example, if previous path was 0xae - 0xa7 - 0x98 ...
			// and next correct iterator should go through 0xaf - 0xa7 - whatever ...
			// it must check all values from 0x00 to 0xfe in next loop, not only 0xa7 ... 0xfe
			overwrite = 1;
			++level;
		}
		else --level;
	}
	if (final == NULL)
	{
		iter->node = NULL;
		return;
	}
	iter->node = final->first;
}
