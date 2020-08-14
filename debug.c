#include <stdio.h>

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
	}
}

void print_Typeinfo(Typeinfo *t){
	p_Typeinfo(t);
	fprintf(stderr, "\n");
}
