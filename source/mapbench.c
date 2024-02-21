#include <cm3l/Lib/HashMap.h>
#include <stdio.h>
#include <string.h> // IWYU pragma: keep
#include <stdlib.h>

int main(int argc, char **argv)
{
    cm3l_HashMap m = cm3l_HashMapCreate(cm3l_HashFuncSsize);
	cm3l_MapEntry ent;

	for (unsigned i = 0; i != 1000000; ++i)
	{
		size_t key = i;//cm3l_HashFuncS(&i, sizeof(i));
		size_t value = i;
		ent = cm3l_MapEntryCreateK(&key, sizeof(key), &value, sizeof(value));
		cm3l_HashMapInsert(&m, ent);
	}

	if (argc > 1 && !strcmp(argv[1], "-p"))
	{
		size_t col = cm3l_HashMapCountCollisions(&m);
		printf (
			"collisions: %zu\n"
			"collision rate: %f\n"
			"count: %zu\n"
			"buckets: %zu\n"
			"fill rate: %f\n",
			col,
			(float)col / m.data.length,
			m.count,
			m.data.length,
			(float)m.count / m.data.length
		);
	}
	cm3l_HashMapDestroy(&m);
}
