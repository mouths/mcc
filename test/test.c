#include <stdio.h>
#include <stdlib.h>

int foo(){
	printf("test\n");
	return 0;
}

void *get_pointer(){
	return malloc(sizeof(int));
}
