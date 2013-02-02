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

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "data.h"
#include "eval.h"
#include "mem.h"
#include "thread.h"

prim_proc_list *the_prim_procs = NULL;
prim_proc_list *last_prim_proc = NULL;

data_t *prim_add(const data_t *list) {
	int iout = 0;
	double dout = 0.0f;
	data_t *head, *tail;

	if(!list)
		return make_symbol("error");

	if(list->type != pair)
		return make_int(0);
	
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
		return make_int(iout);
	}
	
	if((dout - iout) == floor(dout - iout))
		return make_int((int)dout - iout);

	return make_decimal(dout + iout);
}

data_t *prim_mul(const data_t *list) {
	int iout = 1;
	double dout = 1.0f;
	data_t *head, *tail;

	if(!list)
		return make_symbol("error");

	if(list->type != pair)
		return make_int(0);
	
	do {
		head = car(list);
		tail = cdr(list);
		if(head->type == integer)
			iout *= head->val.integer;
		else if(head->type == decimal)
			dout *= head->val.decimal;
		else return make_int(0);

		list = tail;
	} while(list);

	if(dout == 1.0f) {
		return make_int(iout);
	}

	if((dout - iout) == floor(dout - iout))
		return make_int((int)dout * iout);
	
	return make_decimal(dout * iout);
}

data_t *prim_sub(const data_t *list) {
	dtype_t out_type;
	int iout = 0, istart;
	double dout = 0.0f, dstart;
	data_t *head, *tail;

	if(!list)
		return make_symbol("error");
	
	if(list->type != pair)
		return make_int(0);
	head = car(list);
	tail = cdr(list);

	out_type = head->type;
	if(out_type == decimal)
		dstart = head->val.decimal;
	else if(out_type == integer)
		istart = head->val.integer;
	else
		return make_int(0);

	list = tail;

	if(!list) {
		if(out_type == integer) {
			return make_int(-istart);
		} else {
			return make_decimal(-dstart);
		}
	}

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
		return make_int(istart - iout);
	}
	
	return make_decimal(dstart - dout - iout);
}

data_t *prim_div(const data_t *list) {
	dtype_t start_type;
	double dout = 1.0f, dstart;
	data_t *head, *tail;

	if(!list)
		return make_symbol("error");
	
	if(list->type != pair)
		return make_int(0);
	head = car(list);
	tail = cdr(list);

	start_type = head->type;
	if(start_type == decimal)
		dstart = head->val.decimal;
	else if(start_type == integer)
		dstart = (double)head->val.integer;
	else
		return make_int(0);

	list = tail;

	if(!list)
		return make_decimal(1 / dstart);

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
		return make_int((int)(dstart / dout));

	return make_decimal(dstart / dout);
}

data_t *prim_comp_eq(const data_t *list) {
	data_t *first, *second;
	dtype_t type_first, type_second;

	if(!list)
		return make_symbol("error");
	
	if(list->type != pair)
		return make_symbol("#f");

	first = car(list);
	second = cdr(list);

	if(second->type != pair)
		return make_symbol("error");
	second = car(second);

	type_first = first->type;
	type_second = second->type;

	if((type_first != decimal) && (type_first != integer))
		return make_symbol("#f");
	
	if((type_second != decimal) && (type_second != integer))
		return make_symbol("#f");

	if(type_first == integer)
		if(first->val.integer == second->val.integer)
			return make_symbol("#t");

	if(type_first == decimal)
		if(first->val.decimal == second->val.decimal)
			return make_symbol("#t");

	return make_symbol("#f");
}

data_t *prim_comp_less(const data_t *list) {
	data_t *head, *tail;
	
	if(list && list->type == pair)
		head = car(list);
	else
		return make_symbol("error");

	list = cdr(list);

	if(list && list->type == pair)
		tail = car(list);
	else
		return make_symbol("#f");

	if((head->type == integer) && (tail->type == integer)) {
		if(head->val.integer < tail->val.integer) {
			return make_symbol("#t");
		} else {
			return make_symbol("#f");
		}
	} else if((head->type == decimal) && (tail->type == integer)) {
		if(head->val.decimal < tail->val.integer) {
			return make_symbol("#t");
		} else {
			return make_symbol("#f");
		}
	} else if((head->type == integer) && (tail->type == decimal)) {
		if(head->val.integer < tail->val.decimal) {
			return make_symbol("#t");
		} else {
			return make_symbol("#f");
		}
	} else if((head->type == decimal) && (tail->type == decimal)) {
		if(head->val.decimal < tail->val.decimal) {
			return make_symbol("#t");
		} else {
			return make_symbol("#f");
		}
	}

	return make_symbol("#f");

}

data_t *prim_comp_more(const data_t *list) {
	data_t *head, *tail;
	
	if(list && list->type == pair)
		head = car(list);
	else
		return make_symbol("#f");

	list = cdr(list);

	if(list && list->type == pair)
		tail = car(list);
	else
		return make_symbol("#f");

	if((head->type == integer) && (tail->type == integer)) {
		if(head->val.integer > tail->val.integer) {
			return make_symbol("#t");
		} else {
			return make_symbol("#f");
		}
	} else if((head->type == decimal) && (tail->type == integer)) {
		if(head->val.decimal > tail->val.integer) {
			return make_symbol("#t");
		} else {
			return make_symbol("#f");
		}
	} else if((head->type == integer) && (tail->type == decimal)) {
		if(head->val.integer > tail->val.decimal) {
			return make_symbol("#t");
		} else {
			return make_symbol("#f");
		}
	} else if((head->type == decimal) && (tail->type == decimal)) {
		if(head->val.decimal > tail->val.decimal) {
			return make_symbol("#t");
		} else {
			return make_symbol("#f");
		}
	}

	return make_symbol("#f");

}

data_t *prim_or(const data_t *list) {
	if(!list)
		return make_symbol("#f");
	
	if(list->type != pair)
		return make_symbol("#f");

	while(list) {
		if(is_equal(car(list), make_symbol("#t")))
			return make_symbol("#t");
		list = cdr(list);
	}
	return make_symbol("#f");
}

data_t *prim_and(const data_t *list) {
	if(!list)
		return make_symbol("#f");

	if(list->type != pair)
		return make_symbol("#f");

	while(list) {
		if(is_equal(car(list), make_symbol("#f")))
			return make_symbol("#f");
		list = cdr(list);
	}
	return make_symbol("#t");
}

data_t *prim_floor(const data_t *list) {
	if(!list)
		return make_symbol("error");

	if(list->type != pair)
		return make_symbol("error");

	list = car(list);

	if(list->type == integer)
		return make_int(list->val.integer);

	if(list->type == decimal)
		return make_int((int)floor(list->val.decimal));

	return make_symbol("error");
}

data_t *prim_ceiling(const data_t *list) {
	if(!list)
		return make_symbol("error");
	
	if(list->type != pair)
		return make_symbol("error");

	list = car(list);

	if(list->type == integer)
		return make_int(list->val.integer);

	if(list->type == decimal)
		return make_int((int)ceil(list->val.decimal));

	return make_symbol("error");
}

data_t *prim_trunc(const data_t *list) {
	double num;

	if(!list)
		return make_symbol("error");
	
	if(list->type != pair)
		return make_symbol("error");

	list = car(list);

	if(list->type == integer)
		return make_int(list->val.integer);

	if(list->type == decimal) {
		num = list->val.decimal;

		if(num < 0)
			return make_int((int)ceil(list->val.decimal));
		return make_int((int)floor(list->val.decimal));
	}

	return make_symbol("error");
}

data_t *prim_round(const data_t *list) {
	double num, fracpart;
	int intpart;

	if(!list)
		return make_symbol("error");
	
	if(list->type != pair)
		return make_symbol("error");

	list = car(list);

	if(list->type == integer)
		return make_int(list->val.integer);

	if(list->type == decimal) {
		num = list->val.decimal;
		fracpart = num - floor(num);
		if(fracpart < .5)
			return make_int((int)(num - fracpart));
		if(fracpart > .5)
			return make_int((int)(num - fracpart + 1));
		intpart = (int)(num - fracpart);
		if(intpart % 2)
			return make_int(intpart + 1);
		return make_int(intpart);
	}

	return make_symbol("error");
}

data_t *prim_max(const data_t *list) {
	int ival, imax = 0;
	double dval, dmax = 0.0f;
	data_t *val;

	if(!list)
		return make_symbol("error");
	
	if(list->type != pair)
		return make_symbol("error");

	while(list) {
		if(list->type != pair)
			return make_symbol("error");
		val = car(list);
		if(val->type == integer) {
			ival = val->val.integer;
			if(ival > imax)
				imax = ival;
		} else if(val->type == decimal) {
			dval = val->val.decimal;
			if(dval > dmax)
				dmax = dval;
		}
		list = cdr(list);
	}

	if((double)imax > dmax)
		return make_int(imax);
	return make_decimal(dmax);
}

data_t *prim_min(const data_t *list) {
	int ival, imin = INT_MAX;
	double dval, dmin = DBL_MAX;
	data_t *val;

	if(!list)
		return make_symbol("error");

	if(list->type != pair)
		return make_symbol("error");

	while(list) {
		if(list->type != pair)
			return make_symbol("error");
		val = car(list);
		if(val->type == integer) {
			ival = val->val.integer;
			if(ival < imin)
				imin = ival;
		} else if(val->type == decimal) {
			dval = val->val.decimal;
			if(dval < dmin)
				dmin = dval;
		}
		list = cdr(list);
	}

	if((double)imin < dmin)
		return make_int(imin);
	return make_decimal(dmin);
}

data_t *prim_eq(const data_t *list) {
	data_t *first, *second;
	int ret;

	if(!list)
		return make_symbol("error");

	if(list->type != pair)
		return make_symbol("#f");

	first = car(list);
	second = cdr(list);

	if(second->type != pair)
		return make_symbol("error");
	second = car(second);

	ret = is_equal(first, second);

	if(ret)
		return make_symbol("#t");
	return make_symbol("#f");
}

data_t *prim_not(const data_t *list) {
	if(list && list->type == pair)
		list = car(list);
	else
		return make_symbol("#f");

	if(list && list->type == symbol)
		if(!strcmp(list->val.symbol, "#f"))
			return make_symbol("#t");
	return make_symbol("#f");
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
		return make_symbol("error");

	list = cdr(list);

	if(list && list->type == pair)
		tail = car(list);
	else
		return make_symbol("error");

	return cons(head, tail);
}

data_t *prim_list(const data_t *list) {
	if(!list)
		return NULL;
	return cons(car(list), prim_list(cdr(list)));
}

data_t *prim_set_car(const data_t *list) {
	data_t *head, *newcar;
	
	if(!list)
		return NULL;

	if(list->type != pair)
		return NULL;

	head = car(list);
	list = cdr(list);

	if(!list)
		return head;

	if(list->type != pair)
		return head;

	newcar = car(list);

	if(!head)
		return NULL;

	if(head->type != pair)
		return NULL;

	head->val.pair->l = newcar;

	return head;
}

data_t *prim_set_cdr(const data_t *list) {
	data_t *head, *newcdr;
	
	if(!list)
		return NULL;

	if(list->type != pair)
		return NULL;

	head = car(list);
	list = cdr(list);

	if(!list)
		return head;

	if(list->type != pair)
		return head;

	newcdr = car(list);

	if(!head)
		return NULL;

	if(head->type != pair)
		return NULL;

	head->val.pair->r = newcdr;

	return head;
}

data_t *prim_sym_to_str(const data_t *list) {
	data_t *sym;

	if(!list)
		return make_symbol("error");

	if(list->type != pair)
		return make_symbol("error");
	sym = car(list);

	if(sym->type != symbol)
		return make_symbol("error");

	return make_string(sym->val.symbol);
}

data_t *prim_str_to_sym(const data_t *list) {
	data_t *str;

	if(!list)
		return make_symbol("error");

	if(list->type != pair)
		return make_symbol("error");
	str = car(list);

	if(str->type != string)
		return make_symbol("error");

	return make_symbol(str->val.string);
}

data_t *prim_is_sym(const data_t *list) {
	data_t *sym;
	if(!list)
		return make_symbol("#f");

	if(list->type != pair)
		return make_symbol("#f");

	sym = car(list);
	if(sym->type == symbol)
		return make_symbol("#t");
	return make_symbol("#f");
}

data_t *prim_is_str(const data_t *list) {
	data_t *str;

	if(!list)
		return make_symbol("#f");

	if(list->type != pair)
		return make_symbol("#f");

	str = car(list);
	if(str->type == string)
		return make_symbol("#t");
	return make_symbol("#f");
}

data_t *prim_is_pair(const data_t *list) {
	data_t *cons;

	if(!list)
		return make_symbol("#f");

	if(list->type != pair)
		return make_symbol("#f");

	cons = car(list);
	if(cons->type == pair)
		return make_symbol("#t");
	return make_symbol("#f");
}

data_t *prim_is_num(const data_t *list) {
	data_t *head;
	dtype_t type;
	
	if(!list)
		return make_symbol("#f");

	if(list->type != pair)
		return make_symbol("#f");

	head = car(list);
	if(!head)
		return make_symbol("#f");

	type = head->type;
	if((type == integer) || (type == decimal))
		return make_symbol("#t");
	return make_symbol("#f");
}

data_t *prim_is_int(const data_t *list) {
	data_t *head;

	if(!list)
		return make_symbol("#f");
	
	if(list->type != pair)
		return make_symbol("#f");
	
	head = car(list);
	if(!head)
		return make_symbol("#f");

	if(head->type == integer)
		return make_symbol("#t");
	return make_symbol("#f");
}

data_t *prim_is_proc(const data_t *list) {
	if(!list)
		return make_symbol("#f");

	if(list->type != pair)
		return make_symbol("#f");

	list = car(list);
	if(!list)
		return make_symbol("#f");
	if(list->type != pair)
		return make_symbol("#f");

	list = car(list);
	if(!list)
		return make_symbol("#f");
	if(list->type != symbol)
		return make_symbol("#f");
	if((!strcmp(list->val.symbol, "closure")) || (!strcmp(list->val.symbol, "primitive")))
		return make_symbol("#t");
	return make_symbol("#f");
}

data_t *prim_set_config(const data_t *list) {
	data_t *var, *val;
	char *var_name;
	int value;

	if(!list)
		return make_symbol("error");

	if(list->type != pair)
		return make_symbol("error");

	if(!list)
		return make_symbol("No variable supplied");
	if(list->type != pair)
		return make_symbol("error");
	var = car(list);

	list = cdr(list);
	if(!list)
		return make_symbol("No value supplied");
	if(list->type != pair)
		return make_symbol("error");
	val = car(list);

	if(var->type != symbol)
		return make_symbol("Config variable needs to be a symbol");
	var_name = var->val.symbol;

	if(val->type != integer)
		return make_symbol("Config value needs to be an integer");
	value = val->val.integer;
	
	if(!strcmp(var_name, "mem_lim_soft")) {
		mem_lim_soft = value;
		return make_symbol("ok");
	} else if(!strcmp(var_name, "mem_lim_hard")) {
		mem_lim_hard = value;
		return make_symbol("ok");
	} else if(!strcmp(var_name, "thread_timeout")) {
		thread_timeout = val->val.integer;
		return make_symbol("ok");
	}
	return make_symbol("Unknown config variable");
}

static data_t *primitive_procedure_names(void) {
	prim_proc_list *curr_proc = last_prim_proc;
	data_t *out = NULL;

	while(curr_proc) {
		out = cons(make_symbol(curr_proc->name), out);
		curr_proc = curr_proc->prev;
	}

	return out;
}

static data_t *primitive_procedure_objects(void) {
	prim_proc_list *curr_proc = last_prim_proc;
	data_t *out = NULL;

	while(curr_proc) {
		out = cons(cons(make_symbol("primitive"), cons(make_primitive(curr_proc->proc), NULL)), out);
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

void setup_environment(void) {
	data_t *the_empty_environment = cons(cons(NULL, NULL), NULL);
	
	add_prim_proc("+", prim_add);
	add_prim_proc("*", prim_mul);
	add_prim_proc("-", prim_sub);
	add_prim_proc("/", prim_div);
	add_prim_proc("=", prim_comp_eq);
	add_prim_proc("<", prim_comp_less);
	add_prim_proc(">", prim_comp_more);
	add_prim_proc("or", prim_or);
	add_prim_proc("and", prim_and);
	add_prim_proc("not", prim_not);
	add_prim_proc("floor", prim_floor);
	add_prim_proc("ceiling", prim_ceiling);
	add_prim_proc("truncate", prim_trunc);
	add_prim_proc("round", prim_round);
	add_prim_proc("max", prim_max);
	add_prim_proc("min", prim_min);
	add_prim_proc("eq?", prim_eq);
	add_prim_proc("car", prim_car);
	add_prim_proc("cdr", prim_cdr);
	add_prim_proc("set-car!", prim_set_car);
	add_prim_proc("set-cdr!", prim_set_cdr);
	add_prim_proc("cons", prim_cons);
	add_prim_proc("list", prim_list);
	add_prim_proc("number?", prim_is_num);
	add_prim_proc("real?", prim_is_num);
	add_prim_proc("integer?", prim_is_int);
	add_prim_proc("procedure?", prim_is_proc);
	add_prim_proc("set-config!", prim_set_config);
	add_prim_proc("symbol->string", prim_sym_to_str);
	add_prim_proc("string->symbol", prim_str_to_sym);
	add_prim_proc("symbol?", prim_is_sym);
	add_prim_proc("string?", prim_is_str);
	add_prim_proc("pair?", prim_is_pair);

	the_global_env = extend_environment(primitive_procedure_names(), 
										primitive_procedure_objects(),
										the_empty_environment);

	run_exp("(define (caar pair) (car (car pair)))");
	run_exp("(define (cadr pair) (car (cdr pair)))");
	run_exp("(define (cdar pair) (cdr (car pair)))");
	run_exp("(define (cddr pair) (cdr (cdr pair)))");

	run_exp("(define (caaar pair) (car (car (car pair))))");
	run_exp("(define (caadr pair) (car (car (cdr pair))))");
	run_exp("(define (cadar pair) (car (cdr (car pair))))");
	run_exp("(define (caddr pair) (car (cdr (cdr pair))))");
	run_exp("(define (cdaar pair) (cdr (car (car pair))))");
	run_exp("(define (cdadr pair) (cdr (car (cdr pair))))");
	run_exp("(define (cddar pair) (cdr (cdr (car pair))))");
	run_exp("(define (cdddr pair) (cdr (cdr (cdr pair))))");

	run_exp("(define (caaaar pair) (car (car (car (car pair)))))");
	run_exp("(define (caaadr pair) (car (car (car (cdr pair)))))");
	run_exp("(define (caadar pair) (car (car (cdr (car pair)))))");
	run_exp("(define (caaddr pair) (car (car (cdr (cdr pair)))))");
	run_exp("(define (cadaar pair) (car (cdr (car (car pair)))))");
	run_exp("(define (cadadr pair) (car (cdr (car (cdr pair)))))");
	run_exp("(define (caddar pair) (car (cdr (cdr (car pair)))))");
	run_exp("(define (cadddr pair) (car (cdr (cdr (cdr pair)))))");
	run_exp("(define (cdaaar pair) (cdr (car (car (car pair)))))");
	run_exp("(define (cdaadr pair) (cdr (car (car (cdr pair)))))");
	run_exp("(define (cdadar pair) (cdr (car (cdr (car pair)))))");
	run_exp("(define (cdaddr pair) (cdr (car (cdr (cdr pair)))))");
	run_exp("(define (cddaar pair) (cdr (cdr (car (car pair)))))");
	run_exp("(define (cddadr pair) (cdr (cdr (car (cdr pair)))))");
	run_exp("(define (cdddar pair) (cdr (cdr (cdr (car pair)))))");
	run_exp("(define (cddddr pair) (cdr (cdr (cdr (cdr pair)))))");

	run_exp("(define nil '())");
	run_exp("(define (zero? exp) (= 0 exp))");
	run_exp("(define (null? exp) (eq? exp nil))");
	run_exp("(define (negative? exp) (< exp 0))");
	run_exp("(define (positive? exp) (> exp 0))");
	run_exp("(define (boolean? exp) (or (eq? exp '#t) (eq? exp '#f)))");
	run_exp("(define (abs n) (if (negative? n) (- 0 n) n))");
	run_exp("(define (<= a b) (not (> a b)))");
	run_exp("(define (>= a b) (not (< a b)))");
	run_exp("(define (map proc items) (if (null? items) nil (cons (proc (car items)) (map proc (cdr items)))))");
	run_exp("(define (fact n) (if (= n 1) 1 (* n (fact (- n 1)))))");
	run_exp("(define (delay proc) (lambda () proc))");
	run_exp("(define (force proc) (proc))");
	run_exp("(define (length list) (define (list-loop part count) (if (null? part) count (list-loop (cdr part) (+ count 1)))) (list-loop list 0))");
	run_exp("(define (modulo num div) (- num (* (floor (/ num div)) div)))");
	run_exp("(define (quotient num div) (truncate (/ num div)))");
	run_exp("(define (remainder num div) (+ (* (quotient num div) div -1) num))");
	run_exp("(define (gcd a b) (cond ((= a 0) b) ((= b 0) a) ((> a b) (gcd (modulo a b) b)) (else (gcd a (modulo b a)))))");
	run_exp("(define (lcm a b) (/ (* a b) (gcd a b)))");
	run_exp("(define (odd? n) (if (= 1 (modulo n 2)) '#t '#f))");
	run_exp("(define (even? n) (not (odd? n)))");
	
	run_gc(GC_FORCE);
}

void cleanup_lisp(void) {
	prim_proc_list *current = the_prim_procs, *buf;

	run_gc(GC_FORCE);
	free_data_rec(the_global_env);

	while(current) {
		buf = current->next;
		free(current->name);
		free(current);

		current = buf;
	}
}
