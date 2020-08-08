#ifndef PARSE_h
#define PARSE_h
typedef enum{NUM, ADD, SUB, MUL, DIV, MOD, PLUS, MINUS, LES, GRT, LEQ, GEQ, EQ, NEQ, AND, XOR, OR, AS, TAS, DAS, MAS, AAS, SAS, LSAS, RSAS, LAAS, LXAS, LOAS, ID} Ntype;

typedef struct Num{
	Ntype type;
	int i;
	struct Num *lhs, *rhs;
	char *name;
}Num;

typedef enum{NONE, RET, EXP, CPD, ITEM} Stype;
typedef struct Stmt{
	Stype type;
	Num *Nchild;
	struct Stmt *lhs, *rhs;
}Stmt;

void *parse(str *s);
#endif
