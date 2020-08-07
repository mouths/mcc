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

//  constant <- [0-9]+
static Num *constant(){
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
	return NULL;
}

static Num *expression();
// primary_expression <- constant / ( expression )
static Num *primary_expression(){
	Num *res = constant();
	if(res)return res;
	char c = str_getchar(input, pos);
	if(c == '('){
		pos++;
		res = expression();
		if(res == NULL)error("primary_expression:( expression )");
		c = str_getchar(input, pos);
		if(c != ')')error("primary_expression:missing )");
		return res;
	}
	return error("primary_expression");
}

// term <- primary_expression (('*' / '-' / '%') primary_expression)*
static Num *term(){
	Num *res = primary_expression();
	char c = str_getchar(input, pos);
	while(c == '*' || c == '/' || c == '%'){
		Num *tmp = new_Num(NUM);
		switch(c){
			case '*':
				tmp->type = MUL;
				break;
			case '/':
				tmp->type = DIV;
				break;
			case '%':
				tmp->type = MOD;
				break;
		}
		pos++;
		tmp->lhs = res;
		res = tmp;
		res->rhs = primary_expression();
		c = str_getchar(input, pos);
	}
	return res;
}

// expression <- term (('+' / '-') term)*
static Num *expression(){
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
	return expression();
}
