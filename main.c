#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "parse.h"
#include "generator.h"

int main(int argc, char **argv){
	char *fname;
	FILE *f;
	char buf[1005];
	if(argc < 2){
		f = stdin;
	}else{
		fname = argv[1];

		if((f = fopen(fname, "r")) == NULL){
			fprintf(stderr, "fopen error:%s\n", fname);
			exit(1);
		}
	}

	str *input = str_file_convert(f);
	generator(parse(input));

	if(stdin != f){
		fclose(f);
	}
	return 0;
}
