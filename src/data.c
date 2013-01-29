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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

/* MAKE DATA OBJECTS */

data_t *lisp_make_int(const int i) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t))))
		return NULL;

	out->type = integer;
	out->val.integer = i;

	return out;
}

data_t *lisp_make_decimal(const double d) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t))))
		return NULL;

	out->type = decimal;
	out->val.decimal = d;

	return out;
}

data_t *lisp_make_string(const char *str) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t))))
		return NULL;

	if(!(out->val.string = (char*)malloc(strlen(str) + 1))) {
		free(out);
		return NULL;
	}

	out->type = string;
	strcpy(out->val.string, str);

	return out;
}

data_t *lisp_make_symbol(const char *ident) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t))))
		return NULL;

	if(!(out->val.symbol = (char*)malloc(strlen(ident) + 1))) {
		free(out);
		return NULL;
	}

	out->type = symbol;
	strcpy(out->val.symbol, ident);

	return out;
}

data_t *lisp_make_primitive(prim_proc in) {
		data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t))))
		return NULL;

	out->type = prim_procedure;
	out->val.proc = in;

	return out;
}

/* LIST MANIPULATION */

data_t *cons(const data_t *l, const data_t *r) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t))))
		return NULL;

	if(!(out->val.pair = (cons_t*)malloc(sizeof(cons_t)))) {
		free(out);
		return NULL;
	}

	out->type = pair;
	out->val.pair->l = (data_t*)l;
	out->val.pair->r = (data_t*)r;

	return out;
}

static data_t *list_va(va_list ap) {
	data_t *item;

	item = va_arg(ap, data_t*);	

	if(item == NULL)
		return NULL;

	return cons(item, list_va(ap));
}

data_t *list(const data_t *in, ...) {
	data_t *out;
	va_list ap;

	va_start(ap, in);
	out = cons(in, list_va(ap));
	va_end(ap);

	return out;
}

data_t *car(const data_t *in) {
	if(!in)
		return NULL;

	if(in->type != pair)
		return NULL;

	return in->val.pair->l;
}

data_t *cdr(const data_t *in) {
	if(!in)
		return NULL;

	if(in->type != pair)
		return NULL;

	return in->val.pair->r;
}

int is_equal(const data_t *d1, const data_t *d2) {
	if(d1 == d2)
		return 1;

	if(!d1)
		return 0;
	if(!d2)
		return 0;

	if(d1->type != d2->type)
		return 0;

	switch(d1->type) {
		case pair:
			return is_equal(car(d1), car(d2)) && is_equal(cdr(d1), cdr(d2));
		case integer:
			return d1->val.integer == d2->val.integer;
		case decimal:
			return d1->val.decimal == d2->val.decimal;
		case prim_procedure:
			return d1->val.proc == d2->val.proc;
		case string:
			return !strcmp(d1->val.string, d2->val.string);
		case symbol:			
			return !strcmp(d1->val.symbol, d2->val.symbol);
	}

	return 0;
}

int length(const data_t *list) {
	int out = 0;
	
	if(!list)
		return 0;
	
	if(list->type != pair)
		return 0;

	if(car(list) == NULL)
		return 0;

	do {
		out++;
		if(list->type == pair)
			list = list->val.pair->r;
		else
			list = NULL;
	} while(list);

	return out;
}

data_t *set_car(data_t *in, const data_t *val) {
	if(in->type != pair)
		return NULL;
	in->val.pair->l = (data_t*)val;
	return (data_t*)val;
}

data_t *set_cdr(data_t *in, const data_t *val) {
	if(in->type != pair)
		return NULL;
	in->val.pair->r = (data_t*)val;
	return (data_t*)val;
}
