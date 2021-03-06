#include "tool.h"
#include "str.h"
#include "environment.h"

#ifndef PARSE_h
#define PARSE_h

#define INT_SIZE 4
#define PTR_SIZE 8
#define CHAR_SIZE 1

typedef enum{NUM, ADD, SUB, MUL, DIV, MOD, PLUS, MINUS, LES, GRT, LEQ, GEQ, EQ, NEQ, AND, XOR, OR, AS, MULAS, DIVAS, MODAS, ADDAS, SUBAS, LSAS, RSAS, LAAS, LXAS, LOAS, ID, CALL, PTR, DEREF, ARG, GVAR, STR, LAND, LOR, COND} Ntype;

typedef struct Num{
	Ntype type;
	int i;
	struct Num *lhs, *rhs, *center;
	int offset;
	char *name;
	//int ptr;
	int size;
	struct Typeinfo *vtype;
}Num;

typedef enum{NONE, RET, EXP, CPD, ITEM, IF, WHILE, FOR} Stype;

typedef struct Stmt{
	Stype type;
	Num *Nchild, *init, *iteration;
	struct Stmt *lhs, *rhs;
	int count;
}Stmt;

typedef enum{FUN, GVDEF} Dtype;

typedef struct Def{
	Dtype type;
	Stmt *Schild;
	char *name;
	int idcount;
	list *arguments;
	struct Def *next;
	list *strlist;
	env *env;
}Def;

typedef enum{TINT, TCHAR, TPTR, TARRAY, TFUNC} Type;

typedef struct Typeinfo{
	Type type;
	struct Typeinfo *ptr;
	list *args;
	int size;
} Typeinfo;

void *parse(str *s);
#endif
