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

#include "libisp/mem.h"

/* MAKE DATA OBJECTS */

data_t *make_int(const int i, lisp_ctx_t *context) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t), context)))
		return NULL;

	out->type = integer;
	out->integer = i;

	return out;
}

data_t *make_decimal(const double d, lisp_ctx_t *context) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t), context)))
		return NULL;

	out->type = decimal;
	out->decimal = d;

	return out;
}

data_t *make_string(const char *str, lisp_ctx_t *context) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t), context)))
		return NULL;

	if(!(out->string = malloc(strlen(str) + 1))) {
		free(out);
		return NULL;
	}

	out->type = string;
	strcpy(out->string, str);

	return out;
}

data_t *make_symbol(const char *ident, lisp_ctx_t *context) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t), context)))
		return NULL;

	if(!(out->symbol = malloc(strlen(ident) + 1))) {
		free(out);
		return NULL;
	}

	out->type = symbol;
	strcpy(out->symbol, ident);

	return out;
}

data_t *make_primitive(prim_proc in, lisp_ctx_t *context) {
		data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t), context)))
		return NULL;

	out->type = prim_procedure;
	out->proc = in;

	return out;
}

data_t *make_error(const char *errmsg, lisp_ctx_t *context) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t), context)))
		return NULL;

	if(!(out->error = malloc(strlen(errmsg) + 1))) {
		free(out);
		return NULL;
	}

	out->type = error;
	strcpy(out->error, errmsg);

	return out;
}

/* LIST MANIPULATION */

data_t *cons_in_context(const data_t *l, const data_t *r, lisp_ctx_t *context) {
	data_t *out;

	if(!(out = lisp_data_alloc(sizeof(data_t), context)))
		return NULL;

	if(!(out->pair = malloc(sizeof(cons_t)))) {
		free(out);
		return NULL;
	}

	out->type = pair;
	out->pair->l = (data_t*)l;
	out->pair->r = (data_t*)r;

	return out;
}

data_t *car(const data_t *in) {
	if(!in)
		return NULL;

	if(in->type != pair)
		return NULL;

	return in->pair->l;
}

data_t *cdr(const data_t *in) {
	if(!in)
		return NULL;

	if(in->type != pair)
		return NULL;

	return in->pair->r;
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
			return d1->integer == d2->integer;
		case decimal:
			return d1->decimal == d2->decimal;
		case prim_procedure:
			return d1->proc == d2->proc;
		case string:
			return !strcmp(d1->string, d2->string);
		case error:
			return 0;
		case symbol:			
			return !strcmp(d1->symbol, d2->symbol);
	}

	return 0;
}

int length(const data_t *list) {
	int out = 0;
	
	if(!list)
		return 0;
	
	if(list->type != pair)
		return 0;

	do {
		out++;
		if(list->type == pair)
			list = list->pair->r;
		else
			list = NULL;
	} while(list);

	return out;
}

data_t *set_car(data_t *in, const data_t *val) {
	if(in->type != pair)
		return NULL;
	in->pair->l = (data_t*)val;
	return (data_t*)val;
}

data_t *set_cdr(data_t *in, const data_t *val) {
	if(in->type != pair)
		return NULL;
	in->pair->r = (data_t*)val;
	return (data_t*)val;
}

data_t *make_copy(const data_t *in) {
	data_t *out;

	if(!in)
		return NULL;

	out = malloc(sizeof(data_t));
	if(!out)
		return NULL;

	out->type = in->type;

	switch(out->type) {
		case integer: out->integer = in->integer; break;
		case decimal: out->decimal = in->decimal; break;
		case prim_procedure: out->proc = in->proc; break;
		case string: 
			out->string = malloc(strlen(in->string) + 1);
			strcpy(out->string, in->string);
			break;
		case symbol:
			out->symbol = malloc(strlen(in->symbol) + 1);
			strcpy(out->symbol, in->symbol);
			break;
		case error:
			out->error = malloc(strlen(in->error) + 1);
			strcpy(out->error, in->error);
			break;
		case pair:
			out->pair = malloc(sizeof(cons_t));
			if(!out)
				return NULL;
			if(in->pair->l)
				out->pair->l = make_copy(in->pair->l);
			else
				out->pair->l = NULL;

			if(in->pair->r)
				out->pair->r = make_copy(in->pair->r);
			else
				out->pair->r = NULL;
	}

	return out;
}

data_t *append(const data_t *list1, const data_t *list2) {
	data_t *out, *buf;

	if(!list1) {
		if(!list2)
			return NULL;
		if(list2->type != pair)
			return NULL;
		return make_copy(list2);
	}

	if(list1->type != pair)
		return NULL;

	if(!list2)
		return make_copy(list1);

	buf = out = make_copy(list1);

	while(cdr(out))
		out = cdr(out);
	
	set_cdr(out, make_copy(list2));

	return buf;
}
