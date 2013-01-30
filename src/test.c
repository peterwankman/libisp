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

void print_banner(void) {
	printf(" '-._                  ___.....___\n");
	printf("     `.__           ,-'        ,-.`-,\n");
	printf("         `''-------'          ( p )  `._\n");
	printf("                               `-'      (        Have you read your SICP today?\n");
	printf("                                         \\\n");
	printf("                                .         \\\n");
	printf("                                 \\\\---..,--'\n");
	printf("         .............._           --...--,\n");
	printf("                        `-.._         _.-'\n");
	printf("                             `'-----''                     Type (quit) to quit.\n\n");
}

static char *get_line(FILE *fp) {
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
	size_t readto, reclaimed;
	int error;
	char *exp, *buf;

	mem_verbosity = GC_SILENT;

	printf("Setting up the global environment...\n\n");
	setup_environment();

	print_banner();

	while(1) {
		readto = 0;
		printf("HIBT> ");

		exp = get_line(stdin);
		if(!strcmp(exp, "(quit)")) {
			free(exp);
			break;
		}
		buf = exp;

		do {
			exp_list = lisp_read(exp, &readto, &error);
		
			if(error) {
				printf("Syntax Error: '%s'\n", exp);
				break;
			} else {
				ret = eval(exp_list, the_global_env);
				printf("YHBT: ");
				lisp_print_data(ret);
				printf("\n");
			}	
		
			exp += readto;

			if((mem_verbosity == GC_VERBOSE) && (reclaimed = run_gc(GC_LOWMEM)))
				printf("GC: %d bytes of memory reclaimed.\n", reclaimed);
		} while(strlen(exp));
		free(buf);
	}
	cleanup_lisp();
	showmemstats(stdout);
	return 0;
}
