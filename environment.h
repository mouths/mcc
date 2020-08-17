#include "tool.h"

#ifndef ENVIRONMENT_h
#define ENVIRONMENT_h

typedef struct env{
	list *var_list, *type_list;
	struct env* outer;
}env;

env *new_env(env *outer);

#endif
