#include <cm3l/Lib/DHTMap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	
    cm3l_DHTMap m = cm3l_DHTMapCreate(cm3l_HashFuncS);
	cm3l_MapEntry ent;

	for (unsigned i = 0; i != 1000000; ++i)
	{
		size_t key = cm3l_HashFuncS(&i, sizeof(i));
		size_t value = i;
		ent = cm3l_MapEntryCreate(&key, &value, sizeof(key), sizeof(value));
		cm3l_DHTMapInsert(&m, ent);
	}
	
	unsigned index = atoi(argv[1]);
	size_t result = 0;

	if (argc > 2)
	{
		for (unsigned i = 0; i != index; ++i)
		{
			size_t key = cm3l_HashFuncS(&i, sizeof(i));
			cm3l_MapEntry *f = cm3l_DHTMapAccess(&m, &key, sizeof(key));
			result ^= f ? *(size_t*)f->value : 0;
		}
	}
	else
	{
		size_t key = cm3l_HashFuncS(&index, sizeof(index));
		cm3l_MapEntry *f = cm3l_DHTMapAccess(&m, &key, sizeof(key));
		result = f ? *(size_t*)f->value : 0;
	}
	printf("%u\n", (unsigned)result);
}
