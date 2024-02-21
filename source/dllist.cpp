#include <cm3l/Lib/DLList.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

int main(void)
{
	cm3l_DLList q = cm3l_DLListCreate();
	
	std::string resp;
	do {
		std::cout << "> ";
		std::cin >> resp;
		if (resp == "pb")
		{
			std::string r;
			std::cin >> r;
			cm3l_DLListPushBack(&q, r.c_str(), r.size() + 1);
		}
		else if (resp == "a" | resp == "all")
		{
			int c = 0;
			for (cm3l_DLNode *n = q.first; n != NULL; n = n->next)
			{
				printf("[%d]: %s\n", c, n->data);
				c++;
			}
		}
		else if (resp == "g" || resp == "get")
		{
			int ind;
			std::cin >> ind;
			cm3l_DLNode *n = cm3l_DLListGet(&q, ind);
			printf("%s\n", n->data);
		}
		else if (resp == "d" || resp == "del")
		{
			int ind;
			std::cin >> ind;
			cm3l_DLListDelete(&q, ind);
		}
		else if (resp == "i" || resp == "ins")
		{
			int ind;
			std::cin >> ind;
			std::string s;
			std::cin >> s;
			cm3l_DLListInsert(&q, ind, s.c_str(), s.size() + 1);
		}
		else if (resp == "h" || resp == "help")
		{
			std::cout <<
				"     pb: push back\n"
				"a | all: print all\n"
				"g | get: get by index\n"
				"d | del: delete element\n"
				"i | ins: insert\n";
		}
		else std::cout << "Unknown command\n";
	}
	while (resp != "q");
	cm3l_DLListDestroy(&q);
}
