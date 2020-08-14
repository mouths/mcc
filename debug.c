#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

static void p_Typeinfo(Typeinfo *t){
	if(t->type == TINT){
		fprintf(stderr, "int");
	}else if(t->type == TCHAR){
		fprintf(stderr, "char");
	}else if(t->type == TPTR){
		fprintf(stderr, "* -> ");
		p_Typeinfo(t->ptr);
	}else if(t->type == TARRAY){
		fprintf(stderr, "[ ] ");
		p_Typeinfo(t->ptr);
	}else if(t->type == TFUNC){
		p_Typeinfo(t->ptr);
		fprintf(stderr, " FUNC(");
		for(int i = 0; i < list_len(t->args); i++)
			p_Typeinfo(list_getn(i, t->args));
		fprintf(stderr, ")");
	}else{
		fprintf(stderr, "print_Typeinfo:unknown type:%d\n", t->type);
		exit(1);
	}
}

void print_Typeinfo(Typeinfo *t){
	p_Typeinfo(t);
	fprintf(stderr, "\n");
}
