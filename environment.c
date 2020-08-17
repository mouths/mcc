#include <stdlib.h>

#include "tool.h"
#include "environment.h"

env *new_env(env *outer){
	env *res = malloc(sizeof(env));
	res->var_list = list_empty();
	res->type_list = list_empty();
	res->outer = outer;
}
