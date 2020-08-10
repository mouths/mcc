#include <stdio.h>
#include <stdlib.h>

int foo(){
	printf("test\n");
	return 0;
}

void *get_pointer(){
	return malloc(sizeof(int));
}

void print_nums(int a1, int a2, int a3, int a4, int a5, int a6){
	printf("%d, %d, %d, %d, %d, %d\n", a1, a2, a3, a4, a5, a6);
}
