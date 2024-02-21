#include <cm3l/Lexer.h>

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int isWhitespace(char ch)
{
	switch (ch)
	{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case '\v':
		case '\f':
			return 1;
		default:
			return 0;
	}
}

static int isOperChar(char ch)
{
	switch (ch)
	{
		case '=':
		case '!':
		case '*':
		case '/':
		case '+':
		case '-':
		case '&':
		case '^':
		case '~':
		case '%':
		case '.':
		case ',':
		case ':':
			return 1;
		default:
			return 0;
	}
}

static int isControlChar(char ch)
{
	switch (ch)
	{
		case '(':
		case ')':
		case '{':
		case '}':
		case '[':
		case ']':
		case ';':
		case ':':
			return 1;
		default:
			return 0;
	}
}

static int isDigit(char ch)
{
	return ch >= '0' && ch <= '9';
}

static int isLetter(char ch)
{
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

typedef enum
{
	IsUndefined   = 0x0,
	IsWhitespace  = 0x1,
	IsOperChar    = 0x2,
	IsControlChar = 0x4,
	IsDigit       = 0x8,
	IsLetter      = 0x10
}
SymbolState;

static SymbolState getSymbolState(char ch)
{
	SymbolState flags = IsUndefined;
	flags |= isWhitespace(ch) * IsWhitespace;
	if (!flags)
	{
		flags |= isOperChar(ch) * IsOperChar;
		flags |= isControlChar(ch) * IsControlChar;
		flags |= isDigit(ch) * IsDigit;
		flags |= isLetter(ch) * IsLetter;
	}
	return flags;
}

// compares null-terminated string with another that may be not null-terminated,
// up to maxlen symbols. if cstr has more symbols, strings aren't considered equal.
static int strCompare(const char *cstr, const char *s, unsigned maxlen)
{
	for (unsigned i = 0; i != maxlen; ++i)
		if (!cstr[i] || cstr[i] != s[i])
			return 0;

	return !cstr[maxlen];
}

// yet one function to determine if cstr is s or part of s. returns end position if true
static unsigned strPartCompare(const char *cstr, const char *s, unsigned maxlen)
{
	for (unsigned i = 0; i != maxlen; ++i)
	{
		if (!cstr[i])
			return i;

		if (cstr[i] != s[i])
			return 0;
	}
	return cstr[maxlen] ? 0 : maxlen;
}

static const char *s_controls[CM3L_NUM_CONTROLS] =
{
	"return",
	"{",
	"}",
	"(",
	")",
	"[",
	"]",
	"if",
	"else",
	"break",
	"continue",
	"for",
	";",
	"require",
	"function"
};

static cm3l_TokenData s_controlCodes[CM3L_NUM_CONTROLS] =
{
	Tdt_Control_Return,
	Tdt_Control_ScopeBeg,
	Tdt_Control_ScopeEnd,
	Tdt_Control_ParBeg,
	Tdt_Control_ParEnd,
	Tdt_Control_If,
	Tdt_Control_Else,
	Tdt_Control_Break,
	Tdt_Control_Continue,
	Tdt_Control_For,
	Tdt_Control_Semicolon,
	Tdt_Control_Require,
	Tdt_Control_Function
};

static const char *s_operators[CM3L_NUM_OPERATORS] = {
	"=", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "^=",
	"==", "!=", ">", ">=", "<", "<=",
	"+", "-", "*", "/", "%", "<<", ">>", "^",
	".", ":", ",",
	"::"
};

static cm3l_TokenData s_opCodes[CM3L_NUM_OPERATORS] =
{
	Tdt_Oper_Assign,
	Tdt_Oper_AddA,
	Tdt_Oper_SubA,
	Tdt_Oper_MulA,
	Tdt_Oper_DivA,
	Tdt_Oper_RemA,
	Tdt_Oper_ShlA,
	Tdt_Oper_ShrA,
	Tdt_Oper_XorA,

	Tdt_Oper_IsEqual,
	Tdt_Oper_IsNEqual,
	Tdt_Oper_IsGreater,
	Tdt_Oper_IsGreaterEqual,
	Tdt_Oper_IsLess,
	Tdt_Oper_IsLessEqual,

	Tdt_Oper_Add,
	Tdt_Oper_Sub,
	Tdt_Oper_Mul,
	Tdt_Oper_Div,
	Tdt_Oper_Rem,
	Tdt_Oper_Shl,
	Tdt_Oper_Shr,
	Tdt_Oper_Xor,

	Tdt_Oper_Dot,
	Tdt_Oper_Colon,
	Tdt_Oper_Comma,

	Tdt_Oper_ScopeRes
};

const char *cm3l_OperString(cm3l_TokenData data)
{
	switch (data)
	{
		case Tdt_Oper_Assign:         return s_operators[0];
		case Tdt_Oper_AddA:           return s_operators[1];
		case Tdt_Oper_SubA:           return s_operators[2];
		case Tdt_Oper_MulA:           return s_operators[3];
		case Tdt_Oper_DivA:           return s_operators[4];
		case Tdt_Oper_RemA:           return s_operators[5];
		case Tdt_Oper_ShlA:           return s_operators[6];
		case Tdt_Oper_ShrA:           return s_operators[7];
		case Tdt_Oper_XorA:           return s_operators[8];

		case Tdt_Oper_IsEqual:        return s_operators[9];
		case Tdt_Oper_IsNEqual:       return s_operators[10];
		case Tdt_Oper_IsGreater:      return s_operators[11];
		case Tdt_Oper_IsGreaterEqual: return s_operators[12];
		case Tdt_Oper_IsLess:         return s_operators[13];
		case Tdt_Oper_IsLessEqual:    return s_operators[14];

		case Tdt_Oper_Add:            return s_operators[15];
		case Tdt_Oper_Sub:            return s_operators[16];
		case Tdt_Oper_Mul:            return s_operators[17];
		case Tdt_Oper_Div:            return s_operators[18];
		case Tdt_Oper_Rem:            return s_operators[19];
		case Tdt_Oper_Shl:            return s_operators[20];
		case Tdt_Oper_Shr:            return s_operators[21];
		case Tdt_Oper_Xor:            return s_operators[22];

		case Tdt_Oper_Dot:            return s_operators[23];
		case Tdt_Oper_Colon:          return s_operators[24];
		case Tdt_Oper_Comma:          return s_operators[25];
		case Tdt_Oper_ScopeRes:       return s_operators[26];
		default:
			return NULL;
	}
}

const char *cm3l_ControlString(cm3l_TokenData data)
{
	switch (data)
	{
		case Tdt_Control_Return:    return s_controls[0];
		case Tdt_Control_ScopeBeg:  return s_controls[1];
		case Tdt_Control_ScopeEnd:  return s_controls[2];
		case Tdt_Control_ParBeg:    return s_controls[3];
		case Tdt_Control_ParEnd:    return s_controls[4];
		case Tdt_Control_SbrkOpen:  return s_controls[5];
		case Tdt_Control_SbrkClose: return s_controls[6];
		case Tdt_Control_If:        return s_controls[7];
		case Tdt_Control_Else:      return s_controls[8];
		case Tdt_Control_Break:     return s_controls[9];
		case Tdt_Control_Continue:  return s_controls[10];
		case Tdt_Control_For:       return s_controls[11];
		case Tdt_Control_Semicolon: return s_controls[12];
		case Tdt_Control_Require:   return s_controls[13];
		case Tdt_Control_Function:  return s_controls[14];
		default:
			return NULL;
	}
}

typedef struct
{
	// current line number
	size_t line;

	// address of first symbol in current line
	const char *linebeg;

	// currently parsed token
	cm3l_Token token;
	// const char *tokenbeg;
	// const char *tokenend;

	// fileend - filebeg == filesize
	const char *filebeg;
	const char *fileend;
	const char *filename;

	int errors;
}
lexerFlowData;

/* @deprecated
cm3l_Token cm3l_RecognizeToken(const char *beg, const char *end)
{
	assert(end - beg > 0);
	cm3l_Token token = { Ttp_Undefined, Tdt_Undefined, 0, beg, end };

	if (end - beg >= 4)
	{
		if (beg[0] == '<' && beg[1] == '"' && end[-2] == '"' && end[-1] == '>')
		{
			token.type = Ttp_Literal;
			token.data = Tdt_Literal_String;
			return token;
		}
	}
	if (isDigit(beg[0]))
	{
		token.type = Ttp_Literal;
		int has_point = 0;
		for (const char *s = beg + 1; s != end; ++s)
		{
			if (!isDigit(*s))
			{
				if (*s == '.' && !has_point)
					has_point = 1;
				else return token;
			}
		}
		if (has_point)
			token.data = Tdt_Literal_Float;
		else token.data = Tdt_Literal_Int;

		return token;
	}
	if (isOperChar(beg[0]))
	{
		token.type = Ttp_Oper;
		for (int op = 0; op < CM3L_NUM_OPERATORS; ++op)
		{
			if (!strncmp(beg, s_operators[op], end - beg))
			{
				token.data = s_opCodes[op];
				return token;
			}
		}
		return token;
	}

	for (int i = 0; i != CM3L_NUM_CONTROLS; ++i)
	{
		if (!strncmp(beg, s_controls[i], end - beg))
		{
			token.type = Ttp_Control;
			token.data = s_controlCodes[i];
			return token;
		}
	}

	if (!isLetter(*beg))
		return token;

	for (const char *s = beg + 1; s != end; ++s)
		if (!isLetter(*s) && !isDigit(*s))
			return token;

	token.type = Ttp_Var;
	// token.data to be determined later
	return token;
}
*/

static const char *skipWhitespace(lexerFlowData *flow, const char *s)
{
	if (*s == '\n')
	{
		++s;
		if (s != flow->fileend && *s == '\r')
			++s;

		++flow->line;
		flow->linebeg = s;
		return s;
	}
	if (*s == '\r')
	{
		++s;
		if (s != flow->fileend && *s == '\n')
			++s;

		++flow->line;
		flow->linebeg = s;
		return s;
	}
	return s + 1;
}

static int findNextToken (lexerFlowData *flow)
{
	flow->token.type = Ttp_Undefined;
	flow->token.data = Tdt_Undefined;

	const char *s = flow->token.beg /* +1 */;

	// dont forget about line continuation inside tokens
	// (like string <" "> tokens)
	while (s != flow->token.end)
		s = skipWhitespace(flow, s);

	while (s != flow->fileend && isWhitespace(*s))
		s = skipWhitespace(flow, s);

	flow->token.beg = flow->token.end = s;
	if (s == flow->fileend)
		return 0;

	SymbolState st = getSymbolState(*s);

	// guessing which literal type it is
	if (st & IsDigit)
		flow->token.type |= Ttp_Literal;
	if (st & IsOperChar)
		flow->token.type |= Ttp_Oper | Ttp_Literal;
	if (st & IsControlChar)
		flow->token.type |= Ttp_Control;
	if (st & IsLetter)
		flow->token.type |= Ttp_Control | Ttp_Var;

	// don't report errors if the token type is undefined
	// it's job of parseToken
	return 1;
}

// this function does 2 things:
// 1. finding token end
// 2. recognizing the token
static int parseToken (lexerFlowData *flow)
{
	assert(flow->token.beg < flow->fileend);
	const char *const beg = flow->token.beg;
	flow->token.line = flow->line;

	if (flow->token.type & Ttp_Literal)
	{
		// guessing string literal
		if (flow->fileend - beg >= 2)
		{
			if (beg[0] == '<' && beg[1] == '"')
			{
				const char *s;
				for (s = beg + 2; s != flow->fileend - 1; ++s)
					if (s[0] == '"' && s[1] == '>')
						break;

				if (s != flow->fileend)
					s += 2;
				else
				{
					++flow->errors;
					printf (
						"%s:%zu:%zu: error: \"> expected\n",
						flow->filename, flow->line,
						s - flow->linebeg
					);
					return 0;
				}

				flow->token.type = Ttp_Literal;
				flow->token.data = Tdt_Literal_String;
				flow->token.end = s;
				flow->token.symbol = beg - flow->linebeg;
				return 1;
			}
		}

		// @todo improve to support +7.8e34f, -2.27e-12, -7898887u and so on
		int has_point = 0;
		const char *s;
		for (s = beg; s != flow->fileend; ++s)
		{
			if (!isDigit(*s))
			{
				if (*s == '.' && !has_point)
					has_point = 1;
				else break;
			}
		}
		if (s != beg)
		{
			flow->token.type = Ttp_Literal;
			if (has_point)
				flow->token.data = Tdt_Literal_Float;
			else flow->token.data = Tdt_Literal_Int;
			flow->token.end = s;
			flow->token.symbol = beg - flow->linebeg;
			return 1;
		}
	}

	if (flow->token.type & Ttp_Oper)
	{
		const char *s;
		for (s = beg + 1; s != flow->fileend; ++s)
			if (!isOperChar(*s))
				break;

		for (int i = 0; i != CM3L_NUM_OPERATORS; ++i)
		{
			if (strCompare(s_operators[i], beg, s - beg))
			{
				flow->token.type = Ttp_Oper;
				flow->token.data = s_opCodes[i];
				flow->token.end = s;
				flow->token.symbol = beg - flow->linebeg;
				return 1;
			}
		}
	}

	if (flow->token.type & Ttp_Control)
	{
		const char *s = beg;
		SymbolState st = getSymbolState(*s);

		for (++s; s != flow->fileend; ++s)
			if (getSymbolState(*s) != st)
				break;

		if (st & IsControlChar)
		{
			for (int i = 0; i != CM3L_NUM_CONTROLS; ++i)
			{
				unsigned ctlend;
				if (( ctlend = strPartCompare(s_controls[i], beg, s - beg) ))
				{
					flow->token.type = Ttp_Control;
					flow->token.data = s_controlCodes[i];
					flow->token.end = beg + ctlend;
					flow->token.symbol = beg - flow->linebeg;
					return 1;
				}
			}
		}
		else
		{
			for (int i = 0; i != CM3L_NUM_CONTROLS; ++i)
			{
				if (strCompare(s_controls[i], beg, s - beg))
				{
					flow->token.type = Ttp_Control;
					flow->token.data = s_controlCodes[i];
					flow->token.end = s;
					flow->token.symbol = beg - flow->linebeg;
					return 1;
				}
			}
		}
	}

	if (flow->token.type & Ttp_Var)
	{
		assert(isLetter(*beg));
		const char *s;
		for (s = beg + 1; s != flow->fileend; ++s)
			if (!isLetter(*s) && !isDigit(*s))
				break;

		flow->token.type = Ttp_Var;
		flow->token.end = s;
		flow->token.symbol = beg - flow->linebeg;
		return 1;
	}

	++flow->errors;
	++flow->token.end;
	printf (
		"%s:%zu:%zu: error: unknown token\n",
		flow->filename, flow->line,
		beg - flow->linebeg
	);
	return 0;
}

int cm3l_LexerProcess(cm3l_LexerData *lex, const char *filename)
{
	lexerFlowData flow;
	flow.filename = filename;

	FILE *fin;

	if (lex->buffer == NULL)
	{
		fin = fopen(filename, "rb");
		if (fin == NULL)
			return CM3L_ERROR;

		if (fseek(fin, 0, SEEK_END) != 0)
			goto fexit;

		int64_t fsize = (int64_t)ftell(fin);
		if (fsize == 0)
			goto fexit;

		if (fseek(fin, 0, SEEK_SET) != 0)
			goto fexit;

		char *buffer = malloc(fsize);
		if (buffer == NULL)
			goto fexit;

		int rd = fread(buffer, 1, fsize, fin);
		if (rd != fsize)
			goto fexit;

		lex->buffer = buffer;
		lex->bufsize = fsize;
	}

	flow.errors = 0;
	flow.token.beg = flow.token.end = lex->buffer;
	flow.fileend = lex->buffer + lex->bufsize;
	flow.line = 1;
	lex->filename = strdup(filename);

	for (;;)
	{
		if (!findNextToken(&flow))
			break;

		if (parseToken(&flow))
			cm3l_VectorPushBackM(&lex->tokens, &flow.token, cm3l_Token);
	}
	return (lex->syntaxErrors = flow.errors) ? CM3L_ERROR : CM3L_OK;

fexit:
	fclose(fin);
	return CM3L_ERROR;
}
