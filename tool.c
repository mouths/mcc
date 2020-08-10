#include <stdlib.h>

#include "tool.h"

#define BODY struct list_body

list *list_empty(){
	list *res = malloc(sizeof(list));
	res->len = 0;
	res->body = NULL;
	return res;
}

list *list_new(void *con){
	list *res = list_empty();
	BODY *tmp = malloc(sizeof(BODY));
	tmp->con = con;
	tmp->next = NULL;
	res->body = tmp;
	return res;
}

int list_len(list *lst){
	return lst->len;
}

list *list_append(void *con, list *lst){
	BODY *tmp = malloc(sizeof(BODY));
	tmp->next = NULL;
	tmp->con = con;
	lst->len++;
	if(lst->body == NULL){
		lst->body = tmp;
		return lst;
	}
	BODY *i;
	for(i = lst->body; i->next; i= i->next);
	i->next = tmp;
	return lst;
}

list *list_concat(list *a, list *b){
	if(a->body == NULL){
		a->len = b->len;
		a->body = b->body;
		free(b);
		return a;
	}
	BODY *i;
	for(BODY *i = a->body; i->next; i = i->next);
	a->len += b->len;
	i->next = b->body;
	free(b);
	return a;
}

void list_map(void (*operation) (void *), list *lst){
	for(BODY *i = lst->body; i; i = i->next)operation(i->con);
}

int list_map_sum(int (*operation) (void *), list *lst){
	int sum = 0;
	for(BODY *i = lst->body; i; i = i->next)sum += operation(i->con);
	return sum;
}

void *list_search(void * con, int (*cmp) (const void *, const void *), list *lst){
	for(BODY *i = lst->body; i; i = i->next)
		if(cmp(con, i->con) == 0)return i->con;
	return NULL;
}

void *list_getn(int n, list *lst){
	int c = 0;
	for(BODY *i = lst->body; i; i = i->next)
		if(c++ == n)return i->con;
	return NULL;
}
