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
	int offset;
	Typeinfo *type;
	int size;
};

static struct id_container *new_container(){
	struct id_container *res = malloc(sizeof(struct id_container));
	res->name = NULL;
	res->offset = 0;
	res->type = NULL;
	res->size = 0;
}

static char *name_copy(int n){
	char *res = malloc(sizeof(char) * (n + 1));
	strncpy(res, str_pn(input, pos), n);
	res[n] = '\0';
	return res;
}

static char *name_dup(char *in){
	int len = strlen(in);
	char *res = malloc(sizeof(char) * (len + 1));
	strncpy(res, in, len);
	res[len] = '\0';
	return res;
}

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
	res->offset = 0;
	res->name = NULL;
	res->size = 0;
	res->vtype = NULL;
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

static Typeinfo *new_Typeinfo(Type type){
	Typeinfo *res = malloc(sizeof(Typeinfo));
	res->type =type;
	res->ptr = NULL;
	res->size = 0;
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

static void Spacing_n(int n){
	pos += n;
	Spacing();
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

static int count_id_offset(void *in){
	return ((struct id_container *)in)->size;
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
		res->vtype = new_Typeinfo(TINT);
		res->vtype->size = res->size = INT_SIZE;
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
		con->name = name_copy(i);
		res = new_Num(ID);
		tmp = list_search(con, id_container_cmp, idlist);
		if(tmp == NULL)error("undefined variable");
		res->offset = tmp->offset;
		res->vtype = tmp->type;
		res->size = tmp->size;
		free(con->name);
		free(con);
		res->name = name_copy(i);
		Spacing_n(i);
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

static Num *assignment_expression();
// argument_expression_list <- assignment_expression (',' assignment_expression)*
static Num *argument_expression_list(){
	Num *res, *tmp;
	if((tmp = assignment_expression()) == NULL)
		return NULL;
	res = new_Num(ARG);
	res->rhs = tmp;
	res->size = tmp->size;
	tmp = res;
	char c = str_getchar(input, pos);
	while(c == ','){
		Spacing_n(1);
		tmp = tmp->lhs = new_Num(ARG);
		if((tmp->rhs = assignment_expression()) == NULL)
			error("argument_expression_list:missing argument");
		c = str_getchar(input, pos);
	}
	return res;
}

// postfix_expression <- primary_expression ('(' argument_expression_list? ')')?
static Num *postfix_expression(){
	Num *res = primary_expression();
	if(!res)return res;
	char c = str_getchar(input, pos);
	if(c == '('){
		Spacing_n(1);
		res->lhs = argument_expression_list();
		c = str_getchar(input, pos);
		if(c != ')')error("postfix_expression:missing ')'");
		Spacing_n(1);
		res->type = CALL;
	}
	return res;
}

static Num *unary_expression();
// cast_expression <- unary_expression
static Num *cast_expression(){
	return unary_expression();
}

// unary_expression <- 'sizeof' unary_expression / unary-operator? postfix_expression
// unary-operator <- '+' / '-' / '&' / '*'
static Num *unary_expression(){
	Num *res, *tmp;
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
		res->lhs = cast_expression();
		res->vtype = res->lhs->vtype;
		if(c == '*'){
			if(res->lhs->vtype->type != TPTR)
				error("unary_expression:deref to non-pointer object");
			res->vtype = res->lhs->vtype->ptr;
			res->size = res->lhs->vtype->ptr->size;
		}else if(c == '&'){
			res->vtype = new_Typeinfo(TPTR);
			res->vtype->ptr = res->lhs->vtype;
			res->size = PTR_SIZE;
		}
		return res;
	}else if(strncmp(str_pn(input, pos), "sizeof", 6) == 0 && keyword() >= 0){
		Spacing_n(6);
		tmp = unary_expression();
		res = new_Num(NUM);
		res->i = tmp->size;
		res->size = INT_SIZE;
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
		Spacing_n(1);
		tmp->lhs = res;
		res = tmp;
		res->rhs = unary_expression();
		if(res->lhs->size > res->rhs->size){
			res->size = res->lhs->size;
			res->vtype = res->lhs->vtype;
		}else{
			res->size = res->rhs->size;
			res->vtype = res->rhs->vtype;
		}
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
		if(res->lhs->size > res->rhs->size){
			res->size = res->lhs->size;
			res->vtype = res->lhs->vtype;
		}else{
			res->size = res->rhs->size;
			res->vtype = res->rhs->vtype;
		}
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
		res->size = INT_SIZE;
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
		res->size = INT_SIZE;
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
		res->size = (
				res->lhs->size > res->rhs->size
				? res->lhs->size
				: res->rhs->size);
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
		res->size = (
				res->lhs->size > res->rhs->size
				? res->lhs->size
				: res->rhs->size);
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
		res->size = (
				res->lhs->size > res->rhs->size
				? res->lhs->size
				: res->rhs->size);
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
			res->size = res->lhs->size;
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

static bool declaration();
// block_item <- statement / declaration
static Stmt *block_item(){
	Stmt *res, *tmp = NULL;
	if(declaration())
		tmp = new_Stmt(EXP);
	if(!tmp)tmp = statement();
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
	if(c != '}')error("compound-statement");
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

// type_specifier <- 'int'
static Typeinfo *type_specifier(){
	char *c = str_pn(input, pos);
	if(strncmp(c, "int", 3) != 0)return NULL;
	if(nondigit_n(3))return NULL;
	Typeinfo *res = new_Typeinfo(TINT);
	res->size = INT_SIZE;
	Spacing_n(3);
	return res;
}

// declaration_specifiers <- type_specifier
static Typeinfo *declaration_specifiers(){
	return type_specifier();
}

/*
// identifier_list <- identifier (',' identifier)*
static bool identifier_list(){
	int id = is_identifier();
	if(id <= 0)return false;
	struct id_container *con = malloc(sizeof(struct id_container));
	con->name = name_copy(id);
	if(list_search(con, id_container_cmp, idlist))
		error("identifier_list:redeclaration variable");
	list_append(con, idlist);
	con->id = list_len(idlist);
	Spacing_n(id);
	char c = str_getchar(input, pos);
	while(c == ','){
		Spacing_n(1);
		id = is_identifier();
		if(id <= 0)error("identifier_list:missing identifier");
		con = malloc(sizeof(struct id_container));
		con->name = name_copy(id);
		if(list_search(con, id_container_cmp, idlist))
			error("identifier_list:redeclaration variable");
		list_append(con, idlist);
		con->id = list_len(idlist);
		Spacing_n(id);
		c = str_getchar(input, pos);
	}
	return true;
}
*/

static char* declarator(Typeinfo *type);
// parameter_declaration <- declaration_specifiers declarator
static bool parameter_declaration(){
	Typeinfo *type = declaration_specifiers();
	if(!type)return NULL;
	char *tmp;
	if((tmp = declarator(type)) == NULL)error("parameter_declaration:missing identifier");
	free(tmp);
	return true;
}

// parameter_list <- parameter_declaration (',' parameter_declaration)*
static bool parameter_list(){
	if(!parameter_declaration())return false;
	char c = str_getchar(input, pos);
	while(c == ','){
		Spacing_n(1);
		if(!parameter_declaration())error("parameter_list:missing identifier");
		c = str_getchar(input, pos);
	}
	return true;
}

// parameter_type_list <- parameter_list
static bool parameter_type_list(){
	return parameter_list();
}

// direct_declarator <- identifier ('(' parameter_type__list? ')')?
static char *direct_declarator(Typeinfo *type){
	int id = is_identifier();
	if(id <= 0)return NULL;
	struct id_container *con = new_container();
	con->name = name_copy(id);
	if(list_search(con, id_container_cmp, idlist))
		error("redeclaration");
	con->type = type;
	con->size = type->size;
	con->offset = list_map_sum(count_id_offset, idlist) + con->size;
	list_append(con, idlist);
	Spacing_n(id);
	char c = str_getchar(input, pos);
	if(c == '('){
		Spacing_n(1);
		parameter_type_list();
		c = str_getchar(input, pos);
		if(c != ')')error("direct_declarator:miasing ')'");
		Spacing_n(1);
	}
	return con->name;
}

// pointer <- '*'+
static Typeinfo *pointer(Typeinfo* type){
	Typeinfo *tmp;
	char c = str_getchar(input, pos);
	while(c == '*'){
		Spacing_n(1);
		tmp = new_Typeinfo(TPTR);
		tmp->ptr = type;
		tmp->size = PTR_SIZE;
		type = tmp;
		c = str_getchar(input, pos);
	}
	return type;
}

// declarator <- pointer? direct_declarator
static char *declarator(Typeinfo *type){
	type = pointer(type);
	char *name = direct_declarator(type);
	return name;
}

// init_declarator <- declarator
static char *init_declarator(Typeinfo *type){
	return declarator(type);
}

// init_declarator_list <- init_declarator (',' init_declarator)*
static char *init_declarator_list(Typeinfo *type){
	char *name, *tmp;
	if(!(name = init_declarator(type)))return NULL;
	char c = str_getchar(input, pos);
	while(c == ','){
		pos++;
		Spacing();
		if(!(tmp = init_declarator(type)))error("init_declarator_list");
		c = str_getchar(input, pos);
	}
	return name;
}

// declaration <- type_specifier init_declarator_list?
static bool declaration(){
	Typeinfo *type = type_specifier();
	if(!type)return false;
	init_declarator_list(type);
	char c = str_getchar(input, pos);
	if(c != ';')error("declaration:missing semicoron");
	pos++;
	Spacing();
	return true;
}

// function_definition <- type-specifier declarator compound_statement
static Def *function_definition(){
	char *c = str_pn(input, pos);
	Def *res = new_Def(FUN);
	Typeinfo *type = type_specifier();
	if(type == NULL)return NULL;
	res->name = declarator(type);
	if(!res->name)error("function_definition:missing identifier");

	res->Schild = compound_statement();
	// TODO
	//res->idcount
	res->idcount = list_map_sum(count_id_offset, idlist);
	return res;
}


void *parse(str *s){
	input = s;
	pos = 0;
	idlist = list_empty();
	Spacing();
	return function_definition();
}
