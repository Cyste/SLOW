/*
	Copyright 2015 Mariusz Dzikowski

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "SLOW.h"

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string.h>
#include <ctype.h>

#define SLOW_STACK_SIZE  64
#define SLOW_OUTPUT_SIZE 64
#define SLOW_WORD_COUNT  64

#define SLOW_UNKNOWN_OPERATOR 256

#define SLOW_SWAP_ARRAY(type, array, size) \
{							               \
	for (unsigned int n = size; n > 1; n--)\
	{									   \
		for (unsigned int i = 0; i < n - 1; i++) \
		{								   \
			type temp = array[i + 1];      \
			array[i + 1] = array[i];       \
			array[i] = temp;               \
		}								   \
	}									   \
}

typedef struct SLOW_Node
{
	SLOW_Word word;
	struct SLOW_Node* children[2];
	int haveChildren;
} SLOW_Node;

static
int SLOW_IsNumber(const char* string)
{
	while(*string)
	{
		if (!isdigit(*string++))
			return SLOW_FALSE;
	} 
	return SLOW_TRUE;
}

static 
int SLOW_IsSeparator(char c)
{
	static const char* separators = " \t\r,";

	for (unsigned int i = 0; separators[i]; ++i)
		if (separators[i] == c)
			return SLOW_TRUE;

	return SLOW_FALSE;
}

static 
int SLOW_IsOperator(const char* c, char* op, unsigned int* op_size)
{
	static const char* operators[] = 
	{
		"(",
		")",
		"-",
		"+",
		"+",
		"*",
		"/",
		"^",
	};

	for (unsigned int i = 0; i < sizeof(operators) / sizeof(*operators); ++i)
	{
		int result = SLOW_TRUE;
		for (unsigned int j = 0; operators[i][j]; ++j)
		{
			if (!c[j])
			{
				 result = SLOW_FALSE;
				 break;
			}
			if (c[j] != operators[i][j])
			{
				result = SLOW_FALSE;
				break;
			}
		}
		if (result)
		{
			if (op) strcpy(op, operators[i]);
			if (op_size) *op_size = strlen(operators[i]);
			return SLOW_TRUE;
		}
	}

	return SLOW_FALSE;
}

static 
int SLOW_SplitString(const char* string, SLOW_Word* words)
{
	char op[SLOW_WORD_SIZE];
	unsigned int op_size = 0;
	unsigned int word = 0, n = 0;
	unsigned int lineSize = strlen(string);
	unsigned int i = 0;
	while (i < lineSize)
	{
		if (SLOW_IsSeparator(string[i]))
		{
			if (!SLOW_IsSeparator(string[i + 1]))
			{
				if (n > 0)
				{
					words[word].text[n] = 0;
					word++;
					n = 0;
				}
			}
		}
		else if (SLOW_IsOperator(&string[i], op, &op_size))
		{
			if (n > 0)
				words[word++].text[n] = 0;
			strcpy(words[word++].text, op);
			n = 0;
			i += op_size - 1;
		}
		else
		{
			words[word].text[n++] = string[i];
		}
		i++;
	}
	words[word++].text[n] = 0;
	words[word].text[0] = 0;

	return SLOW_TRUE;
}

static 
int SLOW_GetOperatorPrecedence(const char* op)
{
	if (strcmp(op, "(") == 0)
		return 0;
	else if (strcmp(op, ")") == 0)
		return 1;
	else if (strcmp(op, "+") == 0)
		return 1;
	else if (strcmp(op, "-") == 0)
		return 1;
	else if (strcmp(op, "/") == 0)
		return 2;
	else if (strcmp(op, "*") == 0)
		return 2;
	else if (strcmp(op, "^") == 0)
		return 3;

	// Unknown operator
	return SLOW_UNKNOWN_OPERATOR;
}

int SLOW_ShuntingYard(const char* expression, SLOW_Word* output)
{
	SLOW_Word stack[SLOW_STACK_SIZE];
	SLOW_Word words[SLOW_WORD_COUNT];

	SLOW_SplitString(expression, words);

	SLOW_Word* s = stack;

	for (unsigned int i = 0; words[i].text[0]; ++i)
	{
		if (SLOW_IsNumber(words[i].text))
		{
			strcpy((output++)->text, words[i].text);
		}
		else if (words[i].text[0] == '(') // special case
		{
			strcpy((s++)->text, words[i].text);
		}
		else if (words[i].text[0] == ')') // special case
		{
			while (s > stack && (--s)->text[0] != '(')
			{
				strcpy((output++)->text, (s)->text);
			}
			// '(' not found
			if (s == stack)
				return SLOW_FALSE;
		}
		else // operator
		{
			if (s == stack || SLOW_GetOperatorPrecedence(words[i].text) > SLOW_GetOperatorPrecedence(s->text))
			{
				strcpy((s++)->text, words[i].text);
			}
			else
			{
				int p = SLOW_GetOperatorPrecedence(words[i].text);
				while (s > stack && SLOW_GetOperatorPrecedence((s - 1)->text) >= p)
				{
					strcpy((output++)->text, (--s)->text);
				}
				strcpy((s++)->text, words[i].text);
			}
		}
	}

	while (s > stack)
	{
		strcpy((output++)->text, (--s)->text);
	}

	output->text[0] = 0;

	return SLOW_TRUE;
}

void SLOW_FreeNode(SLOW_Node* node)
{
	if (node)
	{
		if (node->haveChildren)
		{
			SLOW_FreeNode(node->children[0]);
			SLOW_FreeNode(node->children[1]);
		}

		free(node);
	}
}

SLOW_Node* SLOW_InitNode(void)
{
	SLOW_Node* node = malloc(sizeof(SLOW_Node));
	node->children[0] = 0;
	node->children[1] = 0;
	node->haveChildren = 0;
	node->word.text[0] = 0;
	return node;
}

static 
int SLOW_ParseNode(SLOW_Node* node, SLOW_Word* output, unsigned int* i)
{
	if (!output->text[0])
		return SLOW_FALSE;

	strcpy(node->word.text, output->text);

	if (SLOW_IsNumber(output->text))
	{

	}
	else if (SLOW_IsOperator(output->text, 0, 0))
	{
		if (i) *i += 2;
		node->haveChildren = SLOW_TRUE;
		node->children[0] = SLOW_InitNode();
		node->children[1] = SLOW_InitNode();

		unsigned int l = 0;
		SLOW_ParseNode(node->children[0], output + 1, &l);
		SLOW_ParseNode(node->children[1], output + 2 + l, &l);
		if (i) *i += l;
	}

	return SLOW_TRUE;
}

int SLOW_Parse(const char* expression, SLOW_Node* node)
{
	SLOW_Word output[SLOW_OUTPUT_SIZE];
	unsigned int outputSize;

	if (!SLOW_ShuntingYard(expression, output))
		return SLOW_FALSE;

	for (outputSize = 0; output[outputSize].text[0]; outputSize++) { }

	SLOW_SWAP_ARRAY(SLOW_Word, output, outputSize);

	SLOW_ParseNode(node, output, 0);

	return SLOW_TRUE;
}