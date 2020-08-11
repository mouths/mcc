#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "parse.h"

#define LHS 0
#define RHS 1
#define CENTER 2

static void error(char *msg){
	if(msg == NULL)
		fprintf(stderr, "generator error\n");
	else
		fprintf(stderr, "generator error:%s\n", msg);
	exit(1);
}

static Type get_type(Num *in){
	return in->vtype->type;
}

static char rsize(Num *in, int side){
	if(side == LHS){
		if(in->lhs->size == PTR_SIZE)return 'r';
		return 'e';
	}else if(side == RHS){
		if(in->rhs->size == PTR_SIZE)return 'r';
		return 'e';
	}else if(side == CENTER){
		if(in->size == PTR_SIZE)return 'r';
		return 'e';
	}
	error("rsize:hand side error");
	return 0;
}

static void print_num(Num *in);

static void print_addr(Num *in){
	if(in->type == ID){
		printf("mov $%d, %%rbx\n", -(in->offset));
		printf("lea (%%rbp, %%rbx), %%rax\n");
		printf("push %%rax\n");
	}else if(in->type == DEREF)
		print_num(in->lhs);
	else
		print_num(in);
}

static void print_num(Num *in){
	if(in == NULL)error("print_num:null");
	if(in->type == NUM){
		printf("push $%d\n", in->i);
		return;
	}else if(
			in->type == MUL ||
			in->type == AND ||
			in->type == XOR ||
			in->type == OR
			){
		char *code;
		switch(in->type){
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
		printf("%s %%%cbx, %%%cax\n", code, rsize(in, CENTER), rsize(in, CENTER));
		printf("push %%rax\n");
		return;
	}else if(in->type == ADD){
		print_num(in->lhs);
		print_num(in->rhs);
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		if(get_type(in->lhs) == TPTR
				&& get_type(in->rhs) != TPTR)
			printf("imul $%d, %%rbx\n", in->lhs->vtype->ptr->size);
		else if(get_type(in->rhs) == TPTR
				&& get_type(in->lhs) != TPTR)
			printf("imul $%d, %%rax\n", in->rhs->vtype->ptr->size);
		else if(get_type(in->lhs) == TPTR
				&& get_type(in->rhs) == TPTR)
			error("ADD:adding between pointers is invalid");
		printf("add %%%cbx, %%%cax\n", rsize(in, CENTER), rsize(in, CENTER));
		printf("push %%rax\n");
		return;
	}else if(in->type == SUB){
		print_num(in->lhs);
		print_num(in->rhs);
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		if(get_type(in->lhs) == TPTR){
			if(get_type(in->rhs) != TPTR)
				printf("imul $%d, %%rbx\n", in->lhs->vtype->ptr->size);
		}else if(get_type(in->rhs) == TPTR)
			error("invalid operation \"int - pointer\"");
		printf("sub %%rbx, %%rax\n");
		if(get_type(in->lhs) == TPTR
				&& get_type(in->rhs) == TPTR){
			printf("mov $0, %%rdx\n");
			printf("mov $%d, %%rcx\n", in->lhs->vtype->ptr->size);
			printf("div %%rcx\n");
		}
		printf("push %%rax\n");
		return;
	}else if(in->type == DIV || in->type == MOD){
		print_num(in->lhs);
		print_num(in->rhs);
		printf("mov $0, %%rdx\n");
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		printf("div %%%cbx\n", rsize(in, RHS));
		printf("push %%r%cx\n", in->type == DIV ? 'a' : 'd');
		return;
	}else if(in->type == PLUS || in->type == MINUS){
		print_num(in->lhs);
		if(in->type == MINUS){
			printf("pop %%rbx\n");
			printf("mov $0, %%rax\n");
			printf("sub %%%cbx, %%%cax\n", rsize(in, CENTER), rsize(in, CENTER));
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
				printf("cmp %%%cax, %%%cbx\n", rsize(in, CENTER), rsize(in, CENTER));
				break;
			case GRT:
			case GEQ:
			default:
				printf("cmp %%%cbx, %%%cax\n", rsize(in, CENTER), rsize(in, CENTER));
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
		printf("mov %%%cax, (%%rbx)\n", rsize(in, CENTER));
		printf("push %%rax\n");
		return;
	}else if(
			in->type == MULAS ||
			in->type == ADDAS ||
			in->type == SUBAS ||
			in->type == LSAS ||
			in->type == RSAS ||
			in->type == LAAS ||
			in->type == LXAS ||
			in->type == LOAS
			){
		char *code;
		switch(in->type){
			case  MULAS:
				code = "mul";
				break;
			case  ADDAS:
				code = "add";
				break;
			case  SUBAS:
				code = "sub";
				break;
			case  LSAS:
				code = "sal";
				break;
			case  RSAS:
				code = "sar";
				break;
			case  LAAS:
				code = "and";
				break;
			case  LXAS:
				code = "xor";
				break;
			case  LOAS:
				code = "or";
				break;
			default:;
		}
		print_addr(in->lhs);
		print_num(in->rhs);
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		printf("%s %%%cbx, (%%rax)\n", code, rsize(in, CENTER));
		printf("mov (%%rax), %%%cax\n", rsize(in, CENTER));
		printf("push %%rax\n");
		return;
	}else if(in->type == DIVAS || in->type == MODAS){
		char c = (in->type == DIVAS ? 'a' : 'd');
		print_addr(in->lhs);
		print_num(in->rhs);
		printf("pop %%rbx\n");
		printf("pop %%rcx\n");
		printf("mov $0, %%rdx\n");
		printf("mov (%%rcx), %%%cax\n", rsize(in, CENTER));
		printf("div %%%cbx\n", rsize(in, LHS));
		printf("mov %%%c%cx, (%%rcx)\n", rsize(in, CENTER), c);
		printf("push %%r%cx\n", c);
		return;
	}else if(in->type == ID){
		print_addr(in);
		printf("pop %%rbx\n");
		printf("mov (%%rbx), %%%cax\n", rsize(in, CENTER));
		printf("push %%rax\n");
		return;
	}else if(in->type == CALL){
		if(in->lhs)print_num(in->lhs);
		printf("call %s\n", in->name);
		printf("push %%rax\n");
		return;
	}else if(in->type == PTR){
		print_addr(in->lhs);
		return;
	}else if(in->type == DEREF){
		print_num(in->lhs);
		printf("pop %%rbx\n");
		printf("mov (%%rbx), %%%cax\n", rsize(in, CENTER));
		printf("push %%rax\n");
		return;
	}else if(in->type == ARG){
		const char *arglist[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
		int c = 0;
		Num *tmp = in;
		while(tmp){
			print_num(tmp->rhs);
			tmp = tmp->lhs;
			c++;
		}
		for(; c; c--){
			printf("pop %%%s\n", arglist[c - 1]);
		}
		return;
	}

	fprintf(stderr, "%d\n", in->type);
	error("print_num:undefined");
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
		if(in->Nchild){
			print_num(in->Nchild);
			printf("pop %%rax\n");
		}
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

void print_def(Def *in){
	if(in == NULL)error("print_def:NULL");
	if(in->type == FUN){
		printf(".globl %s\n", in->name);
		printf("%s:\n", in->name);
		printf("push %%rbp\n");
		printf("mov %%rsp, %%rbp\n");
		if(in->idcount)
			printf("sub $%d, %%rsp\n", in->idcount);
		print_stmt(in->Schild);
		return;
	}
	error("print_def");
}

void generator(void *in){
	if(in == NULL){
		fprintf(stderr, "generator error\n");
		exit(1);
	}
	printf(".globl main\n");
	print_def(in);
}
