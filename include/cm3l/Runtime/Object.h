#ifndef CM3L_OBJECT_H
#define CM3L_OBJECT_H

#include <cm3l/Lexer.h>

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The object can be used as type of object, if it's "type object" */

typedef enum cm3l_ObjectType
{
	/* compact fundamental types */
	Cft_Undefined = 0u,
	Cft_Type,
	Cft_Bool,
	Cft_Uint8,
	Cft_Uint16,
	Cft_Uint32,
	Cft_Uint64,
	Cft_Int8,
	Cft_Int16,
	Cft_Int32,
	Cft_Int64,
	Cft_Float32,
	Cft_Float64,
	
	/* allocatable fundamental types */
	Aft_String,
	Aft_Object, /* custom type defined in runtime */
	Aft_Reference, /* an object that points to an other allocated object */
	
	/* composite types */
	/* (at least 2 objects required to describe the type completely) */
	Cmt_RawPointer,
	Cmt_Vector,
	Cmt_Table
}
cm3l_ObjectType;

typedef struct cm3l_Object cm3l_Object;

struct cm3l_Runtime;

typedef void (*cm3l_ObjectFn_Create)(struct cm3l_Runtime *ctx, cm3l_Object *obj);
typedef void (*cm3l_ObjectFn_Destroy)(struct cm3l_Runtime *ctx, cm3l_Object *obj);
typedef void (*cm3l_ObjectFn_Copy)(struct cm3l_Runtime *ctx, cm3l_Object *dest, cm3l_Object const *src);
typedef void (*cm3l_ObjectFn_Move)(struct cm3l_Runtime *ctx, cm3l_Object *dest, cm3l_Object *src);

// += -= *= /= ||= &&=
typedef void (*cm3l_ObjectFn_BinOperA)(struct cm3l_Runtime *ctx, cm3l_Object *, cm3l_Object const *);

// + - * / || && . ,
typedef void (*cm3l_ObjectFn_BinOperV)(struct cm3l_Runtime *ctx, cm3l_Object const *, cm3l_Object const *);

// this structure can describe all fundamental types
typedef struct cm3l_TypeObject
{
	cm3l_ObjectFn_Create constructor;
	cm3l_ObjectFn_Destroy destructor;
	cm3l_ObjectFn_Copy copy_cons;
	cm3l_ObjectFn_Move move_cons;
	
	cm3l_ObjectFn_BinOperA assign_opers[CM3L_NUM_BINARY_OPERATORS_A];
	cm3l_ObjectFn_BinOperV value_opers[CM3L_NUM_BINARY_OPERATORS_V];
}
cm3l_TypeObject;

struct cm3l_Object
{
	// type is described with type reference only
	uint32_t refcount;
	size_t type_ref; // (ID - 1) of type object, if not 0 (otherwise it's type object itself)
	
	union {
		void *pv;
		uint64_t u64;
		uint32_t u32;
		uint16_t u16;
		uint8_t u8;
		int64_t i64;
		int32_t i32;
		int16_t i16;
		int8_t i8;
		float f32;
		double f64;
		size_t sz;
	} data;
};

#ifdef __cplusplus
}
#endif

#endif
