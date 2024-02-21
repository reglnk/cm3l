#ifndef CM3L_DLLIST_H
#define CM3L_DLLIST_H

#include <cm3l/Lib/Memory.h>

#include <stddef.h>
#include <malloc.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cm3l_DLNode cm3l_DLNode;

struct cm3l_DLNode
{
	cm3l_DLNode *next;
	cm3l_DLNode *prev;
	char data[];
};

static inline cm3l_DLNode *cm3l_DLNodeNew(const void *object, size_t objsize)
{
	cm3l_DLNode *node = (cm3l_DLNode *)cm3l_Alloc(sizeof(cm3l_DLNode) + objsize);
	memcpy(node->data, object, objsize);
	return node;
}

#define cm3l_DLNodeNewA(obj) cm3l_DLNodeNew((obj), sizeof(*(obj)));
#define cm3l_DLNodeNewV(obj) cm3l_DLNodeNew(&(obj), sizeof(obj));

typedef struct
{
	cm3l_DLNode *first;
	cm3l_DLNode *last;
	size_t length;
}
cm3l_DLList;

static inline void cm3l_DLListInit(cm3l_DLList *self)
{
	self->first = NULL;
	self->last = NULL;
	self->length = 0;
}

static inline cm3l_DLList cm3l_DLListCreate(void)
{
	cm3l_DLList lst;
	cm3l_DLListInit(&lst);
	return lst;
}

static inline cm3l_DLList *cm3l_DLListNew(void)
{
	cm3l_DLList *lst = cm3l_New(cm3l_DLList);
	cm3l_DLListInit(lst);
	return lst;
}

void cm3l_DLListDestroy(cm3l_DLList *self);

// @todo cm3l_DLListPushNodeFront

void cm3l_DLListPushNodeBack(cm3l_DLList *self, cm3l_DLNode *node);

static inline void cm3l_DLListPushBack(cm3l_DLList *self, const void *object, size_t objsize)
{
	cm3l_DLNode *node = (cm3l_DLNode *)cm3l_Alloc(sizeof(cm3l_DLNode) + objsize);
	memcpy(node->data, object, objsize);
	cm3l_DLListPushNodeBack(self, node);
}

#define cm3l_DLListPushBackM(self, obj, type) cm3l_DLListPushBack(self, obj, sizeof(type))
#define cm3l_DLListPushBackV(self, obj) cm3l_DLListPushBack(self, &(obj), sizeof(obj))

cm3l_DLNode *cm3l_DLListGet(cm3l_DLList *self, size_t index);

static inline void cm3l_DLListInsertL (
	cm3l_DLList *self,
	cm3l_DLNode *node,
	cm3l_DLNode *dest
) {
	if (dest->prev != NULL)
		dest->prev->next = node;

	if (dest == self->first)
		self->first = node;

	node->prev = dest->prev;
	dest->prev = node;
	node->next = dest;

	++self->length;
}

static inline void cm3l_DLListInsertR (
	cm3l_DLList *self,
	cm3l_DLNode *node,
	cm3l_DLNode *dest
) {
	if (dest->next != NULL)
		dest->next->prev = node;

	if (dest == self->last)
		self->last = node;

	node->next = dest->next;
	dest->next = node;
	node->prev = dest;

	++self->length;
}

static inline void cm3l_DLListInsertNode(cm3l_DLList *self, size_t index, cm3l_DLNode *node)
{
	if (index < self->length)
	{
		cm3l_DLNode *d = cm3l_DLListGet(self, index);
		cm3l_DLListInsertL(self, node, d);
	}
	else if (self->length)
		cm3l_DLListInsertR(self, node, self->last);
	else cm3l_DLListPushNodeBack(self, node);
}

// insert to right side, with check
static inline void cm3l_DLListInsertRc (
	cm3l_DLList *self,
	cm3l_DLNode *node,
	cm3l_DLNode *dest
) {
	if (dest == NULL)
		cm3l_DLListInsertNode(self, 0, node);
	else cm3l_DLListInsertR(self, node, dest);
}

// insert to left side, with check
static inline void cm3l_DLListInsertLc (
	cm3l_DLList *self,
	cm3l_DLNode *node,
	cm3l_DLNode *dest
) {
	if (dest == NULL)
		cm3l_DLListPushNodeBack(self, node);
	else cm3l_DLListInsertL(self, node, dest);
}

static inline void cm3l_DLListInsert (
	cm3l_DLList *self,
	size_t index,
	const void *object,
	size_t objsize
) {
	cm3l_DLNode *node = (cm3l_DLNode *)cm3l_Alloc(sizeof(cm3l_DLNode) + objsize);
	memcpy(node->data, object, objsize);
	cm3l_DLListInsertNode(self, index, node);
}

void cm3l_DLListDetachNode(cm3l_DLList *self, cm3l_DLNode *node);

static inline void cm3l_DLListDeleteNode(cm3l_DLList *self, cm3l_DLNode *node)
{
	cm3l_DLListDetachNode(self, node);
	free(node);
}

static inline void cm3l_DLListPopFront(cm3l_DLList *self)
{
	cm3l_DLNode *node = self->first;
	cm3l_DLListDetachNode(self, node);
	free(node);
}

static inline void cm3l_DLListPopBack(cm3l_DLList *self)
{
	cm3l_DLNode *node = self->last;
	cm3l_DLListDetachNode(self, node);
	free(node);
}

static inline cm3l_DLNode *cm3l_DLListDetach(cm3l_DLList *self, size_t index)
{
	cm3l_DLNode *node = cm3l_DLListGet(self, index);
	cm3l_DLListDetachNode(self, node);
	return node;
}

static inline void cm3l_DLListDelete(cm3l_DLList *self, size_t index) {
	free(cm3l_DLListDetach(self, index));
}

#ifdef __cplusplus
}
#endif

#endif
