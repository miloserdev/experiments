#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// unpopable stack

#define STACK_SIZE 6
uint16_t stack[STACK_SIZE] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x99 };



void stack_print()
{
	for (size_t i = 0; i < STACK_SIZE; i++)
	{
		printf("%d -> 0x%04X \n", i, stack[i]);
	}
}



bool stack_push(uint16_t *dest, uint16_t *data)
{
//	printf("data is 0x%04X \n", data);
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

bool stack_exists(uint16_t *stack, uint16_t *data)
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

	stack_push(stack, (uint16_t) 0xAA);
	stack_push(stack, (uint16_t) 0xBB);
	stack_push(stack, (uint16_t) 0xCC);

	printf("___ after ___ \n");
	stack_print();


	uint16_t data = 0xCE;
	bool exists = stack_exists(stack, data);
	printf("___ is 0x%04X %s exist ___ \n", data, (exists ? "is" : "does not") );

	data = 0xCC;
	exists = stack_exists(stack, data);
	printf("___ is 0x%04X %s exist ___ \n", data, (exists ? "is" : "does not") );
}
