#ifndef CM3L_IDMAP_H
#define CM3L_IDMAP_H

#include <cm3l/Lib/Vector.h>
#include <cm3l/Lib/SLList.h>

#include <stdint.h> // IWYU pragma: keep
#include <string.h> // IWYU pragma: keep

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cm3l_IdMap
{
	cm3l_Vector objects;
	cm3l_Vector active;
	cm3l_Vector freeIds;
}
cm3l_IdMap;

static inline void cm3l_IdMapInit(cm3l_IdMap *self) {
	*self = (cm3l_IdMap) {
		.objects = cm3l_VectorCreate(),
		.active = cm3l_VectorCreate(),
		.freeIds = cm3l_VectorCreate()
	};
}

static inline size_t cm3l_IdMapNext(cm3l_IdMap *self, size_t index) {
	while (index != self->objects.length && !((uint8_t *)self->active.data)[index])
		++index;
	return index;
}

static inline size_t cm3l_IdMapBegin(cm3l_IdMap *self) {
	return cm3l_IdMapNext(self, 0);
}

size_t cm3l_IdMapInsert(cm3l_IdMap *self, const void *obj, const size_t objsize);

#define cm3l_IdMapInsertM(self, obj, type) (cm3l_IdMapInsert((self), (obj), sizeof(type)))

static inline void cm3l_IdMapErase(cm3l_IdMap *self, size_t index, const size_t objsize)
{
	*(uint8_t *)cm3l_VectorAt(&self->active, index, 1) = 0u;
	cm3l_VectorPushBack(&self->freeIds, &index, sizeof(size_t));
}

static inline void *cm3l_IdMapAt(cm3l_IdMap *self, size_t index, const size_t objsize)
{
	assert(*cm3l_VectorAtM(&self->active, index, uint8_t));
	return cm3l_VectorAt(&self->objects, index, objsize);
}

#define cm3l_IdMapAt(self, index, type) (cm3l_IdMapAt((self), (index), sizeof(type)))

// inline cm3l_IdMap cm3l_IdMap_reorder(cm3l_IdMap *self)

#ifdef __cplusplus
}
#endif

#endif
