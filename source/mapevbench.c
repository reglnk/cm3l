#include <cm3l/Lib/HashMapEv.h>
#include <stdio.h>
#include <string.h> // IWYU pragma: keep
#include <stdlib.h>

int main(int argc, char **argv)
{
    cm3l_HashMap m = cm3l_HashMapCreate(cm3l_HashFuncSsize);

	for (size_t i = 0; i != 1000000; ++i)
	{
		size_t key = i;
		size_t value = i;
		cm3l_HashMapEvInsertKV(&m, &key, sizeof(key), &value, sizeof(value));
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
	cm3l_HashMapEvDestroy(&m);
}
