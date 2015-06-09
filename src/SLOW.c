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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

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

typedef float (*SLOW_OPERATOR)(float, float);

float SLOW_OperatorAdd(float a, float b)
{
	return a + b;
}

float SLOW_OperatorSub(float a, float b)
{
	return a - b;
}

float SLOW_OperatorMul(float a, float b)
{
	return a * b;
}

float SLOW_OperatorDiv(float a, float b)
{
	return a / b;
}

float SLOW_OperatorPow(float a, float b)
{
	return powf(a, b);
}

typedef struct SLOW_Operator
{
	const char* string;
	SLOW_OPERATOR eval;
} SLOW_Operator;

SLOW_Operator operators[] =
{
	{ "(", 0 },
	{ ")", 0 },
	{ "-", &SLOW_OperatorSub },
	{ "+", &SLOW_OperatorAdd },
	{ "*", &SLOW_OperatorMul },
	{ "/", &SLOW_OperatorDiv },
	{ "^", &SLOW_OperatorPow },
};

static
const SLOW_Operator* SLOW_GetOperator(const char* string)
{
	for (unsigned int i = 0; i < sizeof(operators) / sizeof(*operators); ++i)
	{
		int result = SLOW_TRUE;
		for (unsigned int j = 0; operators[i].string[j]; ++j)
		{
			if (!string[j])
			{
				result = SLOW_FALSE;
				break;
			}

			if (string[j] != operators[i].string[j])
			{
				result = SLOW_FALSE;
				break;
			}
		}
		if (result)
			return &operators[i];
	}
	return 0;
}

typedef struct SLOW_Node
{
	SLOW_Word word;
	struct SLOW_Node* children[2];
	int hasChildren;
} SLOW_Node;

static
int SLOW_IsNumber(const char* string)
{
	while(*string)
	{
		if (*string != '.' && !isdigit(*string))
			return SLOW_FALSE;

		string++;
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
	for (unsigned int i = 0; i < sizeof(operators) / sizeof(*operators); ++i)
	{
		int result = SLOW_TRUE;
		for (unsigned int j = 0; operators[i].string[j]; ++j)
		{
			if (!c[j])
			{
				 result = SLOW_FALSE;
				 break;
			}
			if (c[j] != operators[i].string[j])
			{
				result = SLOW_FALSE;
				break;
			}
		}
		if (result)
		{
			if (op) strcpy(op, operators[i].string);
			if (op_size) *op_size = strlen(operators[i].string);
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
		if (words[i].text[0] == '(') // special case
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
		else if (SLOW_IsOperator(words[i].text, 0, 0))
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
		else
		{
			strcpy((output++)->text, words[i].text);
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
		if (node->hasChildren)
		{
			SLOW_FreeNode(node->children[0]);
			SLOW_FreeNode(node->children[1]);
		}

		free(node);
	}
}

static
SLOW_Node* SLOW_InitNode(void)
{
	SLOW_Node* node = malloc(sizeof(SLOW_Node));
	node->children[0] = 0;
	node->children[1] = 0;
	node->hasChildren = SLOW_FALSE;
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
		node->hasChildren = SLOW_TRUE;
		node->children[0] = SLOW_InitNode();
		node->children[1] = SLOW_InitNode();

		unsigned int l = 0;
		SLOW_ParseNode(node->children[0], output + 1, &l);
		SLOW_ParseNode(node->children[1], output + 2 + l, &l);
		if (i) *i += l;
	}

	return SLOW_TRUE;
}

SLOW_Node* SLOW_Parse(const char* expression)
{
	SLOW_Node* node;
	SLOW_Word output[SLOW_OUTPUT_SIZE];
	unsigned int outputSize;

	if (!SLOW_ShuntingYard(expression, output))
		return 0;

	for (outputSize = 0; output[outputSize].text[0]; outputSize++) { }

	SLOW_SWAP_ARRAY(SLOW_Word, output, outputSize);

	node = SLOW_InitNode();
	SLOW_ParseNode(node, output, 0);

	return node;
}

int SLOW_NodeHasChildren(SLOW_Node* node)
{
	return node->hasChildren;
}

SLOW_Node* SLOW_NodeGetChild(SLOW_Node* node, unsigned char i)
{
	if (i < 2)
		return node->children[i];
	return 0;
}

const SLOW_Word* SLOW_NodeGetWord(SLOW_Node* node)
{
	return &node->word;
}

float SLOW_Eval(const char* expression)
{
	SLOW_Word output[SLOW_WORD_COUNT];
	float stack[SLOW_STACK_SIZE];
	float* s = stack;

	if (!SLOW_ShuntingYard(expression, output))
		return 0.0f;

	for (unsigned int i = 0; output[i].text[0]; ++i)
	{
		if (SLOW_IsNumber(output[i].text))
		{
			*s++ = (float)atof(output[i].text);
		}
		else // operator
		{
			float a = *(--s);
			float b = *(--s);
			
			SLOW_Operator* op = SLOW_GetOperator(output[i].text);
			if (op && op->eval)
				*s++ = op->eval(b, a);
		}
	}
	return stack[0];
}