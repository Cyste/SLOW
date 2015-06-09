# SLOW
Simple and fast math parser
### Math expression to Reverse Polish Notation
```cpp
#include "SLOW.h"

#include <stdio.h>

int main(int argc, char** argv)
{
	// Declare output
	SLOW_Word output[32];

	// Parse
	SLOW_ShuntingYard("12 + a * (b * c + d / e)", output);

	// Print 
	for (unsigned int i = 0; output[i].text[0]; ++i)
        printf("%s ", output[i].text);
	printf("\n");

	return 0;
}
```
### Math expression to Node hierarchy
```cpp
#include "SLOW.h"

#include <stdio.h>

void WriteNode(SLOW_Node* node)
{
	printf("%s\n", SLOW_NodeGetWord(node)->text);

	if (SLOW_NodeHasChildren(node))
	{
		WriteNode(SLOW_NodeGetChild(node, 0));
		WriteNode(SLOW_NodeGetChild(node, 1));
	}
}

int main(int argc, char** argv)
{
	SLOW_Node* node = SLOW_InitNode();

	SLOW_Parse("12 + a * (b * c + d / e)", node);

	WriteNode(node);

	SLOW_FreeNode(node);

	return 0;
}
```
