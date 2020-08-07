#ifndef STR_h
#define STR_h
typedef struct str{
	size_t len;
	char *body;
}str;

str *str_empty();
str *str_convert(char *in);
str *str_con(str *a, str *b);
str *str_append(str *a, char *in);
char str_getchar(str *a, int n);
char *str_p(str *a);
char *str_pn(str *a, int n);
str *str_file_convert(FILE *f);
#endif
