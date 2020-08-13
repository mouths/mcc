#ifndef TOOL_h
#define TOOL_h

struct list_body {
	void *con;
	struct list_body *next;
};

typedef struct list {
	int len;
	struct list_body *body;
} list;

list *list_empty();
list *list_new(void *con);
int list_len(list *lst);
list *list_append(void *con, list *lst);
list *list_concat(list *a, list *b);
void list_map(void (operation) (void *), list *lst);
int list_map_sum(int (*operation) (void *), list *lst);
void *list_search(void * con, int (*cmp) (const void *, const void *), list *lst);
int list_search_n(void *con, int (*cmp) (const void*, const void *), list *lst);
void *list_getn(int n, list *lst);
#endif
