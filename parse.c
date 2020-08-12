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
list *gidlist;

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

typedef enum{DEC_INI, DEC_PTR, DEC_DEC, DEC_DECTOR, DEC_FUN, DEC_ARRAY, DEC_VAR, DEC_PARAM, DEC_TYPE, DEC_DEF} Dectype;

typedef struct Decs{
	Dectype type;
	struct Decs *next, *lhs, *rhs;
	char *name;
	int size;
}Decs;

static Decs *new_Decs(Dectype t){
	Decs *res = malloc(sizeof(Decs));
	res->next = NULL;
	res->type = t;
	res->name = NULL;
	res->size = 0;
	return res;
}

static struct id_container *new_container(){
	struct id_container *res = malloc(sizeof(struct id_container));
	res->name = NULL;
	res->offset = 0;
	res->type = NULL;
	res->size = 1;
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

static void print_container(void *in){
	struct id_container *tmp = in;
	fprintf(stderr, "%s:%d\n", tmp->name, tmp->offset);
}

static Num *new_Num(Ntype t){
	Num *res = malloc(sizeof(Num));
	res->type = t;
	res->lhs = res->rhs = NULL;
	res->offset = 0;
	res->name = NULL;
	res->size = 1;
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
	switch(type){
		case TINT:
			res->size = INT_SIZE;
			break;
		case TPTR:
		case TARRAY:
			res->size = PTR_SIZE;
	}
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
	struct id_container *con = in;
	if(con->type->type == TARRAY)
		return con->size * con->type->ptr->size;
	else
		return con->type->size;
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
		if(tmp){
			res->offset = tmp->offset;
			res->vtype = tmp->type;
			res->size = tmp->size;
			res->name = con->name;
			free(con);
			Spacing_n(i);
			return res;
		}
		tmp = list_search(con, id_container_cmp, gidlist);
		if(tmp){
			res->type = GVAR;
			res->name = con->name;
			res->vtype = tmp->type;
			res->size = tmp->size;
			free(con);
			Spacing_n(i);
			return res;
		}
		list_map(print_container, gidlist);
		error("identifier:undefined variable");
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

// postfix_expression <- primary_expression ('(' argument_expression_list? ')' / '[' expression ']')?
static Num *postfix_expression(){
	Num *res = primary_expression(), *tmp;
	if(!res)return res;
	char c = str_getchar(input, pos);
	if(c == '('){
		Spacing_n(1);
		res->lhs = argument_expression_list();
		c = str_getchar(input, pos);
		if(c != ')')error("postfix_expression:missing ')'");
		Spacing_n(1);
		res->type = CALL;
	}else if(c == '['){
		Spacing_n(1);
		tmp = new_Num(ADD);
		tmp->lhs = res;
		tmp->rhs = expression();
		if(tmp->lhs->vtype->type == TPTR
				|| tmp->lhs->vtype->type == TARRAY)
			tmp->vtype = tmp->lhs->vtype;
		else
			tmp->vtype = tmp->rhs->vtype;
		res = new_Num(DEREF);
		res->lhs = tmp;
		res->vtype = tmp->vtype->ptr;
		c = str_getchar(input, pos);
		if(c != ']')error("postfix_expression:missing ']'");
		Spacing_n(1);
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
			if(!(res->lhs->vtype->type == TPTR
						|| res->lhs->vtype->type == TARRAY))
				error("unary_expression:deref to non-pointer object");
			res->vtype = res->lhs->vtype->ptr;
		}else if(c == '&'){
			res->vtype = new_Typeinfo(TPTR);
			res->vtype->ptr = res->lhs->vtype;
		}
		return res;
	}else if(strncmp(str_pn(input, pos), "sizeof", 6) == 0 && keyword() >= 0){
		Spacing_n(6);
		tmp = unary_expression();
		res = new_Num(NUM);
		if(tmp->vtype->type == TARRAY)
			res->i = tmp->size * tmp->vtype->ptr->size;
		else
			res->i = tmp->vtype->size;
		res->vtype = new_Typeinfo(TINT);
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
		if(res->lhs->vtype->size >
				res->rhs->vtype->size){
			res->vtype = res->lhs->vtype;
		}else{
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
		if(res->lhs->vtype->type == TPTR ||
				res->lhs->vtype->type == TARRAY){
			res->vtype = new_Typeinfo(TPTR);
			res->vtype->ptr = res->lhs->vtype->ptr;
			//res->vtype = res->lhs->vtype;
		}else if(res->rhs->vtype->type == TPTR ||
				res->rhs->vtype->type == TARRAY){
			res->vtype = new_Typeinfo(TPTR);
			res->vtype->ptr = res->rhs->vtype->ptr;
			//res->vtype = res->rhs->vtype;
		}else if(res->lhs->vtype->size >
				res->rhs->vtype->size){
			res->vtype = res->lhs->vtype;
		}else{
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
		res->vtype = new_Typeinfo(TINT);
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
		res->vtype = new_Typeinfo(TINT);
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
		if(res->lhs->vtype->size > res->rhs->vtype->size)
			res->vtype = res->lhs->vtype;
		else
			res->vtype = res->rhs->vtype;
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
		if(res->lhs->vtype->size > res->rhs->vtype->size)
			res->vtype = res->lhs->vtype;
		else
			res->vtype = res->rhs->vtype;
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
		if(res->lhs->vtype->size > res->rhs->vtype->size)
			res->vtype = res->lhs->vtype;
		else
			res->vtype = res->rhs->vtype;
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
			res->vtype = res->lhs->vtype;
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

static void local_variable_assign(Decs *in){
	static Typeinfo *type;
	if(in->type == DEC_DEC){
		type = NULL;
		local_variable_assign(in->lhs);
		if(in->rhs)local_variable_assign(in->rhs);
	}else if(in->type == DEC_TYPE){
		type = new_Typeinfo(TINT);
	}else if(in->type == DEC_DECTOR){
		if(in->lhs)local_variable_assign(in->lhs);
		local_variable_assign(in->rhs);
		if(in->next)local_variable_assign(in->next);
	}else if(in->type == DEC_VAR || in->type == DEC_ARRAY){
		if(in->type == DEC_ARRAY){
			Typeinfo *tmp = new_Typeinfo(TARRAY);
			tmp->ptr = type;
			type = tmp;
		}
		struct id_container *con = new_container();
		con->name = in->name;
		if(list_search(con, id_container_cmp, idlist))
			error("local_variable_assign:redeclaration");
		con->type = type;
		if(in->type == DEC_ARRAY){
			con->size = in->size;
			con->offset = list_map_sum(count_id_offset, idlist) + type->ptr->size * con->size;
		}else
			con->offset = list_map_sum(count_id_offset, idlist) + type->size;
		list_append(con, idlist);
	}else if(in->type == DEC_PTR){
		Typeinfo *tmp = new_Typeinfo(TPTR);
		tmp->ptr = type;
		type = tmp;
	}else if(in->type == DEC_PARAM){
		type = NULL;
		local_variable_assign(in->lhs);
		local_variable_assign(in->rhs);
		if(in->next)local_variable_assign(in->next);
	}else{
		fprintf(stderr, "dec no:%d\n", in->type);
		error("local_variable_assign:undefined");
	}
}

static Decs *declaration();
// block_item <- statement / declaration
static Stmt *block_item(){
	Stmt *res, *tmp = NULL;
	Decs *dec = NULL;
	if((dec = declaration())){
		tmp = new_Stmt(EXP);
		local_variable_assign(dec);
	}
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
static Decs *type_specifier(){
	char *c = str_pn(input, pos);
	if(strncmp(c, "int", 3) != 0)return NULL;
	if(nondigit_n(3))return NULL;
	Decs *res = new_Decs(DEC_TYPE);
	res->name = name_copy(3);
	Spacing_n(3);
	return res;
}

// declaration_specifiers <- type_specifier
static Decs *declaration_specifiers(){
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

static Decs* declarator();
// parameter_declaration <- declaration_specifiers declarator
// DEC_PARAM
static Decs *parameter_declaration(){
	int p = pos;
	Decs *res, *tmp = declaration_specifiers();
	if(!tmp)return NULL;
	res = new_Decs(DEC_PARAM);
	res->lhs = tmp;
	res->rhs = declarator();
	if(res->rhs == NULL){
		pos = p;
		return NULL;
	}
	return res;
}

// parameter_list <- parameter_declaration (',' parameter_declaration)*
static Decs *parameter_list(){
	Decs *res = parameter_declaration(), *tmp;
	if(!res)return NULL;
	int p = pos;
	tmp = res;
	while(str_getchar(input, pos) == ','){
		tmp->next = parameter_declaration();
		tmp = tmp->next;
		if(!tmp){
			pos = p;
			return res;
		}
		p = pos;
	}
	return res;
}

// parameter_type_list <- parameter_list
static Decs *parameter_type_list(){
	return parameter_list();
}

// direct_declarator <- identifier ('(' parameter_type_list? ')' / '[' digit+ ']')?
// DEC_VAR, DEC_FUN, DEC_ARRAY
static Decs *direct_declarator(){
	int id, p = pos;
	id = is_identifier();
	if(id == 0)return NULL;
	Decs *res = new_Decs(DEC_VAR);
	res->name = name_copy(id);
	Spacing_n(id);
	p = pos;
	if(str_getchar(input, pos) == '('){
		Spacing_n(1);
		res->type = DEC_FUN;
		res->lhs = parameter_type_list();
		if(str_getchar(input, pos) != ')'){
			res->type = DEC_VAR;
			res->lhs = NULL;
			pos = p;
		}
		Spacing_n(1);
		return res;
	}else if(str_getchar(input, pos) == '['){
		Spacing_n(1);
		int i = 0;
		while(digit()){
			i = i * 10 + str_getchar(input, pos) - '0';
			pos++;
		}
		Spacing();
		if(str_getchar(input, pos) != ']'){
			pos = p;
			return res;
		}
		res->type = DEC_ARRAY;
		res->size = i;
		Spacing_n(1);
	}
	return res;
}

// pointer <- '*'+
static Decs *pointer(){
	Decs *res = NULL, *tmp;
	char c = str_getchar(input, pos);
	while(str_getchar(input, pos) == '*'){
		tmp = new_Decs(DEC_PTR);
		tmp->next = res;
		res = tmp;
		Spacing_n(1);
	}
	return res;
}

// declarator <- pointer? direct_declarator
// DEC_DECTOR
static Decs *declarator(){
	int p = pos;
	Decs *res, *lhs, *rhs;
	lhs = pointer();
	rhs = direct_declarator();
	if(rhs == NULL){
		pos = p;
		return NULL;
	}
	res = new_Decs(DEC_DECTOR);
	res->lhs = lhs; res->rhs = rhs;
	return res;
}

// init_declarator <- declarator
static Decs *init_declarator(){
	return declarator();
}

// init_declarator_list <- init_declarator (',' init_declarator)*
static Decs *init_declarator_list(){
	int p = pos;
	Decs *res = init_declarator(), *tmp;
	if(!res)return NULL;
	p = pos;
	tmp = res;
	while(str_getchar(input, pos) == ','){
		Spacing_n(1);
		tmp->next = init_declarator();
		if(tmp->next == NULL){
			pos = p;
			return res;
		}
		p = pos;
		tmp = tmp->next;
	}
	return res;
}

// declaration <- type_specifier init_declarator_list? ';'
// DEC_DEC
static Decs *declaration(){
	int p = pos;
	Decs *res = type_specifier(), *tmp;
	if(!res)return NULL;
	tmp = new_Decs(DEC_DEC);
	tmp->lhs = res;
	res = tmp;
	res->rhs = init_declarator_list();
	if(str_getchar(input, pos) != ';'){
		pos = p;
		return NULL;
	}
	Spacing_n(1);
	return res;
}

static char *assign_function(Decs *in){
	static Typeinfo *type;
	static char *name;
	if(in->type == DEC_DEF){
		type = NULL;
		assign_function(in->lhs);
		assign_function(in->rhs);
	}else if(in->type == DEC_TYPE){
		type = new_Typeinfo(TINT);
	}else if(in->type == DEC_DECTOR){
		// set pointer
		if(in->lhs)assign_function(in->lhs);
		// registratino functino name
		assign_function(in->rhs);
	}else if(in->type == DEC_FUN){
		struct id_container *con = new_container();
		con->name = in->name;
		if(list_search(con, id_container_cmp, gidlist))
			error("assign_function:redeclaration function");
		con->type = type;
		list_append(con, gidlist);
		name = con->name;
		if(in->lhs)assign_function(in->lhs);
	}else if(in->type == DEC_PARAM){
		local_variable_assign(in);
	}else{
		fprintf(stderr, "%d\n", in->type);
		error("assign_function:undefined");
	}
	return name;
}

// function_definition <- type-specifier declarator compound_statement
static Def *function_definition(){
	char *c = str_pn(input, pos);
	Def *res = new_Def(FUN);
	Decs *in = new_Decs(DEC_DEF);
	in->lhs = type_specifier();
	if(in->lhs == NULL)return NULL;
	in->rhs = declarator();
	res->name = assign_function(in);

	res->Schild = compound_statement();
	res->idcount = list_map_sum(count_id_offset, idlist);
	return res;
}

static Def *global_variable_assign(Decs *in){
	static Typeinfo *type;
	static Def *res;
	if(in->type == DEC_DEC){
		type = NULL;
		res = NULL;
		global_variable_assign(in->lhs);
		if(in->rhs)global_variable_assign(in->rhs);
	}else if(in->type == DEC_TYPE){
		type = new_Typeinfo(TINT);
	}else if(in->type == DEC_DECTOR){
		if(in->lhs)global_variable_assign(in->lhs);
		global_variable_assign(in->rhs);
	}else if(in->type == DEC_VAR){
		struct id_container *con = new_container();
		con->name = in->name;
		if(list_search(con, id_container_cmp, gidlist))
			error("global_variable_assign:redeclaration");
		con->type = type;
		list_append(con, gidlist);
		res = new_Def(GVDEF);
		res->name = in->name;
		res->idcount = type->size;
	}else{
		fprintf(stderr, "dec no:%d\n", in->type);
		error("global_variable_assign:undefined");
	}
	return res;
}

//external_declaration <- declaration / function_definition
static Def *external_declaration(){
	Def *res = NULL;
	Decs *dec = declaration();
	if(dec){
		res = global_variable_assign(dec);
		return res;
	}
	res = function_definition();
	return res;
}

// translation_unit <- external_declaration+;
static Def *translation_unit(){
	Def *res = external_declaration(), *tmp;
	if(!res)return NULL;
	tmp = res;
	while((tmp->next = external_declaration()))
		tmp = tmp->next;
	return res;
}

void *parse(str *s){
	input = s;
	pos = 0;
	idlist = list_empty();
	gidlist = list_empty();
	Spacing();
	return translation_unit();
}
