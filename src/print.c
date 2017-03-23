/*
 * libisp -- Lisp evaluator based on SICP
 * (C) 2013-2017 Martin Wolters
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#include <stdio.h>

#include "libisp/data.h"
#include "libisp/eval.h"

static void print_data_rec(const lisp_data_t *d, int print_parens, lisp_ctx_t *context) {
	lisp_data_t *head, *tail;

	if(!d)
		printf("()");
	else if(d == context->the_global_environment)
		printf("<env>");
	else {
		switch(d->type) {
			case lisp_type_prim: printf("<proc>"); break;
			case lisp_type_integer: printf("%d", d->integer); break;
			case lisp_type_decimal: printf("%g", d->decimal); break;
			case lisp_type_symbol: printf("%s", d->symbol); break;
			case lisp_type_string: printf("\"%s\"", d->string); break;
			case lisp_type_error: printf("ERROR: '%s'", d->error); break;
			case lisp_type_pair:
				if(is_compound_procedure(d)) {
					printf("<proc>");
					break;
				}

				if(print_parens)
					printf("(");

				head = lisp_car(d);
				tail = lisp_cdr(d);

				if(tail) {
					print_data_rec(head, 1, context);
					if(tail->type != lisp_type_pair) {
						printf(" . ");
						print_data_rec(tail, 1, context);
					} else {
						printf(" ");
						print_data_rec(tail, 0, context);
					}
				} else {
					print_data_rec(head, 1, context);
				}

				if(print_parens)
					printf(")");
		}
	}
}

void lisp_print(const lisp_data_t *d, lisp_ctx_t *context) {
	print_data_rec(d, 1, context);
}
