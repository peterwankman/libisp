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

#include <math.h>

#include "libisp.h"

data_t *user_mul(const data_t *list) {
	int iout = 1;
	double dout = 1.0f;
	data_t *head, *tail;

	if(list->type != pair)
		return lisp_make_int(0);
	
	do {
		head = car(list);
		tail = cdr(list);
		if(head->type == integer)
			iout *= head->val.integer;
		else if(head->type == decimal)
			dout *= head->val.decimal;
		else return lisp_make_int(0);

		list = tail;
	} while(list);

	if(dout == 1.0f) {
		return lisp_make_int(iout);
	}

	if((dout - iout) == floor(dout - iout))
		return lisp_make_int((int)dout * iout);
	
	return lisp_make_decimal(dout * iout);
}

int main(void) {
	data_t *a, *ret;
	size_t readto;
	int error;

	char *exp = "(define (f n) (if (= n 1) 1 (** n (f (- n 1))))) (f 5)";

	add_prim_proc("**", user_mul);
	the_global_env = setup_environment();

	printf("HAVE YOU READ YOUR SICP TODAY?\n\n");

	do {
		readto = 0;
		a = lisp_read(exp, &readto, &error);
		printf("-> ");
		lisp_print_data(a);
		printf("\n");
		ret = eval(a, the_global_env);
		printf("== ");
		lisp_print_data(ret);
		printf("\n");

		if(a)
			free_data(a);
		else if(error) {
			printf("Syntax Error: '%s'\n", exp);
			break;
		}
				
		exp += readto;
		run_gc();
	} while(strlen(exp));

	cleanup_lisp();
	showmemstats(stdout);
	return 0;
}
