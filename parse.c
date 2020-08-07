#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

Stmt *new_Stmt(Stype t){
	Stmt *res = malloc(sizeof(Stmt));
	res->type = t;
	res->Nchild = NULL;
	res->rhs = res->lhs = NULL;
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
			if(c < '0' || '9' < c)
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
	return NULL;
}

// unary_expression <- unary-operator? primary_expression
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
	}
	res = primary_expression();
	return res;
}

// multiplicative_expression <- unary_expression (('*' / '-' / '%') unary_expression)*
static Num *multiplicative_expression(){
	Num *res = unary_expression();
	if(!res)return res;
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

// additive_expression <- multiplicative_expression (('+' / '-') multiplicative_expression)*
static Num *additive_expression(){
	Num *res = multiplicative_expression();
	if(!res)return res;
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

// TODO shift-expression

// relational_expression <- additive_expression (relational-operator additive_expression)*
// relational-operator <- '<=' / '>=' / '<' / '>'
static Num *relational_expression(){
	Num *res = additive_expression();
	if(!res)return res;
	Num *tmp;
	char c = str_getchar(input, pos);
	while(c == '<' || c == '>'){
		if(c == '<')tmp = new_Num(LES);
		else tmp = new_Num(GRT);
		tmp->lhs = res;
		pos++;
		c = str_getchar(input, pos);
		if(c == '='){
			tmp->type = (tmp->type == LES ? LEQ : GEQ);
			pos++;
		}
		Spacing();
		tmp->rhs = additive_expression();
		res = tmp;
		c = str_getchar(input, pos);
	}
	return res;
}

// equality_expression <- relational_expression (('==' / '!=') relational_expression)*
static Num *equality_expression(){
	Num *res = relational_expression();
	if(!res)return res;
	Num *tmp;
	char *c = str_pn(input, pos);
	while(strncmp("==", c, 2) == 0 || strncmp(c, "!=", 2) == 0){
		pos += 2;
		Spacing();
		if(*c == '!')tmp = new_Num(NEQ);
		else tmp = new_Num(EQ);
		tmp->lhs = res;
		res = tmp;
		res->rhs = relational_expression();
		c = str_pn(input, pos);
	}
	return res;
}

// AND_expression <- equality_expression ('&' equlity_expression)*
static Num *and_expression(){
	Num *res = equality_expression();
	if(!res)return res;
	Num *tmp;
	char c = str_getchar(input, pos);
	while(c == '&'){
		pos++;
		Spacing();
		tmp = new_Num(AND);
		tmp->lhs = res;
		res = tmp;
		res->rhs = equality_expression();
		c = str_getchar(input, pos);
	}
	return res;
}

// exclusive_OR_expression <- AND_expression ('^' AND_expression)*
static Num *exclusive_or_expression(){
	Num *res = and_expression();
	if(!res)return res;
	Num *tmp;
	char c = str_getchar(input, pos);
	while(c == '^'){
		pos++;
		Spacing();
		tmp = new_Num(XOR);
		tmp->lhs = res;
		res = tmp;
		res->rhs = and_expression();
		c = str_getchar(input, pos);
	}
	return res;
}

// inclusive_OR_expression <- exclusive_OR_expression ('|' exclusive_OR_expression)*
static Num *inclusive_or_expression(){
	Num *res = exclusive_or_expression();
	if(!res)return res;
	Num *tmp;
	char c = str_getchar(input, pos);
	while(c == '|'){
		pos++;
		Spacing();
		tmp = new_Num(OR);
		tmp->lhs = res;
		res = tmp;
		res->rhs = exclusive_or_expression();
		c = str_getchar(input, pos);
	}
	return res;
}

// TODO logical-AND-expression ~ assignment-expression

// expression <- equality_expression
static Num *expression(){
	return inclusive_or_expression();
}

static Stmt *statement();
// jump_statement <- 'return' expression^opt ';'
static Stmt *jump_statement(){
	char *c = str_pn(input, pos);
	Stmt *res;
	if(strncmp(c, "return", 6) == 0){
		pos += 6;
		Spacing();
		res = new_Stmt(RET);
		res->Nchild = expression();
		c = str_pn(input, pos);
		if(*c != ';'){
			error("jump-statement: no semicoron");
		}
		pos++;
		Spacing();
		return res;
	}
	return NULL;
}

// expression_statement <- expression^opt ';'
static Stmt *expression_statement(){
	Stmt *res;
	Num *tmp;
	char c = str_getchar(input, pos);
	tmp = expression();
	c = str_getchar(input, pos);
	if(c != ';'){
		if(tmp)error("expression_statement:missing semicoron");
		else return NULL;
	}
	res = new_Stmt(EXP);
	res->Nchild = tmp;
	pos++;
	Spacing();
	return res;
}

// block_item <- statement
static Stmt *block_item(){
	Stmt *res;
	Stmt *tmp = statement();
	if(!tmp)return NULL;
	res = new_Stmt(ITEM);
	res->rhs = tmp;
	return res;
}

// block_item_list <- block_item+
static Stmt *block_item_list(){
	Stmt *res = block_item();
	if(!res)return res;
	Stmt *tmp = res;
	while((tmp->lhs = block_item()))tmp = tmp->lhs;
	return res;
}

// compound_statement <- '{' block_item_list^opt '}'
static Stmt *compound_statement(){
	char c = str_getchar(input, pos);
	Stmt *res;
	if(c != '{')return NULL;
	pos++;
	Spacing();
	res = new_Stmt(CPD);
	res->rhs = block_item_list();
	c = str_getchar(input, pos);
	if(c != '}')error("compound-expression");
	pos++;
	Spacing();
	return res;
}

// statement <- expression_statement / jump_statement
static Stmt *statement(){
	Stmt *res;
	if((res = compound_statement()))return res;
	if((res = jump_statement()))return res;
	if((res = expression_statement()))return res;
	return NULL;
}

void *parse(str *s){
	input = s;
	pos = 0;
	Spacing();
	return statement();
}
