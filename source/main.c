#include <cm3l/Lexer.h>
#include <cm3l/Composer.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc < 2)
		return 1;
	
	cm3l_LexerData data = cm3l_LexerDataCreate();
	int res = cm3l_LexerProcess(&data, argv[1]);
	if (res) {
		printf("Failed to process %s\n", argv[1]);
		return 1;
	}
	printf("Syntax errors: %d\n", data.syntaxErrors);
	
	cm3l_Token *tokens = (cm3l_Token *)data.tokens.data;
	for (int i = 0; i < data.tokens.length; ++i)
	{
		printf("token type: %d, ", (int)tokens[i].type);
		if (tokens[i].type == Ttp_Oper)
			printf("data: %s\n", cm3l_OperString(tokens[i].data));
		else if (tokens[i].type == Ttp_Control)
			printf("data: %s\n", cm3l_ControlString(tokens[i].data));
		else printf("data: %d\n", (int)tokens[i].data);
	}
	
	cm3l_ComposerData cm = cm3l_ComposerDataCreate();
	unsigned cmErrors = cm3l_Compose(&data, &cm);
	printf("Compose errors: %u\n", cmErrors);
}
