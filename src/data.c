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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "libisp/mem.h"

/* MAKE DATA OBJECTS */

lisp_data_t *lisp_make_int(const int i, lisp_ctx_t *context) {
	lisp_data_t *out;

	if(!(out = lisp_data_alloc(sizeof(lisp_data_t), context)))
		return NULL;

	out->type = lisp_type_integer;
	out->integer = i;

	return out;
}

lisp_data_t *lisp_make_decimal(const double d, lisp_ctx_t *context) {
	lisp_data_t *out;

	if(!(out = lisp_data_alloc(sizeof(lisp_data_t), context)))
		return NULL;

	out->type = lisp_type_decimal;
	out->decimal = d;

	return out;
}

lisp_data_t *lisp_make_string(const char *str, lisp_ctx_t *context) {
	lisp_data_t *out;

	if(!(out = lisp_data_alloc(sizeof(lisp_data_t), context)))
		return NULL;

	if(!(out->string = malloc(strlen(str) + 1))) {
		free(out);
		return NULL;
	}

	out->type = lisp_type_string;
	strcpy(out->string, str);

	return out;
}

lisp_data_t *lisp_make_symbol(const char *ident, lisp_ctx_t *context) {
	lisp_data_t *out;

	if(!(out = lisp_data_alloc(sizeof(lisp_data_t), context)))
		return NULL;

	if(!(out->symbol = malloc(strlen(ident) + 1))) {
		free(out);
		return NULL;
	}

	out->type = lisp_type_symbol;
	strcpy(out->symbol, ident);

	return out;
}

lisp_data_t *lisp_make_prim(lisp_prim_proc in, lisp_ctx_t *context) {
		lisp_data_t *out;

	if(!(out = lisp_data_alloc(sizeof(lisp_data_t), context)))
		return NULL;

	out->type = lisp_type_prim;
	out->proc = in;

	return out;
}

lisp_data_t *lisp_make_error(const char *errmsg, lisp_ctx_t *context) {
	lisp_data_t *out;

	if(!(out = lisp_data_alloc(sizeof(lisp_data_t), context)))
		return NULL;

	if(!(out->error = malloc(strlen(errmsg) + 1))) {
		free(out);
		return NULL;
	}

	out->type = lisp_type_error;
	strcpy(out->error, errmsg);

	return out;
}

/* LIST MANIPULATION */

lisp_data_t *lisp_cons_in_context(const lisp_data_t *l, const lisp_data_t *r, lisp_ctx_t *context) {
	lisp_data_t *out;

	if(!(out = lisp_data_alloc(sizeof(lisp_data_t), context)))
		return NULL;

	if(!(out->pair = malloc(sizeof(lisp_cons_t)))) {
		free(out);
		return NULL;
	}

	out->type = lisp_type_pair;
	out->pair->l = (lisp_data_t*)l;
	out->pair->r = (lisp_data_t*)r;

	return out;
}

lisp_data_t *lisp_car(const lisp_data_t *in) {
	if(!in)
		return NULL;

	if(in->type != lisp_type_pair)
		return NULL;

	return in->pair->l;
}

lisp_data_t *lisp_cdr(const lisp_data_t *in) {
	if(!in)
		return NULL;

	if(in->type != lisp_type_pair)
		return NULL;

	return in->pair->r;
}

int lisp_is_equal(const lisp_data_t *d1, const lisp_data_t *d2) {
	if(d1 == d2)
		return 1;

	if(!d1)
		return 0;
	if(!d2)
		return 0;

	if(d1->type != d2->type)
		return 0;

	switch(d1->type) {
		case lisp_type_pair:
			return lisp_is_equal(lisp_car(d1), lisp_car(d2)) && lisp_is_equal(lisp_cdr(d1), lisp_cdr(d2));
		case lisp_type_integer:
			return d1->integer == d2->integer;
		case lisp_type_decimal:
			return d1->decimal == d2->decimal;
		case lisp_type_prim:
			return d1->proc == d2->proc;
		case lisp_type_string:
			return !strcmp(d1->string, d2->string);
		case lisp_type_error:
			return 0;
		case lisp_type_symbol:			
			return !strcmp(d1->symbol, d2->symbol);
	}

	return 0;
}

int lisp_list_length(const lisp_data_t *list) {
	int out = 0;
	
	if(!list)
		return 0;
	
	if(list->type != lisp_type_pair)
		return 0;

	do {
		out++;
		if(list->type == lisp_type_pair)
			list = list->pair->r;
		else
			list = NULL;
	} while(list);

	return out;
}

lisp_data_t *lisp_set_car(lisp_data_t *in, const lisp_data_t *val) {
	if(in->type != lisp_type_pair)
		return NULL;
	in->pair->l = (lisp_data_t*)val;
	return (lisp_data_t*)val;
}

lisp_data_t *lisp_set_cdr(lisp_data_t *in, const lisp_data_t *val) {
	if(in->type != lisp_type_pair)
		return NULL;
	in->pair->r = (lisp_data_t*)val;
	return (lisp_data_t*)val;
}

lisp_data_t *lisp_make_copy(const lisp_data_t *in) {
	lisp_data_t *out;

	if(!in)
		return NULL;

	out = malloc(sizeof(lisp_data_t));
	if(!out)
		return NULL;

	out->type = in->type;

	switch(out->type) {
		case lisp_type_integer: out->integer = in->integer; break;
		case lisp_type_decimal: out->decimal = in->decimal; break;
		case lisp_type_prim: out->proc = in->proc; break;
		case lisp_type_string: 
			out->string = malloc(strlen(in->string) + 1);
			strcpy(out->string, in->string);
			break;
		case lisp_type_symbol:
			out->symbol = malloc(strlen(in->symbol) + 1);
			strcpy(out->symbol, in->symbol);
			break;
		case lisp_type_error:
			out->error = malloc(strlen(in->error) + 1);
			strcpy(out->error, in->error);
			break;
		case lisp_type_pair:
			out->pair = malloc(sizeof(lisp_cons_t));
			if(!out)
				return NULL;
			if(in->pair->l)
				out->pair->l = lisp_make_copy(in->pair->l);
			else
				out->pair->l = NULL;

			if(in->pair->r)
				out->pair->r = lisp_make_copy(in->pair->r);
			else
				out->pair->r = NULL;
	}

	return out;
}

lisp_data_t *lisp_append(const lisp_data_t *list1, const lisp_data_t *list2) {
	lisp_data_t *out, *buf;

	if(!list1) {
		if(!list2)
			return NULL;
		if(list2->type != lisp_type_pair)
			return NULL;
		return lisp_make_copy(list2);
	}

	if(list1->type != lisp_type_pair)
		return NULL;

	if(!list2)
		return lisp_make_copy(list1);

	buf = out = lisp_make_copy(list1);

	while(lisp_cdr(out))
		out = lisp_cdr(out);
	
	lisp_set_cdr(out, lisp_make_copy(list2));

	return buf;
}
