#ifndef COMPOSE_SLLIST_H
#define COMPOSE_SLLIST_H

#include <cm3l/Lib/Memory.h>

#include <stddef.h>
#include <stdint.h> // IWYU pragma: keep
#include <malloc.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cm3l_SLNode cm3l_SLNode;

struct cm3l_SLNode
{
	cm3l_SLNode *next;
	uint8_t data[];
};

typedef struct
{
	cm3l_SLNode *first;
	cm3l_SLNode *last;
	size_t length;
}
cm3l_SLList;

static inline void cm3l_SLListInit(cm3l_SLList *self)
{
	self->first = NULL;
	self->last = NULL;
	self->length = 0;
}

static inline cm3l_SLList cm3l_SLListCreate(void)
{
	cm3l_SLList lst;
	cm3l_SLListInit(&lst);
	return lst;
}

static inline cm3l_SLList *cm3l_SLListNew(void)
{
	cm3l_SLList *lst = cm3l_New(cm3l_SLList);
	cm3l_SLListInit(lst);
	return lst;
}

static inline void cm3l_SLListPushNode(cm3l_SLList *self, cm3l_SLNode *node)
{
	node->next = NULL;

	if (self->last)
		self->last->next = node;
	else self->first = node;

	self->last = node;
	++self->length;
}

static inline void cm3l_SLListPush(cm3l_SLList *self, const void *object, size_t objsize)
{
	cm3l_SLNode *node = (cm3l_SLNode *)cm3l_Alloc(sizeof(cm3l_SLNode) + objsize);
	memcpy(node->data, object, objsize);
	cm3l_SLListPushNode(self, node);
}

// deletes first element
void cm3l_SLListPop(cm3l_SLList *self);

cm3l_SLNode *cm3l_SLListGet(cm3l_SLList *self, size_t index);

cm3l_SLNode *cm3l_SLListDetach(cm3l_SLList *self, size_t index);

static inline void cm3l_SLListDelete(cm3l_SLList *self, size_t index) {
	free(cm3l_SLListDetach(self, index));
}

static inline void cm3l_SLListClear(cm3l_SLList *self)
{
	while (self->length)
		cm3l_SLListPop(self);
}

static inline cm3l_SLNode *cm3l_SLListDetachFirst(cm3l_SLList *self)
{
	assert(self->first);
	assert(self->last);
	assert(self->length);

	--self->length;
	cm3l_SLNode *node = self->first;

	self->first = node->next;
	if (self->first == NULL)
		self->last = NULL;

	return node;
}

cm3l_SLNode *cm3l_SLListDetachNode(cm3l_SLList *self, cm3l_SLNode *prev);

#ifdef __cplusplus
}
#endif

#endif
