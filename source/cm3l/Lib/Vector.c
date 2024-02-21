#include <cm3l/Lib/Vector.h>
#include <cm3l/Lib/Memory.h>

#include <malloc.h>
#include <string.h>
#include <stdint.h>

static inline size_t growFn(size_t current) {
	// nearly current * 1.625
	return current + (current >> 1) + (current >> 3) + 1;
}

static inline size_t shrinkFn(size_t current) {
	// nearly current * 0.625
	return current - ((current >> 2) + (current >> 3));
}

void cm3l_VectorDestroy(cm3l_Vector *self)
{
	if (self->data)
		free(self->data);
	self->length = 0;
	self->capacity = 0;
}

void cm3l_VectorResize(cm3l_Vector *self, size_t newlen, const size_t objsize)
{
	if (self->capacity >= newlen) {
		self->length = newlen;
		return;
	}

	size_t newCap = growFn(self->capacity);
	while (newCap < newlen)
		newCap = growFn(newCap);

	*self = (cm3l_Vector) {
		.data = cm3l_Realloc(self->data, newCap * objsize),
		.length = newlen,
		.capacity = newCap
	};
}

void cm3l_VectorPushBack(cm3l_Vector *self, const void *obj, const size_t objsize)
{
	cm3l_VectorResize(self, self->length + 1, objsize);
	memcpy((uint8_t *)self->data + (self->length - 1) * objsize, obj, objsize);
}

void cm3l_VectorPopBack(cm3l_Vector *self, const size_t objsize)
{
	cm3l_VectorResize(self, self->length - 1, objsize);
}
