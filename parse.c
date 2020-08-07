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

static void Spacing(){
	char c;
	while((c = str_getchar(input, pos))){
		if(
				c == ' ' ||
				c == '\t' ||
				c == '\r' ||
				c == '\n'
		  ){
			pos++;
			continue;
		}
		break;
	}
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
		Spacing();
		return res;
	}
	Spacing();
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
		Spacing();
		res = expression();
		if(res == NULL)error("primary_expression:( expression )");
		c = str_getchar(input, pos);
		if(c != ')')error("primary_expression:missing )");
		pos++;
		Spacing();
		return res;
	}
	return error("primary_expression");
}

// unary_expression <- unary-operator primary_expression
// unary-operator <- '+' / '-'
static Num *unary_expression(){
	char c = str_getchar(input, pos);
	Num *res;
	if(c == '+' || c == '-'){
		pos++;
		Spacing();
		res = new_Num(c == '+' ? PLUS : MINUS);
		res->lhs = primary_expression();
		return res;
	}else{
		res = primary_expression();
		return res;
	}
	return error("unary_expression");
}

// multiplicative_expression <- unary_expression (('*' / '-' / '%') unary_expression)*
static Num *multiplicative_expression(){
	Num *res = unary_expression();
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
		Spacing();
		tmp->lhs = res;
		res = tmp;
		res->rhs = unary_expression();
		c = str_getchar(input, pos);
	}
	Spacing();
	return res;
}

// expression <- multiplicative_expression (('+' / '-') multiplicative_expression)*
static Num *expression(){
	Num *res = multiplicative_expression();
	char c = str_getchar(input, pos);
	while(c == '+' || c == '-'){
		Num *tmp = new_Num(c == '+' ? ADD : SUB);
		pos++;
		tmp->lhs = res;
		res = tmp;
		Spacing();
		res->rhs = multiplicative_expression();
		c = str_getchar(input, pos);
	}
	Spacing();
	return res;
}

void *parse(str *s){
	input = s;
	pos = 0;
	Spacing();
	return expression();
}
