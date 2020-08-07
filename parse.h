#ifndef PARSE_h
#define PARSE_h
typedef enum{NUM, ADD, SUB} Ntype;

typedef struct Num{
	Ntype type;
	int i;
	struct Num *lhs, *rhs;
}Num;

void *parse(str *s);
#endif
