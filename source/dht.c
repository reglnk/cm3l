#include <cm3l/Lib/DHTMap.h>
#include <stdio.h>
#include <string.h>

void printEntry(cm3l_MapEntry ent) {
	if (ent.key == NULL) {
		puts("<null>");
		return;
	}
	printf("[\"");
	for (int i = 0; i != ent.keysize; ++i)
		putchar(((const char *)ent.key)[i]);
	
	printf("\"]: %d <ks: %d, vs: %d>\n", ent.value ? *(int*)ent.value : 0, (int)ent.keysize, (int)ent.valsize);
}

int main(int argc, char **argv)
{
    cm3l_DHTMap m = cm3l_DHTMapCreate(cm3l_HashFuncS);
	cm3l_MapEntry ent;

	for (;;)
	{
		printf("> ");
		char cmds[256];
		scanf("%s", cmds);
		char cmd = cmds[0];

		if (cmd == 'i')
		{
			char key[256];
			int value;
			scanf("%s%d", key, &value);
			ent = cm3l_MapEntryCreate(key, &value, strlen(key), sizeof(int));
			cm3l_DHTMapInsert(&m, ent);
		}
		else if (cmd == 'r')
		{
			char key[256];
			scanf("%s", key);
			cm3l_MapEntry *f = cm3l_DHTMapAccess(&m, key, strlen(key));
			ent = f ? *f : (cm3l_MapEntry){ NULL, NULL, 0, 0 };
			printEntry(ent);
		}
		else if (cmd == 'a')
		{
			cm3l_DHTMapIterator iter = cm3l_DHTMapIteratorCreate(&m);
			for (;;)
			{
				cm3l_DHTMapNext(&iter);
				if (iter.node == NULL)
					break;
				
				cm3l_MapEntry *ent = cm3l_DHTMapIterAccess(&iter);
				printEntry(*ent);
			}
		}
		else if (cmd == 'b')
			break;
		else printf("unknown command\n");
	}
}
