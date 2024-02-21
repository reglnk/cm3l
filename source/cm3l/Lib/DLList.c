#include <cm3l/Lib/DLList.h>
#include <cm3l/Lib/Memory.h>

#include <malloc.h>
#include <string.h>
#include <assert.h>

void cm3l_DLListDestroy(cm3l_DLList *self)
{
	while (self->length)
		cm3l_DLListPopFront(self);
}

void cm3l_DLListPushNodeBack(cm3l_DLList *self, cm3l_DLNode *node)
{
	node->next = NULL;

	if ((node->prev = self->last))
		self->last->next = node;
	else self->first = node;
	
	self->last = node;
	++self->length;
}

// [] [] [] [] [] [] [] [] []
// self->first->next->next->next->next;
// self->last->prev->prev->prev->prev
// forward if index 

cm3l_DLNode *cm3l_DLListGet(cm3l_DLList *self, size_t index)
{
	assert(self->first);
	assert(self->last);
	assert(index < self->length);
	
	cm3l_DLNode *iter;
	if (index <= self->length >> 1)
	{
		iter = self->first;
		while (index)
		{
			iter = iter->next;
			--index;
		}
	}
	else
	{
		iter = self->last;
		const size_t last_ind = self->length - 1;
		while (index < last_ind)
		{
			iter = iter->prev;
			++index;
		}
	}
	return iter;
}

void cm3l_DLListDetachNode(cm3l_DLList *self, cm3l_DLNode *node)
{
	if (node->prev != NULL)
		node->prev->next = node->next;
	else self->first = node->next;

	if (node->next != NULL)
		node->next->prev = node->prev;
	else self->last = node->prev;
	
	--self->length;
}
