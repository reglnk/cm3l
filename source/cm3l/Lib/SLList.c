#include <cm3l/Lib/SLList.h>
#include <cm3l/Lib/Memory.h>

#include <malloc.h>
#include <string.h>
#include <assert.h>

void cm3l_SLListPop(cm3l_SLList *self)
{
	assert(self->first);
	assert(self->last);
	assert(self->length);

	--self->length;

	cm3l_SLNode *node = self->first;
	self->first = node->next;

	if (!self->first)
		self->last = NULL;

	free(node);
}

cm3l_SLNode *cm3l_SLListGet(cm3l_SLList *self, size_t index)
{
	assert(self->first);
	assert(self->last);
	assert(index < self->length);

	cm3l_SLNode *iter = self->first;
	while (index)
	{
		iter = iter->next;
		--index;
	}
	return iter;
}

cm3l_SLNode *cm3l_SLListDetach(cm3l_SLList *self, size_t index)
{
	assert(self->first);
	assert(self->last);
	assert(index < self->length);

	--self->length;
	cm3l_SLNode *node = self->first;

	if (!index)
	{
		self->first = node->next;
		if (self->first == NULL)
			self->last = NULL;

		return node;
	}

	while (--index)
		node = node->next;

	cm3l_SLNode *detached = node->next;
	assert(detached);

	node->next = detached->next;
	if (detached == self->last)
		self->last = node;

	return detached;
}

cm3l_SLNode *cm3l_SLListDetachNode(cm3l_SLList *self, cm3l_SLNode *prev)
{
	assert(self->first);
	assert(self->last);

	--self->length;
	cm3l_SLNode *node = self->first;

	if (prev == NULL)
	{
		self->first = node->next;
		if (self->first == NULL)
			self->last = NULL;

		return node;
	}

	cm3l_SLNode *detached = prev->next;
	assert(detached);

	prev->next = detached->next;
	if (detached == self->last)
		self->last = prev;

	return detached;
}
