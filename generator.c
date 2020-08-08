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

static void print_num(Num *in);

static void print_addr(Num *in){
	if(in->type == ID)
		printf("push $%d\n", *(in->name) - 'a');
	else
		print_num(in);
}

static void print_num(Num *in){
	if(in == NULL)error(NULL);
	if(in->type == NUM){
		printf("push $%d\n", in->i);
		return;
	}else if(
			in->type == ADD ||
			in->type == SUB ||
			in->type == MUL ||
			in->type == AND ||
			in->type == XOR ||
			in->type == OR
			){
		char *code;
		switch(in->type){
			case ADD:
				code = "add";
				break;
			case SUB:
				code = "sub";
				break;
			case MUL:
				code = "imul";
				break;
			case AND:
				code = "and";
				break;
			case XOR:
				code = "xor";
				break;
			case OR:
				code = "or";
			default:;
		}
		print_num(in->lhs);
		print_num(in->rhs);
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		printf("%s %%rbx, %%rax\n", code);
		printf("push %%rax\n");
		return;
	}else if(in->type == DIV || in->type == MOD){
		print_num(in->lhs);
		print_num(in->rhs);
		printf("mov $0, %%rdx\n");
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		printf("div %%rbx\n");
		printf("push %%r%cx\n", in->type == DIV ? 'a' : 'd');
		return;
	}else if(in->type == PLUS || in->type == MINUS){
		print_num(in->lhs);
		if(in->type == MINUS){
			printf("pop %%rbx\n");
			printf("mov $0, %%rax\n");
			printf("sub %%rbx, %%rax\n");
			printf("push %%rax\n");
		}
		return;
	}else if(
			in->type == LES ||
			in->type == GRT ||
			in->type == LEQ ||
			in->type == GEQ ||
			in->type == EQ ||
			in->type == NEQ
			){
		print_num(in->lhs);
		print_num(in->rhs);
		printf("pop %%rax\n");
		printf("pop %%rbx\n");
		switch(in->type){
			case LES:
			case LEQ:
				printf("cmp %%rax, %%rbx\n");
				break;
			case GRT:
			case GEQ:
			default:
				printf("cmp %%rbx, %%rax\n");
				break;
		}
		switch(in->type){
			case LES:
			case GRT:
				printf("setl %%al\n");
				break;
			case LEQ:
			case GEQ:
				printf("setle %%al\n");
				break;
			case EQ:
				printf("sete %%al\n");
				break;
			case NEQ:
				printf("setne %%al\n");
				break;
			default:;
		}
		printf("movzb %%al, %%rax\n");
		printf("push %%rax\n");
		return ;
	}else if(
			in->type == AS
			){
		print_addr(in->lhs);
		print_num(in->rhs);
		printf("pop %%rax\n");
		printf("pop %%rbx\n");
		printf("mov %%rax, (%%rbp, %%rbx, 4)\n");
		printf("push %%rax\n");
		return;
	}else if(
			in->type == ID
			){
		print_addr(in);
		printf("pop %%rbx\n");
		printf("mov (%%rbp, %%rbx, 4), %%rax\n");
		printf("push %%rax\n");
		return;
	}

	error(NULL);
}

static void print_stmt(Stmt *in){
	if(in->type == RET){
		if(in->Nchild){
			print_num(in->Nchild);
			printf("pop %%rax\n");
		}
		printf("mov %%rbp, %%rsp\n");
		printf("pop %%rbp\n");
		printf("ret\n");
		return;
	}else if(in->type == EXP){
		print_num(in->Nchild);
		printf("pop %%rax\n");
		return;
	}else if(in->type == CPD){
		if(in->rhs)print_stmt(in->rhs);
		return;
	}else if(in->type == ITEM){
		print_stmt(in->rhs);
		if(in->lhs)print_stmt(in->lhs);
		return;
	}
	error("print_statement");
}

void generator(void *in){
	if(in == NULL){
		fprintf(stderr, "generator error\n");
		exit(1);
	}
	printf(".globl _main\n");
	printf("_main:\n");
	printf("push %%rbp\n");
	printf("mov %%rsp, %%rbp\n");
	printf("sub $%d, %%rsp\n", 26 * 4);

	print_stmt(in);
}
