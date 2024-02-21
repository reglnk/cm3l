#ifndef CM3L_VECTOR_H
#define CM3L_VECTOR_H

#include <cm3l/Lib/Memory.h>

#include <stddef.h>
#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	void *data;
	size_t length;
	size_t capacity;
}
cm3l_Vector;

static inline cm3l_Vector cm3l_VectorCreate(void)
{
	cm3l_Vector vec = { .data = NULL, .length = 0u, .capacity = 0u };
	return vec;
}

void cm3l_VectorDestroy(cm3l_Vector *self);

void cm3l_VectorResize(cm3l_Vector *self, size_t newlen, const size_t objsize);

static void cm3l_VectorResizeExact(cm3l_Vector *self, size_t newlen, const size_t objsize)
{
	*self = (cm3l_Vector) {
		.data = cm3l_Realloc(self->data, newlen * objsize),
		.length = newlen,
		.capacity = newlen
	};
}

void cm3l_VectorPushBack(cm3l_Vector *self, const void *obj, const size_t objsize);

void cm3l_VectorPopBack(cm3l_Vector *self, const size_t objsize);

static inline void *cm3l_VectorAt(cm3l_Vector *self, size_t index, const size_t objsize) {
	return (uint8_t *)self->data + index * objsize;
}

static inline void *cm3l_VectorBack(cm3l_Vector *self, const size_t objsize) {
	assert(self->length);
	return cm3l_VectorAt(self, self->length - 1, objsize);
}

static inline void *cm3l_VectorPushBackR(cm3l_Vector *self, const void *obj, const size_t objsize)
{
	cm3l_VectorPushBack(self, obj, objsize);
	return cm3l_VectorBack(self, objsize);
}

#define cm3l_VectorResizeM(self, newlen, type) cm3l_VectorResize(self, newlen, sizeof(type))
#define cm3l_VectorResizeExactM(self, newlen, type) cm3l_VectorResizeExact(self, newlen, sizeof(type))
#define cm3l_VectorPushBackM(self, obj, type) cm3l_VectorPushBack(self, obj, sizeof(type))
#define cm3l_VectorPushBackA(self, obj) cm3l_VectorPushBack(self, obj, sizeof(*(obj)))
#define cm3l_VectorPushBackV(self, obj) cm3l_VectorPushBack(self, &(obj), sizeof(obj))
#define cm3l_VectorPopBackM(self, type) cm3l_VectorPopBack(self, sizeof(type))
// #define cm3l_VectorAtM(self, index, type) ((type *)((uint8_t *)((self)->data) + index * sizeof(type)))
#define cm3l_VectorAtM(self, index, type) (((type *)((self)->data)) + (index))
#define cm3l_VectorBackM(self, type) ((type *)cm3l_VectorBack(self, sizeof(type)))
#define cm3l_VectorPushBackRM(self, obj, type) ((type *)cm3l_VectorPushBackR(self, obj, sizeof(type)))

#ifdef __cplusplus
}
#endif

#endif
