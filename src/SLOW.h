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
#ifndef SLOW_H
#define SLOW_H

#define SLOW_VERSION 0x000100

#define SLOW_TRUE  1
#define SLOW_FALSE 0

#define SLOW_WORD_SIZE   64

#define SLOW_TOKEN_NUMBER   0
#define SLOW_TOKEN_OPERATOR 1

typedef struct SLOW_Word
{
	char text[SLOW_WORD_SIZE];
} SLOW_Word;

typedef struct SLOW_Node SLOW_Node;

#ifdef __cplusplus
extern "C" {
#endif

void SLOW_FreeNode(SLOW_Node* node);

int SLOW_ShuntingYard(const char* expression, SLOW_Word* output);

SLOW_Node* SLOW_Parse(const char* expression);

int SLOW_NodeHasChildren(SLOW_Node* node);

SLOW_Node* SLOW_NodeGetChild(SLOW_Node* node, unsigned char i);

const SLOW_Word* SLOW_NodeGetWord(SLOW_Node* node);

float SLOW_Eval(const char* expression);

#ifdef __cplusplus
}
#endif

#endif