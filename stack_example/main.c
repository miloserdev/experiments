#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// unpopable stack

#define STACK_SIZE 6
uint32_t stack[STACK_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };



void stack_print()
{
	for (size_t i = 0; i < STACK_SIZE; i++)
	{
		printf("%d -> 0x%08X \n", i, stack[i]);
	}
}



bool stack_push(uint32_t *dest, uint32_t *data)
{
//	printf("data is 0x%08X \n", data);
	size_t i = STACK_SIZE;
	if (i == 0) return false;
	do {
		stack[i] = (i == 0) ? data : stack[i - 1];
	} while( (i--) != 0);

/*	for (size_t i = STACK_SIZE; i--; i != 0)
	{
		printf("checking %d \n", i);
		stack[i] = (i == 0) ? data : stack[i - 1];

	}
*/
	return true;
}

bool stack_exists(uint32_t *stack, uint32_t *data)
{
	for (size_t i = 0; i < STACK_SIZE; i++)
	{
		if (stack[i] == data) return true;
	}
	return false;
}


int main ()
{
	printf("___ begin ___ \n");


	printf("___ before ___ \n");
	stack_print();


	for (size_t i = 0; i <= 0x00000FFF; i++)
{
	stack_push(stack, (uint32_t) i);
}
	printf("___ after ___ \n");
	stack_print();


	uint32_t data = 0xAEAAFFFF;
	bool exists = stack_exists(stack, data);
	printf("___ is 0x%08X %s exist ___ \n", data, (exists ? "is" : "does not") );

	data = 0xBBFAFF;
	exists = stack_exists(stack, data);
	printf("___ is 0x%08X %s exist ___ \n", data, (exists ? "is" : "does not") );
}
