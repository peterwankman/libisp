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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "data.h"
#include "eval.h"
#include "mem.h"

prim_proc_list *the_prim_procs = NULL;
prim_proc_list *last_prim_proc = NULL;

data_t *prim_add(const data_t *list) {
	int iout = 0;
	double dout = 0.0f;
	data_t *head, *tail;

	if(list->type != pair)
		return lisp_make_int(0);
	
	do {
		head = car(list);
		tail = cdr(list);
		if(head->type == integer)
			iout += head->val.integer;
		else if(head->type == decimal)
			dout += head->val.decimal;
		else return 0;

		list = tail;
	} while(list);

	if(dout == 0.0f) {
		return lisp_make_int(iout);
	}
	
	if((dout - iout) == floor(dout - iout))
		return lisp_make_int((int)dout - iout);

	return lisp_make_decimal(dout + iout);
}

data_t *prim_mul(const data_t *list) {
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

data_t *prim_sub(const data_t *list) {
	dtype_t out_type;
	int iout = 0, istart;
	double dout = 0.0f, dstart;
	data_t *head, *tail;

	if(list->type != pair)
		return lisp_make_int(0);
	head = car(list);
	tail = cdr(list);

	out_type = head->type;
	if(out_type == decimal)
		dstart = head->val.decimal;
	else if(out_type == integer)
		istart = head->val.integer;
	else
		return lisp_make_int(0);

	list = tail;

	if(!list)
		if(out_type == integer)
			return lisp_make_int(-istart);
		else
			return lisp_make_decimal(-dstart);

	do {
		head = car(list);
		tail = cdr(list);
		if(head->type == integer)
			iout += head->val.integer;
		else if(head->type == decimal) {
			out_type = decimal;
			dstart = (double)istart;
			dout += head->val.decimal;
		}
		else return 0;

		list = tail;
	} while(list);

	if(out_type == integer) {
		return lisp_make_int(istart - iout);
	}
	
	return lisp_make_decimal(dstart - dout - iout);
}

data_t *prim_div(const data_t *list) {
	dtype_t start_type;
	double dout = 1.0f, dstart;
	data_t *head, *tail;

	if(list->type != pair)
		return lisp_make_int(0);
	head = car(list);
	tail = cdr(list);

	start_type = head->type;
	if(start_type == decimal)
		dstart = head->val.decimal;
	else if(start_type == integer)
		dstart = (double)head->val.integer;
	else
		return lisp_make_int(0);

	list = tail;

	if(!list)
		return lisp_make_decimal(1 / dstart);

	do {
		head = car(list);
		tail = cdr(list);
		if(head->type == integer)
			dout *= head->val.integer;
		else if(head->type == decimal)
			dout *= head->val.decimal;
		else return 0;

		list = tail;
	} while(list);
	
	if(dstart / dout == floor(dstart / dout))
		return lisp_make_int((int)(dstart / dout));

	return lisp_make_decimal(dstart / dout);
}

data_t *prim_comp_eq(const data_t *list) {
	data_t *first, *second;
	dtype_t type_first, type_second;

	if(list->type != pair)
		return lisp_make_symbol("#f");

	first = car(list);
	second = cdr(list);

	if(second->type != pair)
		return lisp_make_symbol("error");
	second = car(second);

	type_first = first->type;
	type_second = second->type;

	if((type_first != decimal) && (type_first != integer))
		return lisp_make_symbol("#f");
	
	if((type_second != decimal) && (type_second != integer))
		return lisp_make_symbol("#f");

	if(type_first == integer)
		if(first->val.integer == second->val.integer)
			return lisp_make_symbol("#t");

	if(type_first == decimal)
		if(first->val.decimal == second->val.decimal)
			return lisp_make_symbol("#t");

	return lisp_make_symbol("#f");
}

data_t *prim_comp_less(const data_t *list) {
	data_t *head, *tail;
	
	if(list && list->type == pair)
		head = car(list);
	else
		return lisp_make_symbol("#f");

	list = cdr(list);

	if(list && list->type == pair)
		tail = car(list);
	else
		return lisp_make_symbol("#f");

	if((head->type == integer) && (tail->type == integer)) {
		if(head->val.integer < tail->val.integer) {
			return lisp_make_symbol("#t");
		} else {
			return lisp_make_symbol("#f");
		}
	} else if((head->type == decimal) && (tail->type == integer)) {
		if(head->val.decimal < tail->val.integer) {
			return lisp_make_symbol("#t");
		} else {
			return lisp_make_symbol("#f");
		}
	} else if((head->type == integer) && (tail->type == decimal)) {
		if(head->val.integer < tail->val.decimal) {
			return lisp_make_symbol("#t");
		} else {
			return lisp_make_symbol("#f");
		}
	} else if((head->type == decimal) && (tail->type == decimal)) {
		if(head->val.decimal < tail->val.decimal) {
			return lisp_make_symbol("#t");
		} else {
			return lisp_make_symbol("#f");
		}
	}

	return lisp_make_symbol("#f");

}

data_t *prim_comp_more(const data_t *list) {
	data_t *head, *tail;
	
	if(list && list->type == pair)
		head = car(list);
	else
		return lisp_make_symbol("#f");

	list = cdr(list);

	if(list && list->type == pair)
		tail = car(list);
	else
		return lisp_make_symbol("#f");

	if((head->type == integer) && (tail->type == integer)) {
		if(head->val.integer > tail->val.integer) {
			return lisp_make_symbol("#t");
		} else {
			return lisp_make_symbol("#f");
		}
	} else if((head->type == decimal) && (tail->type == integer)) {
		if(head->val.decimal > tail->val.integer) {
			return lisp_make_symbol("#t");
		} else {
			return lisp_make_symbol("#f");
		}
	} else if((head->type == integer) && (tail->type == decimal)) {
		if(head->val.integer > tail->val.decimal) {
			return lisp_make_symbol("#t");
		} else {
			return lisp_make_symbol("#f");
		}
	} else if((head->type == decimal) && (tail->type == decimal)) {
		if(head->val.decimal > tail->val.decimal) {
			return lisp_make_symbol("#t");
		} else {
			return lisp_make_symbol("#f");
		}
	}

	return lisp_make_symbol("#f");

}

data_t *prim_eq(const data_t *list) {
	data_t *first, *second;
	int ret;

	if(list->type != pair)
		return lisp_make_symbol("#f");

	first = car(list);
	second = cdr(list);

	if(second->type != pair)
		return lisp_make_symbol("error");
	second = car(second);

	ret = is_equal(first, second);

	if(ret)
		return lisp_make_symbol("#t");
	return lisp_make_symbol("#f");
}

data_t *prim_not(const data_t *list) {
	if(list && list->type == pair)
		list = car(list);
	else
		return lisp_make_symbol("#f");

	if(list && list->type == symbol)
		if(!strcmp(list->val.symbol, "#f"))
			return lisp_make_symbol("#t");
	return lisp_make_symbol("#f");
}

data_t *prim_car(const data_t *list) {
	if(list && list->type == pair)
		list = car(list);
	else
		return NULL;

	if(list && list->type == pair)
		return car(list);
	return NULL;
}

data_t *prim_cdr(const data_t *list) {
	if(list && list->type == pair)
		list = car(list);
	else
		return NULL;
	if(list && list->type == pair)
		return cdr(list);
	return NULL;
}

data_t *prim_cons(const data_t *list) {
	data_t *head, *tail;

	if(list && list->type == pair)
		head = car(list);
	else
		return lisp_make_symbol("#f");

	list = cdr(list);

	if(list && list->type == pair)
		tail = car(list);
	else
		return lisp_make_symbol("#f");

	return cons(head, tail);
}

static data_t *primitive_procedure_names(void) {
	prim_proc_list *curr_proc = last_prim_proc;
	data_t *out = NULL;

	while(curr_proc) {
		out = cons(lisp_make_symbol(curr_proc->name), out);
		curr_proc = curr_proc->prev;
	}

	return out;
}

static data_t *primitive_procedure_objects(void) {
	prim_proc_list *curr_proc = last_prim_proc;
	data_t *out = NULL;

	while(curr_proc) {
		out = cons(list(
			lisp_make_symbol("primitive"),
			lisp_make_primitive(curr_proc->proc), NULL), out);
		curr_proc = curr_proc->prev;
	}

	return out;
}

void add_prim_proc(char *name, prim_proc proc) {
	prim_proc_list *curr_proc;

	if(last_prim_proc == NULL) {
		the_prim_procs = (prim_proc_list*)malloc(sizeof(prim_proc_list));
		the_prim_procs->name = (char*)malloc(strlen(name) + 1);
		strcpy(the_prim_procs->name, name);
		the_prim_procs->proc = proc;
		the_prim_procs->next = NULL;
		the_prim_procs->prev = NULL;
		last_prim_proc = the_prim_procs;
		return;
	}
	
	curr_proc = (prim_proc_list*)malloc(sizeof(prim_proc_list));
	curr_proc->name = (char*)malloc(strlen(name) + 1);
	strcpy(curr_proc->name, name);
	curr_proc->proc = proc;
	curr_proc->prev = last_prim_proc;
	curr_proc->next = NULL;

	last_prim_proc->next = curr_proc;
	last_prim_proc = curr_proc;
}

data_t *setup_environment(void) {
	data_t *initial_env, *the_empty_environment;
	the_empty_environment = cons(cons(NULL, NULL), NULL);
	
	add_prim_proc("+", prim_add);
	add_prim_proc("*", prim_mul);
	add_prim_proc("-", prim_sub);
	add_prim_proc("/", prim_div);
	add_prim_proc("=", prim_comp_eq);
	add_prim_proc("<", prim_comp_less);
	add_prim_proc("<", prim_comp_more);
	add_prim_proc("not", prim_not);
	add_prim_proc("eq?", prim_eq);
	add_prim_proc("car", prim_car);
	add_prim_proc("cdr", prim_cdr);
	add_prim_proc("cons", prim_cons);

	initial_env = extend_environment(primitive_procedure_names(), 
									 primitive_procedure_objects(),
									 the_empty_environment);

	return initial_env;
}

void cleanup_lisp(void) {
	prim_proc_list *current = the_prim_procs, *buf;

	free_data_rec(the_global_env);

	while(current) {
		buf = current->next;
		free(current->name);
		free(current);

		current = buf;
	}
}
