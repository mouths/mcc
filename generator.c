#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "parse.h"

static void error(char *msg){
	if(msg == NULL)
		fprintf(stderr, "generator error\n");
	else
		fprintf(stderr, "generator error:%s\n", msg);
	exit(1);
}

static void print_num(Num *in){
	if(in == NULL)error(NULL);
	if(in->type == NUM){
		printf("push $%d\n", in->i);
		return;
	}else if(in->type == ADD || in->type == SUB){
		print_num(in->lhs);
		print_num(in->rhs);
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		printf("%s %%rbx, %%rax\n", in->type == ADD ? "add" : "sub");
		printf("push %%rax\n");
		return;
	}
	error(NULL);
}

void generator(Num *in){
	if(in == NULL){
		fprintf(stderr, "generator error\n");
		exit(1);
	}
	printf(".globl _main\n");
	printf("_main:\n");

	print_num(in);
	//printf("mov $%d, %%rax\n", in->i);

	printf("pop %%rax\n");
	printf("ret\n");
}
