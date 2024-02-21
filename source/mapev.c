#include <cm3l/Lib/HashMapEv.h>
#include <stdio.h>
#include <string.h>

void printEntry(cm3l_MapEntryEv const *ent) {
	if (!ent) {
		puts("<null>");
		return;
	}
		
	const void *keyaddr = CM3L_KEYADDR_SELECT(ent->key);
	if (keyaddr == NULL) {
		puts("<null>");
		return;
	}
	printf("[\"");
	for (int i = 0; i != ent->key.size; ++i)
		putchar(((const char *)keyaddr)[i]);
	
	printf("\"]: %d <ks: %d>\n", *(int*)ent->value, (int)ent->key.size);
}

int main(int argc, char **argv)
{
    cm3l_HashMap m = cm3l_HashMapCreate(cm3l_HashFuncS);
	
	int echo = 1;
	FILE *fp = stdin;
	FILE *fr = fp;
	unsigned aerr = 0;
	
	if (argc > 1)
	{
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "failed to open %s\n", argv[1]);
			return 1;
		}
	}

	for (;;)
	{
		if (echo && fp == stdin) printf("> ");
		char cmds[256];
		strcpy(cmds, "quit");
		fscanf(fp, "%s", cmds);

		if (!strcmp(cmds, "i") || !strcmp(cmds, "ins"))
		{
			char key[256];
			int value;
			fscanf(fp, "%s%d", key, &value);
			if (echo == 2) printf("%s %s %d\n", cmds, key, value);
			cm3l_HashMapEvInsertKV(&m, key, strlen(key), &value, sizeof(int));
		}
		else if (!strcmp(cmds, "g") || !strcmp(cmds, "get"))
		{
			char key[256];
			fscanf(fp, "%s", key);
			if (echo == 2) printf("%s %s\n", cmds, key);
			cm3l_MapEntryEv *f = cm3l_HashMapEvAccessK(&m, key, strlen(key));
			printEntry(f);
		}
		else if (!strcmp(cmds, "e") || !strcmp(cmds, "erase"))
		{
			char key[256];
			fscanf(fp, "%s", key);
			if (echo == 2) printf("%s %s\n", cmds, key);
			int erased = cm3l_HashMapEvEraseK(&m, key, strlen(key));
			if (echo) printf("[erased: %d entries]\n", erased);
		}
		else if (!strcmp(cmds, "a") || !strcmp(cmds, "all"))
		{
			if (echo == 2) printf("%s\n", cmds);
			cm3l_HashMapIterator iter = cm3l_HashMapIteratorCreate(&m);
			for (;;)
			{
				cm3l_HashMapNext(&iter);
				if (iter.node == NULL)
					break;
				
				cm3l_MapEntryEv *ent = cm3l_HashMapEvIterAccess(&iter);
				printEntry(ent);
			}
		}
		else if (!strcmp(cmds, "echo"))
		{
			char variant[256];
			fscanf(fp, "%s", variant);
			if (!strcmp(variant, "on"))
				echo = 2;
			else if (!strcmp(variant, "default"))
				echo = 1;
			else if (!strcmp(variant, "off"))
				echo = 0;
			else puts(variant);
		}
		else if (!strcmp(cmds, "switch"))
		{
			if (echo == 2) printf("%s\n", cmds);
			FILE *sw = fp;
			fp = fr;
			fr = sw;
		}
		else if (!strcmp(cmds, "as") || !strcmp(cmds, "assert"))
		{
			if (echo == 2) printf("%s\n", cmds);
			char key[256];
			char value[64];
			fscanf(fp, "%s%s", key, value);
			if (echo == 2) printf("%s %s %s\n", cmds, key, value);
			
			cm3l_MapEntryEv *f = cm3l_HashMapEvAccessK(&m, key, strlen(key));
			
			int notNull = strcmp(value, "null");
			int cval = notNull ? atoi(value) : 0;
			int fval = f ? *(int*)f->value : 0;
			
			if (notNull)
			{
				++aerr;
				if (f == NULL)
					printf("[assertion failed: %d != null]\n", cval);
				else if (cval != fval)
					printf("[assertion failed: %d != %d]\n", cval, fval);
				else --aerr;
			}
			else if (f != NULL)
			{
				++aerr;
				printf("[assertion failed: null != %d]\n", fval);
			}
		}
		else if (!strcmp(cmds, "q") || !strcmp(cmds, "quit")) {
			if (echo == 2) printf("%s\n", cmds);
			if (echo) puts("[exit]");
			break;
		}
		else puts("[unknown command]");
	}
	
	cm3l_HashMapDestroy(&m);
	return aerr ? 1 : 0;
}
