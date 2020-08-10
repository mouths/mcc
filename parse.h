#ifndef PARSE_h
#define PARSE_h

#define INT_SIZE 4
#define PTR_SIZE 8

typedef enum{NUM, ADD, SUB, MUL, DIV, MOD, PLUS, MINUS, LES, GRT, LEQ, GEQ, EQ, NEQ, AND, XOR, OR, AS, MULAS, DIVAS, MODAS, ADDAS, SUBAS, LSAS, RSAS, LAAS, LXAS, LOAS, ID, CALL, PTR, DEREF, ARG} Ntype;

typedef struct Num{
	Ntype type;
	int i;
	struct Num *lhs, *rhs;
	int id;
	char *name;
	int ptr;
	int size;
}Num;

typedef enum{NONE, RET, EXP, CPD, ITEM} Stype;

typedef struct Stmt{
	Stype type;
	Num *Nchild;
	struct Stmt *lhs, *rhs;
}Stmt;

typedef enum{FUN} Dtype;

typedef struct Def{
	Dtype type;
	Stmt *Schild;
	char *name;
	int idcount;
}Def;

void *parse(str *s);
#endif
