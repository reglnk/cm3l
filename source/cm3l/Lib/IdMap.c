#include <cm3l/Lib/IdMap.h>

size_t cm3l_IdMapInsert(cm3l_IdMap *self, const void *obj, const size_t objsize)
{
	if (self->freeIds.length)
	{
		size_t ind = *(size_t *)cm3l_VectorBack(&self->freeIds, sizeof(size_t));
		cm3l_VectorPopBack(&self->freeIds, sizeof(size_t));
		((uint8_t *)self->active.data)[ind] = 1u;
		memcpy(cm3l_VectorAt(&self->objects, ind, objsize), obj, objsize);
		return ind;
	}
	else
	{
		cm3l_VectorPushBack(&self->objects, obj, objsize);
		cm3l_VectorPushBack(&self->active, "\001", 1);
		return self->objects.length - 1;
	}
}
