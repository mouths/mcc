#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

#define BUF_SIZE 1000

str *str_empty(){
	str *res = malloc(sizeof(str));
	res->len = 0;
	*(res->body = malloc(sizeof(char))) = '\0';
	return res;
}

str *str_convert(char *in){
	str *res = malloc(sizeof(str));
	res->len = strlen(in);
	res->body = malloc(sizeof(char) * (res->len + 1));
	strcpy(res->body, in);
	return res;
}

str *str_con(str *a, str *b){
	char *tmp = a->body;
	a->body = malloc(sizeof(char) * (a->len + b->len + 1));
	sprintf(a->body, "%s%s", tmp, b->body);
	a->len += b->len;
	free(tmp);
	free(b->body);
	free(b);
	return a;
}

str *str_append(str *a, char *in){
	size_t len = strlen(in);
	char *tmp = a->body;
	a->body = malloc(sizeof(char) * (a->len + len + 1));
	a->len += len;
	sprintf(a->body, "%s%s", tmp, in);
	free(tmp);
	return a;
}

char str_getchar(str *a, int n){
	if(a->len <= n)return '\0';
	return *(a->body + n);
}

char *str_p(str *a){
	return a->body;
}

char *str_pn(str *a, int n){
	if(a->len < n)return NULL;
	return a->body + n;
}

str *str_file_convert(FILE *f){
	char buf[BUF_SIZE + 5];
	str *res = str_empty();
	while(fgets(buf, BUF_SIZE, f))
		str_append(res, buf);
	return res;
}
