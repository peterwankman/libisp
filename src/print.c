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

#include "libisp/data.h"
#include "libisp/eval.h"

static void print_data_rec(const data_t *d, int print_parens, lisp_ctx_t *context) {
	data_t *head, *tail;

	if(!d)
		printf("()");
	else if(d == context->the_global_environment)
		printf("<env>");
	else {
		switch(d->type) {
			case prim_procedure: printf("<proc>"); break;
			case integer: printf("%d", d->integer); break;
			case decimal: printf("%g", d->decimal); break;
			case symbol: printf("%s", d->symbol); break;
			case string: printf("\"%s\"", d->string); break;
			case error: printf("ERROR: '%s'", d->error); break;
			case pair:
				if(is_compound_procedure(d)) {
					printf("<proc>");
					break;
				}

				if(print_parens)
					printf("(");

				head = car(d);
				tail = cdr(d);

				if(tail) {
					print_data_rec(head, 1, context);
					if(tail->type != pair) {
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

void print_data(const data_t *d, lisp_ctx_t *context) {
	print_data_rec(d, 1, context);
}
