#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "parse.h"

str *input;
int pos;

static void *error(char *msg){
	if(msg == NULL)
		fprintf(stderr, "parse error:%d\n%.10s\n", pos, str_pn(input, pos));
	else
		fprintf(stderr, "parse error:%d:%s\n%.10s\n", pos, msg, str_pn(input, pos));

	exit(1);
}

Num *new_Num(Ntype t){
	Num *res = malloc(sizeof(Num));
	res->type = t;
	res->lhs = res->rhs = NULL;
	return res;
}

// num <- [0-9]+
static Num *num(){
	char c = str_getchar(input, pos);
	Num *res;
	if('0' <= c && c <= '9'){
		res = new_Num(NUM);
		res->i = c - '0';
		pos++;
		while((c = str_getchar(input, pos))){
			if(c <= '0' || '9' <= c)
				break;
			res->i = res->i * 10 + c - '0';
			pos++;
		}
		return res;
	}
	return error("expr");
}

// term <- num (('*' / '-') num)*
static Num *term(){
	Num *res = num();
	char c = str_getchar(input, pos);
	while(c == '*' || c == '/'){
		Num *tmp = new_Num(c == '*' ? MUL : DIV);
		pos++;
		tmp->lhs = res;
		res = tmp;
		res->rhs = num();
		c = str_getchar(input, pos);
	}
	return res;
}

// expr <- term (('+' / '-') term)*
static Num *expr(){
	Num *res = term();
	char c = str_getchar(input, pos);
	while(c == '+' || c == '-'){
		Num *tmp = new_Num(c == '+' ? ADD : SUB);
		pos++;
		tmp->lhs = res;
		res = tmp;
		res->rhs = term();
		c = str_getchar(input, pos);
	}
	return res;
}

void *parse(str *s){
	input = s;
	pos = 0;
	return expr();
}
