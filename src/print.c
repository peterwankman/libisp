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

#include "data.h"

extern data_t *the_global_env;

static void lisp_print_data_rec(const data_t *d) {
	data_t *head, *tail;

	if(!d)
		printf("()");
	else if(d == the_global_env)
		printf("<env>");
	else {
		switch(d->type) {
			case prim_procedure: printf("<proc>"); break;
			case integer: printf("%d", d->val.integer); break;
			case decimal: printf("%f", d->val.decimal); break;
			case symbol: printf("%s", d->val.symbol); break;
			case string: printf("\"%s\"", d->val.string); break;
			case pair:
				head = car(d);
				tail = cdr(d);
				if(!head && !tail) {
					printf("()");
				} else {
					if(head) {
	 					if(head->type == pair)
							printf("(");
						lisp_print_data_rec(head);
						if(head->type == pair)
							printf(")");
					}

					if(tail) {
						if(tail->type != pair)
						printf(" . ");
						else
							printf(" ");

						lisp_print_data_rec(tail);
					}					
				}
				break;
		}
	}
}

void lisp_print_data(const data_t *d) {
	if(!d) {
		printf("()");
	} else {
		if(d->type == pair)
			printf("(");
		lisp_print_data_rec(d);
		if(d->type == pair)
			printf(")");
	}
}