/*
 * libisp -- Lisp evaluator based on SICP
 * (C) 2013 Martin Wolters
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libisp.h"

static char *getline(FILE *fp) {
	size_t size = 0, len  = 0, last = 0;
	char *buf  = NULL;

	do {
		size += BUFSIZ;
		buf = (char*)realloc(buf, size);
		fgets(buf + last, size, fp);
		len = strlen(buf);
		last = len - 1;
	} while (!feof(fp) && buf[last]!='\n');
	buf[last] = '\0';

	return buf;
}

int main(void) {
	data_t *exp_list, *ret;
	size_t readto;
	int error;
	char *exp;

	the_global_env = setup_environment();

	printf("HAVE YOU READ YOUR SICP TODAY?\n\n");

	while(1) {
		readto = 0;
		printf("-> ");

		exp = getline(stdin);
		if(!strcmp(exp, "(quit)"))
			break;

		do {
			exp_list = lisp_read(exp, &readto, &error);
		
			if(error) {
				printf("Syntax Error: '%s'\n", exp);
			} else {
				ret = eval(exp_list, the_global_env);
				printf("== ");
				lisp_print_data(ret);
				printf("\n");
			}	
		
			exp += readto;
			run_gc();
		} while(strlen(exp));
	}
	cleanup_lisp();
	showmemstats(stdout);
	return 0;
}
