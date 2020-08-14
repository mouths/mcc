#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "parse.h"
#include "tool.h"

#define LHS 0
#define RHS 1
#define CENTER 2

const char *arglist[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

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
		if(in->lhs->vtype->size == PTR_SIZE)return 'r';
		return 'e';
	}else if(side == RHS){
		if(in->rhs->vtype->size == PTR_SIZE)return 'r';
		return 'e';
	}else if(side == CENTER){
		if(in->vtype->size == PTR_SIZE)return 'r';
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
	}else if(in->type == GVAR){
		printf("lea %s(%%rip), %%rax\n", in->name);
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
	}else if(in->type == ADD){
		print_num(in->lhs);
		print_num(in->rhs);
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		if((
					get_type(in->lhs) == TPTR ||
					get_type(in->lhs) == TARRAY)
				&& !(
					get_type(in->rhs) == TPTR ||
					get_type(in->rhs) == TARRAY))
			printf("imul $%d, %%rbx\n", in->lhs->vtype->ptr->size);
		else if((
					get_type(in->rhs) == TPTR ||
					get_type(in->rhs) == TARRAY)
				&& !(
					get_type(in->lhs) == TPTR ||
					get_type(in->lhs) == TARRAY))
			printf("imul $%d, %%rax\n", in->rhs->vtype->ptr->size);
		else if((
					get_type(in->lhs) == TPTR ||
					get_type(in->lhs) == TARRAY)
				&& (
					get_type(in->rhs) == TPTR ||
					get_type(in->rhs) == TARRAY))
			error("ADD:adding between pointers is invalid");
		printf("add %%%cbx, %%%cax\n", rsize(in, CENTER), rsize(in, CENTER));
		printf("push %%rax\n");
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
	}else if(in->type == DIV || in->type == MOD){
		print_num(in->lhs);
		print_num(in->rhs);
		printf("mov $0, %%rdx\n");
		printf("pop %%rbx\n");
		printf("pop %%rax\n");
		printf("div %%%cbx\n", rsize(in, RHS));
		printf("push %%r%cx\n", in->type == DIV ? 'a' : 'd');
	}else if(in->type == PLUS || in->type == MINUS){
		print_num(in->lhs);
		if(in->type == MINUS){
			printf("pop %%rbx\n");
			printf("mov $0, %%rax\n");
			printf("sub %%%cbx, %%%cax\n", rsize(in, CENTER), rsize(in, CENTER));
			printf("push %%rax\n");
		}
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
	}else if(
			in->type == AS
			){
		print_addr(in->lhs);
		print_num(in->rhs);
		printf("pop %%rax\n");
		printf("pop %%rbx\n");
		if(in->vtype->size != CHAR_SIZE)
			printf("mov %%%cax, (%%rbx)\n", rsize(in, CENTER));
		else
			printf("mov %%al, (%%rbx)\n");
		printf("push %%rax\n");
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
	}else if(in->type == ID){
		print_addr(in);
		if(get_type(in) != TARRAY){
			printf("pop %%rbx\n");
			if(in->vtype->size != CHAR_SIZE)
				printf("mov (%%rbx), %%%cax\n", rsize(in, CENTER));
			else
				printf("movsx (%%rbx), %%ax\n");
			printf("push %%rax\n");
		}
	}else if(in->type == CALL){
		if(in->lhs)print_num(in->lhs);
		printf("mov $0, %%al\n");
		printf("call %s\n", in->name);
		printf("push %%rax\n");
	}else if(in->type == PTR){
		print_addr(in->lhs);
	}else if(in->type == DEREF){
		print_num(in->lhs);
		printf("pop %%rbx\n");
		printf("mov (%%rbx), %%%cax\n", rsize(in, CENTER));
		printf("push %%rax\n");
	}else if(in->type == ARG){
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
	}else if(in->type == GVAR){
		print_addr(in);
		if(get_type(in) != TARRAY){
			printf("pop %%rbx\n");
			printf("mov (%%rbx), %%%cax\n", rsize(in, CENTER));
			printf("push %%rax\n");
		}
	}else if(in->type == STR){
		printf("lea .L%d(%%rip), %%rax\n", in->i);
		printf("push %%rax\n");
	}else if(in->type == LAND){
		print_num(in->lhs);
		printf("pop %%rax\n");
		printf("cmp $0, %%rax\n");
		printf("je .Liend%d\n", in->i);
		print_num(in->rhs);
		printf("pop %%rax\n");
		printf("cmp $0, %%rax\n");
		printf("setne %%al\n");
		printf("movzb %%al, %%rax\n");
		printf(".Liend%d:\n", in->i);
		printf("push %%rax\n");
	}else{
		fprintf(stderr, "%d\n", in->type);
		error("print_num:undefined");
	}
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
	}else if(in->type == EXP){
		if(in->Nchild){
			print_num(in->Nchild);
			printf("pop %%rax\n");
		}
	}else if(in->type == CPD){
		if(in->rhs)print_stmt(in->rhs);
	}else if(in->type == ITEM){
		print_stmt(in->rhs);
		if(in->lhs)print_stmt(in->lhs);
	}else if(in->type == IF){
		print_num(in->Nchild);
		printf("pop %%rax\n");
		printf("cmp $0, %%rax\n");
		if(in->rhs){
			printf("je .Lielse%d\n", in->count);
			print_stmt(in->lhs);
			printf("jmp .Liend%d\n", in->count);
			printf(".Lielse%d:\n", in->count);
		print_stmt(in->rhs);
		}else{
			printf("je .Liend%d\n", in->count);
			print_stmt(in->lhs);
		}
		printf(".Liend%d:\n", in->count);
	}else if(in->type == WHILE){
		if(in->rhs){
			printf(".Llbegin%d:\n", in->count);
			print_num(in->Nchild);
			printf("pop %%rax\n");
			printf("cmp $0, %%rax\n");
			printf("je .Llend%d\n", in->count);
			print_stmt(in->rhs);
			printf("jmp .Llbegin%d\n", in->count);
			printf(".Llend%d:\n", in->count);
		}else{
			printf(".Llbegin%d:\n", in->count);
			print_stmt(in->lhs);
			print_num(in->Nchild);
			printf("pop %%rax\n");
			printf("cmp $0, %%rax\n");
			printf("jne .Llbegin%d\n", in->count);
		}
	}else if(in->type == FOR){
		if(in->init){
			print_num(in->init);
			printf("pop %%rax\n");
		}
		printf(".Llbegin%d:\n", in->count);
		printf("mov $1, %%rax\n");
		if(in->Nchild){
			print_num(in->Nchild);
			printf("pop %%rax\n");
		}
		printf("cmp $0, %%rax\n");
		printf("je .Llend%d\n", in->count);
		print_stmt(in->rhs);
		if(in->iteration){
			print_num(in->iteration);
			printf("pop %%rax\n");
		}
		printf("jmp .Llbegin%d\n", in->count);
		printf(".Llend%d:\n", in->count);
	}else{
		error("print_statement");
	}
}

void print_def(Def *in){
	if(in == NULL)error("print_def:NULL");
	if(in->type == FUN){
		printf(".text\n");
		printf(".globl %s\n", in->name);
		printf("%s:\n", in->name);
		printf("push %%rbp\n");
		printf("mov %%rsp, %%rbp\n");
		if(in->idcount)
			printf("sub $%d, %%rsp\n", in->idcount);
		int p = 0;
		for(Num *i = in->arguments; i; i = i->lhs){
			printf("mov %%%s, %%rax\n", arglist[p++]);
			printf("mov $%d, %%rbx\n", -i->offset);
			printf("lea (%%rbp, %%rbx), %%rbx\n");
			printf("mov %%%cax, (%%rbx)\n", rsize(i, CENTER));
		}
		print_stmt(in->Schild);
		if(in->next)print_def(in->next);
	}else if(in->type == GVDEF){
		printf(".data\n");
		printf("%s:\n", in->name);
		printf(".zero %d\n", in->idcount);
		if(in->next)print_def(in->next);
	}else{
		error("print_def");
	}
}

static void print_lit(list *lst){
	int len = list_len(lst);
	for(int i = 0; i < len; i++){
		printf(".L%d:\n", i);
		printf(".string \"%s\"\n", (char*)list_getn(i, lst));
	}
}

void generator(void *in){
	if(in == NULL){
		fprintf(stderr, "generator error\n");
		exit(1);
	}
	if(list_len(((Def *)in)->strlist)){
		printf(".section\t.rodata\n");
		print_lit(((Def *)in)->strlist);
	}
	print_def(in);
}
