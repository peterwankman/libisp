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
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "libisp/builtin.h"
#include "libisp/data.h"
#include "libisp/eval.h"
#include "libisp/mem.h"
#include "libisp/thread.h"

data_t *prim_add(const data_t *list, lisp_ctx_t *context) {
	int iout = 0;
	double dout = 0.0f;
	data_t *head, *tail;

	while(list) {
		if((head = car(list)) == NULL)
			return make_error("+ -- Expected number", context);
		tail = cdr(list);

		if(head->type == integer)
			iout += head->integer;
		else if(head->type == decimal)
			dout += head->decimal;
		else return make_error("+ -- Expected number", context);

		list = tail;
	}

	if(dout == 0.0f)
		return make_int(iout, context);
	
	if((dout + iout) == floor(dout + iout))
		return make_int((int)dout + iout, context);

	return make_decimal(dout + iout, context);
}

data_t *prim_mul(const data_t *list, lisp_ctx_t *context) {
	int iout = 1;
	double dout = 1.0f;
	data_t *head, *tail;

	while(list) {
		if((head = car(list)) == NULL)
			return make_error("* -- Expected number", context);
		tail = cdr(list);
		if(head->type == integer)
			iout *= head->integer;
		else if(head->type == decimal)
			dout *= head->decimal;
		else return make_error("* -- Expected number", context);

		list = tail;
	}

	if(dout == 1.0f)
		return make_int(iout, context);

	if((dout * iout) == floor(dout * iout))
		return make_int((int)dout * iout, context);
	
	return make_decimal(dout * iout, context);
}

data_t *prim_sub(const data_t *list, lisp_ctx_t *context) {
	dtype_t out_type;
	int iout = 0, istart;
	double dout = 0.0f, dstart;
	data_t *head, *tail;

	if(!length(list))
		return make_error("- -- No operands", context);
	if((head = car(list)) == NULL)
		return make_error("- -- Expected number", context);

	tail = cdr(list);
	out_type = head->type;
	if(out_type == decimal)
		dstart = head->decimal;
	else if(out_type == integer)
		istart = head->integer;
	else
		return make_error("- -- Expected number", context);

	list = tail;

	if(!list) {
		if(out_type == integer) {
			return make_int(-istart, context);
		} else {
			return make_decimal(-dstart, context);
		}
	}

	do {
		if((head = car(list)) == NULL)
			return make_error("- -- Expected number", context);
		tail = cdr(list);
		if(head->type == integer)
			iout += head->integer;
		else if(head->type == decimal) {
			if(out_type == integer) {
				out_type = decimal;
				dstart = (double)istart;
			}
			dout += head->decimal;
		}
		else return make_error("- -- Expected number", context);

		list = tail;
	} while(list);

	if(out_type == integer)
		return make_int(istart - iout, context);
	
	return make_decimal(dstart - dout - iout, context);
}

data_t *prim_div(const data_t *list, lisp_ctx_t *context) {
	dtype_t start_type;
	double dout = 1.0f, dstart;
	data_t *head, *tail;

	if(!length(list))
		return make_error("/ -- No operands", context);
	if((head = car(list)) == NULL)
		return make_error("/ -- Expected number", context);

	tail = cdr(list);
	start_type = head->type;
	if(start_type == decimal)
		dstart = head->decimal;
	else if(start_type == integer)
		dstart = (double)head->integer;
	else
		return make_error("/ -- Expected number", context);

	list = tail;

	if(!list)
		return make_decimal(1 / dstart, context);

	do {
		if((head = car(list)) == NULL)
			return make_error("/ -- Expected number", context);
		tail = cdr(list);

		if(head->type == integer)
			dout *= head->integer;
		else if(head->type == decimal)
			dout *= head->decimal;
		else return 0;

		list = tail;
	} while(list);

	if(dout == 0)
		return make_error("/ -- Division by zero", context);
	
	if(dstart / dout == floor(dstart / dout))
		return make_int((int)(dstart / dout), context);

	return make_decimal(dstart / dout, context);
}

data_t *prim_comp_eq(const data_t *list, lisp_ctx_t *context) {
	data_t *first, *second;
	dtype_t type_first, type_second;

	if(length(list) != 2)
		return make_error("= -- Expected two operands", context);
	if((first = car(list)) == NULL)
		return make_error("= -- Expected number", context);
	if((second = cdr(list)) == NULL)
		return make_error("= -- Expected pair", context);
	if(second->type != pair)
		return make_error("= -- Expected pair", context);
	if((second = car(second)) == NULL)
		return make_error("= -- Expected number", context);

	type_first = first->type;
	type_second = second->type;

	if((type_first != decimal) && (type_first != integer))
		return make_error("= -- Expected number", context);
	if((type_second != decimal) && (type_second != integer))
		return make_error("= -- Expected number", context);

	if(type_first == integer)
		if(first->integer == second->integer)
			return make_symbol("#t", context);

	if(type_first == decimal)
		if(first->decimal == second->decimal)
			return make_symbol("#t", context);

	return make_symbol("#f", context);
}

data_t *prim_comp_less(const data_t *list, lisp_ctx_t *context) {
	data_t *head, *tail;

	if(length(list) != 2)
		return make_error("< -- Expected two operands", context);
	if((head = car(list)) == NULL)
		return make_error("< -- Expected number", context);
	if((tail = car(cdr(list))) == NULL)
		return make_error("< -- Expected number", context);
		
	if((head->type == integer) && (tail->type == integer)) {
		if(head->integer < tail->integer) {
			return make_symbol("#t", context);
		} else {
			return make_symbol("#f", context);
		}
	} else if((head->type == decimal) && (tail->type == integer)) {
		if(head->decimal < tail->integer) {
			return make_symbol("#t", context);
		} else {
			return make_symbol("#f", context);
		}
	} else if((head->type == integer) && (tail->type == decimal)) {
		if(head->integer < tail->decimal) {
			return make_symbol("#t", context);
		} else {
			return make_symbol("#f", context);
		}
	} else if((head->type == decimal) && (tail->type == decimal)) {
		if(head->decimal < tail->decimal) {
			return make_symbol("#t", context);
		} else {
			return make_symbol("#f", context);
		}
	}

	return make_error("< -- Invalid comparison", context);
}

data_t *prim_comp_more(const data_t *list, lisp_ctx_t *context) {
	data_t *head, *tail;

	if(length(list) != 2)
		return make_error("> -- Expected two operands", context);
	if((head = car(list)) == NULL)
		return make_error("> -- Expected number", context);
	if((tail = car(cdr(list))) == NULL)
		return make_error("> -- Expected number", context);

	if((head->type == integer) && (tail->type == integer)) {
		if(head->integer > tail->integer) {
			return make_symbol("#t", context);
		} else {
			return make_symbol("#f", context);
		}
	} else if((head->type == decimal) && (tail->type == integer)) {
		if(head->decimal > tail->integer) {
			return make_symbol("#t", context);
		} else {
			return make_symbol("#f", context);
		}
	} else if((head->type == integer) && (tail->type == decimal)) {
		if(head->integer > tail->decimal) {
			return make_symbol("#t", context);
		} else {
			return make_symbol("#f", context);
		}
	} else if((head->type == decimal) && (tail->type == decimal)) {
		if(head->decimal > tail->decimal) {
			return make_symbol("#t", context);
		} else {
			return make_symbol("#f", context);
		}
	}

	return make_error("> -- Invalid comparison", context);
}

data_t *prim_or(const data_t *list, lisp_ctx_t *context) {
	while(list) {
		if(is_equal(car(list), make_symbol("#t", context)))
			return make_symbol("#t", context);
		list = cdr(list);
	}
	return make_symbol("#f", context);
}

data_t *prim_and(const data_t *list, lisp_ctx_t *context) {
	while(list) {
		if(is_equal(car(list), make_symbol("#f", context)))
			return make_symbol("#f", context);
		list = cdr(list);
	}
	return make_symbol("#t", context);
}

data_t *prim_floor(const data_t *list, lisp_ctx_t *context) {
	if(length(list) != 1)
		return make_error("FLOOR -- Expected one operand", context);
	if((list = car(list)) == NULL)
		return make_error("FLOOR -- Expected number", context);
		
	if(list->type == integer)
		return make_int(list->integer, context);

	if(list->type == decimal)
		return make_int((int)floor(list->decimal), context);

	return make_error("FLOOR -- Expected number", context);
}

data_t *prim_ceiling(const data_t *list, lisp_ctx_t *context) {
	if(length(list) != 1)
		return make_error("CEILING -- Expected one operand", context);
	if((list = car(list)) == NULL)
		return make_error("CEILING -- Expected number", context);

	if(list->type == integer)
		return make_int(list->integer, context);

	if(list->type == decimal)
		return make_int((int)ceil(list->decimal), context);

	return make_error("CEILING -- Invalid comparison", context);
}

data_t *prim_trunc(const data_t *list, lisp_ctx_t *context) {
	double num;

	if(length(list) != 1)
		return make_error("TRUNCATE -- Expected one operand", context);
	if((list = car(list)) == NULL)
		return make_error("TRUNCATE -- Expected number", context);
		
	if(list->type == integer)
		return make_int(list->integer, context);

	if(list->type == decimal) {
		num = list->decimal;

		if(num < 0)
			return make_int((int)ceil(list->decimal), context);
		return make_int((int)floor(list->decimal), context);
	}

	return make_error("TRUNCATE -- Expected number", context);
}

data_t *prim_round(const data_t *list, lisp_ctx_t *context) {
	double num, fracpart;
	int intpart;

	if(length(list) != 1)
		return make_error("ROUND -- Expected one operand", context);
	if((list = car(list)) == NULL)
		return make_error("ROUND -- Expected number", context);

	if(list->type == integer)
		return make_int(list->integer, context);

	if(list->type == decimal) {
		num = list->decimal;
		fracpart = num - floor(num);
		if(fracpart < .5)
			return make_int((int)(num - fracpart), context);
		if(fracpart > .5)
			return make_int((int)(num - fracpart + 1), context);
		intpart = (int)(num - fracpart);
		if(intpart % 2)
			return make_int(intpart + 1, context);
		return make_int(intpart, context);
	}

	return make_error("ROUND -- Expected number", context);
}

data_t *prim_max(const data_t *list, lisp_ctx_t *context) {
	int ival, imax = 0;
	double dval, dmax = 0.0f;
	data_t *val;

	if(!length(list))
		return make_error("MAX -- No operands", context);

	while(list) {
		if(list->type != pair)
			return make_error("MAX -- Expected pair", context);
		val = car(list);
		if(val->type == integer) {
			ival = val->integer;
			if(ival > imax)
				imax = ival;
		} else if(val->type == decimal) {
			dval = val->decimal;
			if(dval > dmax)
				dmax = dval;
		} else
			return make_error("MAX -- Expected number", context);
		list = cdr(list);
	}

	if((double)imax > dmax)
		return make_int(imax, context);
	return make_decimal(dmax, context);
}

data_t *prim_min(const data_t *list, lisp_ctx_t *context) {
	int ival, imin = INT_MAX;
	double dval, dmin = DBL_MAX;
	data_t *val;

	if(!length(list))
		return make_error("MIN -- No operands", context);

	while(list) {
		if(list->type != pair)
			return make_error("MIN -- Expected pair", context);
		val = car(list);
		if(val->type == integer) {
			ival = val->integer;
			if(ival < imin)
				imin = ival;
		} else if(val->type == decimal) {
			dval = val->decimal;
			if(dval < dmin)
				dmin = dval;
		}
		list = cdr(list);
	}

	if((double)imin < dmin)
		return make_int(imin, context);
	return make_decimal(dmin, context);
}

data_t *prim_eq(const data_t *list, lisp_ctx_t *context) {
	data_t *first, *second;

	if(length(list) != 2)
		return make_error("EQ? -- No operands", context);

	first = car(list);
	second = car(cdr(list));
	
	if(is_equal(first, second))
		return make_symbol("#t", context);
	return make_symbol("#f", context);
}

data_t *prim_not(const data_t *list, lisp_ctx_t *context) {
	if(length(list) != 1)
		return make_error("NOT -- Expected one operand", context);
	if((list = car(list)) == NULL)
		return make_error("NOT -- Expected boolean", context);
	
	if(!strcmp(list->symbol, "#f"))
		return make_symbol("#t", context);
	return make_symbol("#f", context);
}

data_t *prim_car(const data_t *list, lisp_ctx_t *context) {
	if(length(list) != 1)
		return make_error("CAR -- Expected one operand", context);
	
	list = car(list);
	
	if(list && list->type == pair)
		return car(list);
	return NULL;
}

data_t *prim_cdr(const data_t *list, lisp_ctx_t *context) {
	if(length(list) != 1)
		return make_error("CDR -- Expected one operand", context);
		
	list = car(list);
	
	if(list && list->type == pair)
		return cdr(list);
	return NULL;
}

data_t *prim_cons(const data_t *list, lisp_ctx_t *context) {
	if(length(list) != 2)
		return make_error("CONS -- Expected two operands", context);
	
	return cons(car(list), car(cdr(list)));
}

data_t *prim_list(const data_t *list, lisp_ctx_t *context) {
	if(!list)
		return NULL;
	return cons(car(list), prim_list(cdr(list), context));
}

data_t *prim_set_car(const data_t *list, lisp_ctx_t *context) {
	data_t *head, *newcar;
	
	if(length(list) != 2)
		return make_error("SET-CAR -- Expected two operands", context);
	if((head = car(list)) == NULL)
		return make_error("SET-CAR -- Expected pair", context);

	newcar = car(cdr(list));
	if(head->type != pair)
		return make_error("SET-CAR -- Expected pair", context);

	head->pair->l = newcar;

	return head;
}

data_t *prim_set_cdr(const data_t *list, lisp_ctx_t *context) {
	data_t *head, *newcdr;
	
	if(length(list) != 2)
		return make_error("SET-CDR -- Expected two operands", context);
	if((head = car(list)) == NULL)
		return make_error("SET-CDR -- Expected pair", context);

	newcdr = car(cdr(list));
	if(head->type != pair)
		return make_error("SET-CDR -- Expected pair", context);

	head->pair->r = newcdr;

	return head;
}

data_t *prim_sym_to_str(const data_t *list, lisp_ctx_t *context) {
	data_t *sym;

	if(length(list) != 1)
		return make_error("SYMBOL->STRING -- Expected one operand", context);
	sym = car(list);

	if(!sym || sym->type != symbol)
		return make_error("SYMBOL->STRING -- Expected symbol", context);

	return make_string(sym->symbol, context);
}

data_t *prim_str_to_sym(const data_t *list, lisp_ctx_t *context) {
	data_t *str;

	if(length(list) != 1)
		return make_error("STRING->SYMBOL -- Expected one operand", context);
	str = car(list);

	if(!str || str->type != string)
		return make_error("STRING->SYMBOL -- Expected string", context);

	return make_symbol(str->string, context);
}

static data_t *is_type(const data_t *list, dtype_t type, lisp_ctx_t *context) {
	data_t *sym;
	if(length(list) != 1)
		return make_error("IS-TYPE -- Expected one operand", context);

	sym = car(list);
	if(sym && (sym->type == type))
		return make_symbol("#t", context);
	return make_symbol("#f", context);
}

data_t *prim_is_sym(const data_t *list, lisp_ctx_t *context) { return is_type(list, symbol, context); }

data_t *prim_is_str(const data_t *list, lisp_ctx_t *context) { return is_type(list, string, context); }
	
data_t *prim_is_pair(const data_t *list, lisp_ctx_t *context) { return is_type(list, pair, context); }

data_t *prim_is_int(const data_t *list, lisp_ctx_t *context) { return is_type(list, integer, context); }

data_t *prim_is_num(const data_t *list, lisp_ctx_t *context) {
	data_t *head;
	dtype_t type;
	
	if(length(list) != 1)
		return make_error("IS-NUM -- Expected one operand", context);

	if((head = car(list)) == NULL)
		return make_symbol("#f", context);

	type = head->type;
	if((type == integer) || (type == decimal))
		return make_symbol("#t", context);
	return make_symbol("#f", context);
}

data_t *prim_is_proc(const data_t *list, lisp_ctx_t *context) {
	if(length(list) != 1)
		return make_error("IS-PROC -- Expected one operand", context);

	list = car(list);
	if(!list || list->type != pair)
		return make_symbol("#f", context);
	
	list = car(list);
	if(!list || (list->type != symbol))
		return make_symbol("#f", context);
	
	if((!strcmp(list->symbol, "closure")) || (!strcmp(list->symbol, "primitive")))
		return make_symbol("#t", context);
	return make_symbol("#f", context);
}

static data_t *mathfn(const data_t *list, double (*func)(double), lisp_ctx_t *context) {
	data_t *val;
	
	if(length(list) != 1)
		return make_error("MATHFN -- Expected one operand", context);
	if((val = car(list)) == NULL)
		return make_error("MATHFN -- Expected number", context);

	if(val->type == integer)
		return make_decimal(func((double)val->integer), context);
	if(val->type == decimal)
		return make_decimal(func(val->decimal), context);
	return make_error("MATHFN -- Expected number", context);
}

data_t *prim_sin(const data_t *list, lisp_ctx_t *context) { return mathfn(list, sin, context); }

data_t *prim_cos(const data_t *list, lisp_ctx_t *context) { return mathfn(list, cos, context); }

data_t *prim_tan(const data_t *list, lisp_ctx_t *context) { return mathfn(list, tan, context); }

data_t *prim_asin(const data_t *list, lisp_ctx_t *context) { return mathfn(list, asin, context); }

data_t *prim_acos(const data_t *list, lisp_ctx_t *context) { return mathfn(list, acos, context); }

data_t *prim_atan(const data_t *list, lisp_ctx_t *context) { return mathfn(list, atan, context); }

data_t *prim_log(const data_t *list, lisp_ctx_t *context) { return mathfn(list, log, context); }

data_t *prim_exp(const data_t *list, lisp_ctx_t *context) { return mathfn(list, exp, context); }

data_t *prim_expt(const data_t *list, lisp_ctx_t *context) {
	data_t *base, *ex;
	double dbase, dex;
	
	if(length(list) != 2)
		return make_error("EXPT -- Expected one operand", context);
	if((base = car(list)) == NULL)
		return make_error("EXPT -- Expected number", context);
	if((ex = car(cdr(list))) == NULL)
		return make_error("EXPT -- Expected number", context);

	if(base->type == integer)
		dbase = (double)base->integer;
	else if(base->type == decimal)
		dbase = base->decimal;
	else
		return make_error("EXPT -- Expected number", context);

	if(ex->type == integer)
		dex = (double)ex->integer;
	else if(ex->type == decimal)
		dex = ex->decimal;
	else
		return make_error("EXPT -- Expected number", context);

	return make_decimal(pow(dbase, dex), context);
}

static int gcd(const int a, const int b) {
	if(a == 0)
		return b;
	else if(b == 0)
		return a;
	else if(a > b)
		return gcd(a % b, b);
	else
		return gcd(a, b % a);
}

static int lcm(const int a, const int b) { return a * b / gcd(a, b); }

static data_t *cumulfn(const data_t *list, int (*func)(const int, const int), lisp_ctx_t *context)  {
	int cumul, n;
	data_t *head;

	if(length(list) == 0)
		return make_int(0, context);
		
	head = car(list);
	if(!head || (head->type != integer))
		return make_error("CUMULFN -- Expected integer", context);
	cumul = head->integer;

	list = cdr(list);
	while(list) {
		head = car(list);
		if(!head || (head->type != integer))
			return make_error("CUMULFN -- Expected integer", context);

		n = head->integer;
		cumul = func(cumul, n);

		list = cdr(list);
	}

	return make_int(cumul, context);
}

data_t *prim_gcd(const data_t *list, lisp_ctx_t *context) {
	return cumulfn(list, gcd, context);
}

data_t *prim_lcm(const data_t *list, lisp_ctx_t *context) {
	return cumulfn(list, lcm, context);
}

data_t *prim_set_cvar(const data_t *list, lisp_ctx_t *context) {
	cvar_list_t *cvar = context->the_cvars;
	data_t *var, *val;
	char *var_name;
	int value;

	if(length(list) != 2)
		return make_error("SET-CVAR -- Expected two operands", context);

	var = car(list);
	val = car(cdr(list));

	if(!var || (var->type != symbol))
		return make_error("SET-CVAR -- Expected identifier", context);
	var_name = var->symbol;

	if(!val || (val->type != integer))
		return make_error("SET-CVAR -- Expected integer", context);
	value = val->integer;

	while(cvar) {
		if(!strcmp(cvar->name, var_name)) {
			if(cvar->access == CVAR_READONLY)
				return make_error("SET-CVAR -- Read only", context);
			*(cvar->value) = value;
			return make_symbol("ok", context);
		}
		cvar = cvar->next;
	}

	return make_error("SET-CVAR -- Unknown CVAR", context);
}

data_t *prim_get_cvar(const data_t *list, lisp_ctx_t *context) {
	cvar_list_t *cvar = context->the_cvars;
	data_t *var;
	char *var_name;

	if(length(list) != 1)
		return make_error("GET-CVAR -- Expected one operand", context);
	if((var = car(list)) == NULL)
		return make_error("GET-CVAR -- Expected identifier", context);

	if(var->type != symbol)
		return make_error("GET-CVAR -- Expected identifier", context);
	var_name = var->symbol;

	while(cvar) {
		if(!strcmp(cvar->name, var_name))
			return make_int(*(cvar->value), context);
		cvar = cvar->next;
	}
	
	return make_error("GET-CVAR -- Unknown CVAR", context);
}

/* --- */

static data_t *primitive_procedure_names(lisp_ctx_t *context) {
	prim_proc_list_t *curr_proc = context->the_last_prim_proc;
	data_t *out = NULL;

	while(curr_proc) {
		out = cons(make_symbol(curr_proc->name, context), out);
		curr_proc = curr_proc->prev;
	}

	return out;
}

static data_t *primitive_procedure_objects(lisp_ctx_t *context) {
	prim_proc_list_t *curr_proc = context->the_last_prim_proc;
	data_t *out = NULL;

	while(curr_proc) {
		out = cons(cons(make_symbol("primitive", context), cons(make_primitive(curr_proc->proc, context), NULL)), out);
		curr_proc = curr_proc->prev;
	}

	return out;
}

void add_prim_proc(char *name, prim_proc proc, lisp_ctx_t *context) {
	prim_proc_list_t *curr_proc;

	if(context->the_last_prim_proc == NULL) {
		context->the_prim_procs = malloc(sizeof(prim_proc_list_t));
		context->the_prim_procs->name = malloc(strlen(name) + 1);
		strcpy(context->the_prim_procs->name, name);
		context->the_prim_procs->proc = proc;
		context->the_prim_procs->next = NULL;
		context->the_prim_procs->prev = NULL;
		context->the_last_prim_proc = context->the_prim_procs;
		return;
	}
	
	curr_proc = malloc(sizeof(prim_proc_list_t));
	curr_proc->name = malloc(strlen(name) + 1);
	strcpy(curr_proc->name, name);
	curr_proc->proc = proc;
	curr_proc->prev = context->the_last_prim_proc;
	curr_proc->next = NULL;

	context->the_last_prim_proc->next = curr_proc;
	context->the_last_prim_proc = curr_proc;
}

void add_cvar(const char *name, const size_t *valptr, const int access, lisp_ctx_t *context) {
	cvar_list_t *curr_var;

	if(context->the_last_cvar == NULL) {
		context->the_cvars = malloc(sizeof(cvar_list_t));
		context->the_cvars->name = malloc(strlen(name) + 1);
		strcpy(context->the_cvars->name, name);
		context->the_cvars->value = (size_t*)valptr;
		context->the_cvars->access = (int)access;
		context->the_cvars->next = NULL;
		context->the_last_cvar = context->the_cvars;
		return;
	}
	
	curr_var = malloc(sizeof(cvar_list_t));
	curr_var->name = malloc(strlen(name) + 1);
	strcpy(curr_var->name, name);
	curr_var->value = (size_t*)valptr;
	curr_var->access = (int)access;
	curr_var->next = NULL;

	context->the_last_cvar->next = curr_var;
	context->the_last_cvar = curr_var;
}

void add_builtin_prim_procs(lisp_ctx_t *context) {
	add_prim_proc("+", prim_add, context);
	add_prim_proc("*", prim_mul, context);
	add_prim_proc("-", prim_sub, context);
	add_prim_proc("/", prim_div, context);
	add_prim_proc("=", prim_comp_eq, context);
	add_prim_proc("<", prim_comp_less, context);
	add_prim_proc(">", prim_comp_more, context);
	add_prim_proc("or", prim_or, context);
	add_prim_proc("and", prim_and, context);
	add_prim_proc("not", prim_not, context);
	add_prim_proc("floor", prim_floor, context);
	add_prim_proc("ceiling", prim_ceiling, context);
	add_prim_proc("truncate", prim_trunc, context);
	add_prim_proc("round", prim_round, context);
	add_prim_proc("max", prim_max, context);
	add_prim_proc("min", prim_min, context);
	add_prim_proc("eq?", prim_eq, context);
	add_prim_proc("car", prim_car, context);
	add_prim_proc("cdr", prim_cdr, context);
	add_prim_proc("set-car!", prim_set_car, context);
	add_prim_proc("set-cdr!", prim_set_cdr, context);
	add_prim_proc("cons", prim_cons, context);
	add_prim_proc("list", prim_list, context);
	add_prim_proc("number?", prim_is_num, context);
	add_prim_proc("real?", prim_is_num, context);
	add_prim_proc("integer?", prim_is_int, context);
	add_prim_proc("procedure?", prim_is_proc, context);
	add_prim_proc("symbol->string", prim_sym_to_str, context);
	add_prim_proc("string->symbol", prim_str_to_sym, context);
	add_prim_proc("symbol?", prim_is_sym, context);
	add_prim_proc("string?", prim_is_str, context);
	add_prim_proc("pair?", prim_is_pair, context);
	add_prim_proc("gcd", prim_gcd, context);
	add_prim_proc("lcm", prim_lcm, context);

	add_prim_proc("sin", prim_sin, context);
	add_prim_proc("cos", prim_cos, context);
	add_prim_proc("tan", prim_tan, context);
	add_prim_proc("asin", prim_asin, context);
	add_prim_proc("acos", prim_acos, context);
	add_prim_proc("atan", prim_atan, context);
	add_prim_proc("log", prim_log, context);
	add_prim_proc("exp", prim_exp, context);
	add_prim_proc("expt", prim_expt, context);

	add_prim_proc("set-cvar!", prim_set_cvar, context);
	add_prim_proc("get-cvar", prim_get_cvar, context);
}

void setup_environment(lisp_ctx_t *context) {
	data_t *the_empty_environment = cons(cons(NULL, NULL), NULL);

	add_cvar("mem_lim_hard", &context->mem_lim_hard, CVAR_READONLY, context);
	add_cvar("mem_lim_soft", &context->mem_lim_soft, CVAR_READONLY, context);
	add_cvar("mem_list_entries", &context->mem_list_entries, CVAR_READONLY, context);
	add_cvar("mem_verbosity", &context->mem_verbosity, CVAR_READWRITE, context);
	add_cvar("mem_allocated", &context->mem_allocated, CVAR_READONLY, context);
	add_cvar("thread_timeout", &context->thread_timeout, CVAR_READWRITE, context);

	context->the_global_environment = 
		extend_environment(primitive_procedure_names(context), 
						   primitive_procedure_objects(context),
						   the_empty_environment, context);

	run_exp("(define (caar pair) (car (car pair)))", context);
	run_exp("(define (cadr pair) (car (cdr pair)))", context);
	run_exp("(define (cdar pair) (cdr (car pair)))", context);
	run_exp("(define (cddr pair) (cdr (cdr pair)))", context);

	run_exp("(define (caaar pair) (car (car (car pair))))", context);
	run_exp("(define (caadr pair) (car (car (cdr pair))))", context);
	run_exp("(define (cadar pair) (car (cdr (car pair))))", context);
	run_exp("(define (caddr pair) (car (cdr (cdr pair))))", context);
	run_exp("(define (cdaar pair) (cdr (car (car pair))))", context);
	run_exp("(define (cdadr pair) (cdr (car (cdr pair))))", context);
	run_exp("(define (cddar pair) (cdr (cdr (car pair))))", context);
	run_exp("(define (cdddr pair) (cdr (cdr (cdr pair))))", context);

	run_exp("(define (caaaar pair) (car (car (car (car pair)))))", context);
	run_exp("(define (caaadr pair) (car (car (car (cdr pair)))))", context);
	run_exp("(define (caadar pair) (car (car (cdr (car pair)))))", context);
	run_exp("(define (caaddr pair) (car (car (cdr (cdr pair)))))", context);
	run_exp("(define (cadaar pair) (car (cdr (car (car pair)))))", context);
	run_exp("(define (cadadr pair) (car (cdr (car (cdr pair)))))", context);
	run_exp("(define (caddar pair) (car (cdr (cdr (car pair)))))", context);
	run_exp("(define (cadddr pair) (car (cdr (cdr (cdr pair)))))", context);
	run_exp("(define (cdaaar pair) (cdr (car (car (car pair)))))", context);
	run_exp("(define (cdaadr pair) (cdr (car (car (cdr pair)))))", context);
	run_exp("(define (cdadar pair) (cdr (car (cdr (car pair)))))", context);
	run_exp("(define (cdaddr pair) (cdr (car (cdr (cdr pair)))))", context);
	run_exp("(define (cddaar pair) (cdr (cdr (car (car pair)))))", context);
	run_exp("(define (cddadr pair) (cdr (cdr (car (cdr pair)))))", context);
	run_exp("(define (cdddar pair) (cdr (cdr (cdr (car pair)))))", context);
	run_exp("(define (cddddr pair) (cdr (cdr (cdr (cdr pair)))))", context);

	run_exp("(define nil '())", context);
	run_exp("(define (zero? exp) (= 0 exp))", context);
	run_exp("(define (null? exp) (eq? exp nil))", context);
	run_exp("(define (negative? exp) (< exp 0))", context);
	run_exp("(define (positive? exp) (> exp 0))", context);
	run_exp("(define (boolean? exp) (or (eq? exp '#t) (eq? exp '#f)))", context);
	run_exp("(define (abs n) (if (negative? n) (- 0 n) n))", context);
	run_exp("(define (<= a b) (not (> a b)))", context);
	run_exp("(define (>= a b) (not (< a b)))", context);
	run_exp("(define (map proc items) (if (null? items) nil (cons (proc (car items)) (map proc (cdr items)))))", context);
	run_exp("(define (fact n) (if (= n 1) 1 (* n (fact (- n 1)))))", context);
	run_exp("(define (delay proc) (lambda () proc))", context);
	run_exp("(define (force proc) (proc))", context);
	run_exp("(define (length list) (define (list-loop part count) (if (null? part) count (list-loop (cdr part) (+ count 1)))) (list-loop list 0))", context);
	run_exp("(define (modulo num div) (- num (* (floor (/ num div)) div)))", context);
	run_exp("(define (quotient num div) (truncate (/ num div)))", context);
	run_exp("(define (remainder num div) (+ (* (quotient num div) div -1) num))", context);
	run_exp("(define (odd? n) (if (= 1 (modulo n 2)) '#t '#f))", context);
	run_exp("(define (even? n) (not (odd? n)))", context);
	run_exp("(define (square n) (* n n))", context);
	run_exp("(define (average a b) (/ (+ a b) 2))", context);
	run_exp("(define (sqrt x) (define (good-enough? guess) (< (abs (- (square guess) x)) 0.000001)) (define (improve guess) (average guess (/ x guess))) (define (sqrt-iter guess) (if (good-enough? guess) (abs guess) (sqrt-iter (improve guess)))) (sqrt-iter 1.0))", context);
	run_exp("(define (append list1 list2) (if (null? list1) list2 (cons (car list1) (append (cdr list1) list2))))", context);

	run_gc(GC_FORCE, context);
}

void cleanup_lisp(lisp_ctx_t *context) {
	prim_proc_list_t *current_proc = context->the_prim_procs, *procbuf;
	cvar_list_t *current_var = context->the_cvars, *varbuf;

	run_gc(GC_FORCE, context);
	free_data_rec(context->the_global_environment, context);

	while(current_proc) {
		procbuf = current_proc->next;
		free(current_proc->name);
		free(current_proc);
		current_proc = procbuf;
	}
	context->the_prim_procs = NULL;
	context->the_last_prim_proc = NULL;

	while(current_var) {
		varbuf = current_var->next;
		free(current_var->name);
		free(current_var);
		current_var = varbuf;
	}
	context->the_cvars = NULL;
	context->the_last_cvar = NULL;
}

lisp_ctx_t *make_context(const size_t mem_lim_soft, const size_t mem_lim_hard, const size_t mem_verbosity, const size_t thread_timeout) {
	lisp_ctx_t *out;

	if((out = malloc(sizeof(lisp_ctx_t))) == NULL)
		return NULL;

	out->the_cvars = NULL;
	out->the_last_cvar = NULL;
	out->the_prim_procs = NULL;
	out->the_last_prim_proc = NULL;

	out->mem_lim_soft = mem_lim_soft;
	out->mem_lim_hard = mem_lim_hard;
	out->mem_list_entries = 0;
	out->mem_allocated = 0;
	out->mem_verbosity = mem_verbosity;
	out->n_allocs = 0;
	out->n_frees = 0;
	out->n_bytes_peak = 0;
	out->warned = 0;
	out->alloc_list = NULL;

	out->thread_timeout = thread_timeout;
	out->thread_running = 0;
	out->eval_plz_die = 0;

	add_builtin_prim_procs(out);

	return out;
}

void destroy_context(lisp_ctx_t *context) {
	if(context == NULL)
		return;

	cleanup_lisp(context);
	showmemstats(stderr, context);

	free(context);
}