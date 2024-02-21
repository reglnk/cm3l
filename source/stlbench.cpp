#include <cm3l/Lib/Hash.h>
#include <unordered_map>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	std::unordered_map<size_t, size_t> m;

	for (size_t i = 0; i != 500000; ++i)
	{
		size_t key = i;
		size_t value = i;
		m[key] = value;
	}

	if (argc > 1 && !strcmp(argv[1], "-p"))
	{
		printf("count: %zu\n", m.size());
	}
}
