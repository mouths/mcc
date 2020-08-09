#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "str.h"
#include "parse.h"
#include "tool.h"

str *input;
int pos;
list *idlist;

struct oplist{
	char *op;
	int len;
	Ntype type;
};

struct keywords{
	char *word;
	int len;
};

struct id_container{
	char *name;
	int id;
};

static void *error(char *msg){
	if(msg == NULL)
		fprintf(stderr, "parse error:%d\n%.10s\n", pos, str_pn(input, pos));
	else
		fprintf(stderr, "parse error:%d:%s\n%.10s\n", pos, msg, str_pn(input, pos));

	exit(1);
}

static Num *new_Num(Ntype t){
	Num *res = malloc(sizeof(Num));
	res->type = t;
	res->lhs = res->rhs = NULL;
	res->id = 0;
	res->name = NULL;
	return res;
}

static Stmt *new_Stmt(Stype t){
	Stmt *res = malloc(sizeof(Stmt));
	res->type = t;
	res->Nchild = NULL;
	res->rhs = res->lhs = NULL;
	return res;
}

static Def *new_Def(Dtype t){
	Def *res = malloc(sizeof(Def));
	res->type = t;
	res->Schild = NULL;
	res->name = NULL;
	res->idcount = 0;
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

// nondigit <- [a-zA-Z_]
static bool nondigit(){
	char c = str_getchar(input, pos);
	return
		c == '_' ||
		('a' <= c && c <= 'z') ||
		('A' <= c && c <= 'Z');
}

static bool nondigit_n(int n){
	char c = str_getchar(input, pos + n);
	return
		c == '_' ||
		('a' <= c && c <= 'z') ||
		('A' <= c && c <= 'Z');
}

// digit <- [0-9]
static bool digit(){
	char c = str_getchar(input, pos);
	return ('0' <= c && c <= '9');
}

static bool digit_n(int n){
	char c = str_getchar(input, pos + n);
	return ('0' <= c && c <= '9');
}

static int keyword(){
	const struct keywords keys[44] = {
		{"auto", 4},
		{"break", 5},
		{"case", 4},
		{"char", 4},
		{"const", 5},
		{"continue", 8},
		{"default", 7},
		{"do", 2},
		{"double", 6},
		{"else", 4},
		{"enum", 4},
		{"extern", 6},
		{"float", 5},
		{"for", 3},
		{"goto", 4},
		{"if", 2},
		{"inline", 6},
		{"int", 3},
		{"long", 4},
		{"register", 8},
		{"restrict", 8},
		{"return", 6},
		{"short", 5},
		{"signed", 6},
		{"sizeof", 6},
		{"static", 6},
		{"struct", 6},
		{"switch", 6},
		{"typedef", 7},
		{"union", 5},
		{"unsigned", 8},
		{"void", 4},
		{"volatile", 8},
		{"while", 5},
		{"_Alignas", 8},
		{"_Alignof", 8},
		{"_Atomic", 7},
		{"_Bool", 5},
		{"_Complex", 8},
		{"_Generic", 8},
		{"_Imaginary", 10},
		{"_Noreturn", 9},
		{"_Static_assert", 14},
		{"_Thread_local", 13},
	};
	char *c = str_pn(input, pos);
	for(int i = 43; i >= 0; i--){
		if(strncmp(c, keys[i].word, keys[i].len) == 0 && !nondigit_n(keys[i].len))return i;
	}
	return -1;
}

static int is_identifier(){
	if(keyword() >= 0)return 0;
	int res = 0;
	if(nondigit()){
		res = 1;
		while(nondigit_n(res) || digit_n(res))res++;
	}
	return res;
}

//  constant <- digit+
static Num *constant(){
	char c = str_getchar(input, pos);
	Num *res;
	if(digit()){
		res = new_Num(NUM);
		res->i = c - '0';
		pos++;
		while((c = str_getchar(input, pos))){
			if(!digit())
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

static int id_container_cmp(const void *a, const void *b){
	return strcmp(((struct id_container*)a)->name, ((struct id_container*)b)->name);
}

// identifier <- !keyword nondigit (nondigit / digit)*
static Num *identifier(){
	Num *res;
	int i = is_identifier();
	struct id_container *con, *tmp;
	if(i > 0){
		con = malloc(sizeof(struct id_container));
		con->name = malloc(sizeof(char) * (i + 1));
		strncpy(con->name, str_pn(input, pos), i);
		con->name[i] = '\0';
		res = new_Num(ID);
		tmp = list_search(con, id_container_cmp, idlist);
		if(tmp == NULL){
			list_append(con, idlist);
			con->id = list_len(idlist);
			res->id = con->id;
		}else{
			res->id = tmp->id;
			free(con->name);
			free(con);
		}
		res->name = malloc(sizeof(char) * (i + 1));
		strncpy(res->name, str_pn(input, pos), i);
		res->name[i] = '\0';
		pos += i;
		Spacing();
		return res;
	}
	return NULL;
}

static Num *expression();
// primary_expression <- constant / identifier / ( expression )
static Num *primary_expression(){
	Num *res;
	if((res = constant()))return res;
	if((res = identifier()))return res;
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

// postfix_expression <- primary_expression ('(' ')')?
static Num *postfix_expression(){
	Num *res = primary_expression();
	if(!res)return res;
	char c = str_getchar(input, pos);
	if(c == '('){
		pos++;
		Spacing();
		c = str_getchar(input, pos);
		if(c != ')')error("postfix_expression:missing ')'");
		pos++;
		Spacing();
		res->type = CALL;
	}
	return res;
}

// unary_expression <- unary-operator? postfix_expression
// unary-operator <- '+' / '-' / '&' / '*'
static Num *unary_expression(){
	Num *res;
	char c = str_getchar(input, pos);
	if(c == '+' || c == '-' || c == '&' || c == '*'){
		pos++;
		Spacing();
		switch(c){
			case '+':
				res = new_Num(PLUS);
				break;
			case '-':
				res = new_Num(MINUS);
				break;
			case '&':
				res = new_Num(PTR);
				break;
			case '*':
				res = new_Num(DEREF);
				break;
		}
		res->lhs = postfix_expression();
		return res;
	}
	res = postfix_expression();
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

// assignment_expression <- unary_expression assignment-operator assignment_expression / inclusive_or_expression
// assignment-operator <- '=' / '*=' / '/=' / '%=' / '+=' / '-=' / '<<=' / '>>=' / '&=' / '^=' / '|='
static Num *assignment_expression(){
	const struct oplist ops[11] = {
		{"=", 1, AS},
		{"*=", 2, MULAS},
		{"/=", 2, DIVAS},
		{"%=", 2, MODAS},
		{"+=", 2, ADDAS},
		{"-=", 2, SUBAS},
		{"<<=", 3, LSAS},
		{">>=", 3, RSAS},
		{"&=", 2, LAAS},
		{"^=", 2, LXAS},
		{"|=", 2, LOAS},
	};
	int p = pos;
	Num *res = unary_expression();
	Num *tmp;
	if(!res)return inclusive_or_expression();
	char *c = str_pn(input, pos);
	for(int i = 0; i < 11; i++){
		if(i == 0 && strncmp(c, "==", 2) == 0)continue;
		if(strncmp(c, ops[i].op, ops[i].len) == 0){
			pos += ops[i].len;
			Spacing();
			tmp = new_Num(ops[i].type);
			tmp->lhs = res;
			res = tmp;
			if(!(res->rhs = assignment_expression()))
				error("assignment_expression:rhs is NULL");
			return res;
		}
	}
	pos = p;
	return inclusive_or_expression();
}

// expression <- inclusive_or_expression
static Num *expression(){
	return assignment_expression();
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
		if(tmp){
			fprintf(stderr, "expression_type:%d\n", tmp->type);
			error("expression_statement:missing semicoron");
		}
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

// function_definition <- type-specifier identifier '(' ')' compound_statement
// type-specifier <- 'int'
static Def *function_definition(){
	char *c = str_pn(input, pos);
	Def *res = new_Def(FUN);
	if(strncmp(c, "int", 3) != 0 || keyword() < 0)return NULL;
	pos += 3;
	Spacing();
	int id = is_identifier();
	if(id == 0)return NULL;
	res->name = malloc(sizeof(char) * (id + 1));
	strncpy(res->name, str_pn(input, pos), id);
	res->name[id] = '\0';
	pos += id;
	Spacing();
	c = str_pn(input, pos);
	if(*c != '(') return NULL;
	pos++;
	Spacing();
	c = str_pn(input, pos);
	if(*c != ')')return NULL;
	pos++;
	Spacing();
	res->Schild = compound_statement();
	res->idcount = list_len(idlist);
	return res;
}


void *parse(str *s){
	input = s;
	pos = 0;
	idlist = list_empty();
	Spacing();
	return function_definition();
}
