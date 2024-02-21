#include <cm3l/Lib/Hash.h>

#include <stdint.h> // IWYU pragma: keep
#include <stddef.h> // IWYU pragma: keep

#define hash_num(n, type) ((((n) + (type)0x7acf0ebd8e1b4ce7llu) * (type)1283u) ^ (type)0xbf63e91dc71c4acdllu)

static inline size_t hashnum(size_t n) {
	return n;
}

size_t cm3l_HashFuncS(const void *mem, size_t size)
{
	size_t hash = (size_t)0x18a6e4b978ca0e2dllu;

	const size_t full = size / sizeof(size_t);
	const size_t *f_iter = mem;
	for (size_t i = 0; i != full; ++i) {
		hash ^= hash_num(f_iter[i], size_t);
	}
	const uint8_t *c_iter = mem;
	for (size_t r = full * sizeof(size_t); r != size; ++r) {
		hash ^= hash_num(c_iter[r], size_t);
	}
	return hash;
}

size_t cm3l_HashFuncS64(const void *mem, size_t size) {
	return hash_num(*(uint64_t*)mem, uint64_t);
}

size_t cm3l_HashFuncS32(const void *mem, size_t size) {
	return hash_num(*(uint32_t*)mem, uint32_t);
}

size_t cm3l_HashFuncS16(const void *mem, size_t size) {
	return hash_num(*(uint16_t*)mem, uint16_t);
}

size_t cm3l_HashFuncS8(const void *mem, size_t size) {
	return hash_num(*(uint8_t*)mem, uint8_t);
}
