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

static lisp_data_t *prim_add(const lisp_data_t *list, lisp_ctx_t *context) {
	int iout = 0;
	double dout = 0.0f;
	lisp_data_t *head, *tail;

	while(list) {
		if((head = lisp_car(list)) == NULL)
			return lisp_make_error("+ -- Expected number", context);
		tail = lisp_cdr(list);

		if(head->type == lisp_type_integer)
			iout += head->integer;
		else if(head->type == lisp_type_decimal)
			dout += head->decimal;
		else return lisp_make_error("+ -- Expected number", context);

		list = tail;
	}

	if(dout == 0.0f)
		return lisp_make_int(iout, context);
	
	if((dout + iout) == floor(dout + iout))
		return lisp_make_int((int)dout + iout, context);

	return lisp_make_decimal(dout + iout, context);
}

static lisp_data_t *prim_mul(const lisp_data_t *list, lisp_ctx_t *context) {
	int iout = 1;
	double dout = 1.0f;
	lisp_data_t *head, *tail;

	while(list) {
		if((head = lisp_car(list)) == NULL)
			return lisp_make_error("* -- Expected number", context);
		tail = lisp_cdr(list);
		if(head->type == lisp_type_integer)
			iout *= head->integer;
		else if(head->type == lisp_type_decimal)
			dout *= head->decimal;
		else return lisp_make_error("* -- Expected number", context);

		list = tail;
	}

	if(dout == 1.0f)
		return lisp_make_int(iout, context);

	if((dout * iout) == floor(dout * iout))
		return lisp_make_int((int)dout * iout, context);
	
	return lisp_make_decimal(dout * iout, context);
}

static lisp_data_t *prim_sub(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_type_t out_type;
	int iout = 0, istart;
	double dout = 0.0f, dstart;
	lisp_data_t *head, *tail;

	if(!lisp_list_length(list))
		return lisp_make_error("- -- No operands", context);
	if((head = lisp_car(list)) == NULL)
		return lisp_make_error("- -- Expected number", context);

	tail = lisp_cdr(list);
	out_type = head->type;
	if(out_type == lisp_type_decimal)
		dstart = head->decimal;
	else if(out_type == lisp_type_integer)
		istart = head->integer;
	else
		return lisp_make_error("- -- Expected number", context);

	list = tail;

	if(!list) {
		if(out_type == lisp_type_integer) {
			return lisp_make_int(-istart, context);
		} else {
			return lisp_make_decimal(-dstart, context);
		}
	}

	do {
		if((head = lisp_car(list)) == NULL)
			return lisp_make_error("- -- Expected number", context);
		tail = lisp_cdr(list);
		if(head->type == lisp_type_integer)
			iout += head->integer;
		else if(head->type == lisp_type_decimal) {
			if(out_type == lisp_type_integer) {
				out_type = lisp_type_decimal;
				dstart = (double)istart;
			}
			dout += head->decimal;
		}
		else return lisp_make_error("- -- Expected number", context);

		list = tail;
	} while(list);

	if(out_type == lisp_type_integer)
		return lisp_make_int(istart - iout, context);
	
	return lisp_make_decimal(dstart - dout - iout, context);
}

static lisp_data_t *prim_div(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_type_t start_type;
	double dout = 1.0f, dstart;
	lisp_data_t *head, *tail;

	if(!lisp_list_length(list))
		return lisp_make_error("/ -- No operands", context);
	if((head = lisp_car(list)) == NULL)
		return lisp_make_error("/ -- Expected number", context);

	tail = lisp_cdr(list);
	start_type = head->type;
	if(start_type == lisp_type_decimal)
		dstart = head->decimal;
	else if(start_type == lisp_type_integer)
		dstart = (double)head->integer;
	else
		return lisp_make_error("/ -- Expected number", context);

	list = tail;

	if(!list)
		return lisp_make_decimal(1 / dstart, context);

	do {
		if((head = lisp_car(list)) == NULL)
			return lisp_make_error("/ -- Expected number", context);
		tail = lisp_cdr(list);

		if(head->type == lisp_type_integer)
			dout *= head->integer;
		else if(head->type == lisp_type_decimal)
			dout *= head->decimal;
		else return 0;

		list = tail;
	} while(list);

	if(dout == 0)
		return lisp_make_error("/ -- Division by zero", context);
	
	if(dstart / dout == floor(dstart / dout))
		return lisp_make_int((int)(dstart / dout), context);

	return lisp_make_decimal(dstart / dout, context);
}

static lisp_data_t *prim_comp_eq(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *first, *second;
	lisp_type_t type_first, type_second;

	if(lisp_list_length(list) != 2)
		return lisp_make_error("= -- Expected two operands", context);
	if((first = lisp_car(list)) == NULL)
		return lisp_make_error("= -- Expected number", context);
	if((second = lisp_cdr(list)) == NULL)
		return lisp_make_error("= -- Expected pair", context);
	if(second->type != lisp_type_pair)
		return lisp_make_error("= -- Expected pair", context);
	if((second = lisp_car(second)) == NULL)
		return lisp_make_error("= -- Expected number", context);

	type_first = first->type;
	type_second = second->type;

	if((type_first != lisp_type_decimal) && (type_first != lisp_type_integer))
		return lisp_make_error("= -- Expected number", context);
	if((type_second != lisp_type_decimal) && (type_second != lisp_type_integer))
		return lisp_make_error("= -- Expected number", context);

	if(type_first == lisp_type_integer)
		if(first->integer == second->integer)
			return lisp_make_symbol("#t", context);

	if(type_first == lisp_type_decimal)
		if(first->decimal == second->decimal)
			return lisp_make_symbol("#t", context);

	return lisp_make_symbol("#f", context);
}

static lisp_data_t *prim_comp_less(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *head, *tail;

	if(lisp_list_length(list) != 2)
		return lisp_make_error("< -- Expected two operands", context);
	if((head = lisp_car(list)) == NULL)
		return lisp_make_error("< -- Expected number", context);
	if((tail = lisp_car(lisp_cdr(list))) == NULL)
		return lisp_make_error("< -- Expected number", context);
		
	if((head->type == lisp_type_integer) && (tail->type == lisp_type_integer)) {
		if(head->integer < tail->integer) {
			return lisp_make_symbol("#t", context);
		} else {
			return lisp_make_symbol("#f", context);
		}
	} else if((head->type == lisp_type_decimal) && (tail->type == lisp_type_integer)) {
		if(head->decimal < tail->integer) {
			return lisp_make_symbol("#t", context);
		} else {
			return lisp_make_symbol("#f", context);
		}
	} else if((head->type == lisp_type_integer) && (tail->type == lisp_type_decimal)) {
		if(head->integer < tail->decimal) {
			return lisp_make_symbol("#t", context);
		} else {
			return lisp_make_symbol("#f", context);
		}
	} else if((head->type == lisp_type_decimal) && (tail->type == lisp_type_decimal)) {
		if(head->decimal < tail->decimal) {
			return lisp_make_symbol("#t", context);
		} else {
			return lisp_make_symbol("#f", context);
		}
	}

	return lisp_make_error("< -- Invalid comparison", context);
}

static lisp_data_t *prim_comp_more(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *head, *tail;

	if(lisp_list_length(list) != 2)
		return lisp_make_error("> -- Expected two operands", context);
	if((head = lisp_car(list)) == NULL)
		return lisp_make_error("> -- Expected number", context);
	if((tail = lisp_car(lisp_cdr(list))) == NULL)
		return lisp_make_error("> -- Expected number", context);

	if((head->type == lisp_type_integer) && (tail->type == lisp_type_integer)) {
		if(head->integer > tail->integer) {
			return lisp_make_symbol("#t", context);
		} else {
			return lisp_make_symbol("#f", context);
		}
	} else if((head->type == lisp_type_decimal) && (tail->type == lisp_type_integer)) {
		if(head->decimal > tail->integer) {
			return lisp_make_symbol("#t", context);
		} else {
			return lisp_make_symbol("#f", context);
		}
	} else if((head->type == lisp_type_integer) && (tail->type == lisp_type_decimal)) {
		if(head->integer > tail->decimal) {
			return lisp_make_symbol("#t", context);
		} else {
			return lisp_make_symbol("#f", context);
		}
	} else if((head->type == lisp_type_decimal) && (tail->type == lisp_type_decimal)) {
		if(head->decimal > tail->decimal) {
			return lisp_make_symbol("#t", context);
		} else {
			return lisp_make_symbol("#f", context);
		}
	}

	return lisp_make_error("> -- Invalid comparison", context);
}

static lisp_data_t *prim_or(const lisp_data_t *list, lisp_ctx_t *context) {
	while(list) {
		if(lisp_is_equal(lisp_car(list), lisp_make_symbol("#t", context)))
			return lisp_make_symbol("#t", context);
		list = lisp_cdr(list);
	}
	return lisp_make_symbol("#f", context);
}

static lisp_data_t *prim_and(const lisp_data_t *list, lisp_ctx_t *context) {
	while(list) {
		if(lisp_is_equal(lisp_car(list), lisp_make_symbol("#f", context)))
			return lisp_make_symbol("#f", context);
		list = lisp_cdr(list);
	}
	return lisp_make_symbol("#t", context);
}

static lisp_data_t *prim_floor(const lisp_data_t *list, lisp_ctx_t *context) {
	if(lisp_list_length(list) != 1)
		return lisp_make_error("FLOOR -- Expected one operand", context);
	if((list = lisp_car(list)) == NULL)
		return lisp_make_error("FLOOR -- Expected number", context);
		
	if(list->type == lisp_type_integer)
		return lisp_make_int(list->integer, context);

	if(list->type == lisp_type_decimal)
		return lisp_make_int((int)floor(list->decimal), context);

	return lisp_make_error("FLOOR -- Expected number", context);
}

static lisp_data_t *prim_ceiling(const lisp_data_t *list, lisp_ctx_t *context) {
	if(lisp_list_length(list) != 1)
		return lisp_make_error("CEILING -- Expected one operand", context);
	if((list = lisp_car(list)) == NULL)
		return lisp_make_error("CEILING -- Expected number", context);

	if(list->type == lisp_type_integer)
		return lisp_make_int(list->integer, context);

	if(list->type == lisp_type_decimal)
		return lisp_make_int((int)ceil(list->decimal), context);

	return lisp_make_error("CEILING -- Invalid comparison", context);
}

static lisp_data_t *prim_trunc(const lisp_data_t *list, lisp_ctx_t *context) {
	double num;

	if(lisp_list_length(list) != 1)
		return lisp_make_error("TRUNCATE -- Expected one operand", context);
	if((list = lisp_car(list)) == NULL)
		return lisp_make_error("TRUNCATE -- Expected number", context);
		
	if(list->type == lisp_type_integer)
		return lisp_make_int(list->integer, context);

	if(list->type == lisp_type_decimal) {
		num = list->decimal;

		if(num < 0)
			return lisp_make_int((int)ceil(list->decimal), context);
		return lisp_make_int((int)floor(list->decimal), context);
	}

	return lisp_make_error("TRUNCATE -- Expected number", context);
}

static lisp_data_t *prim_round(const lisp_data_t *list, lisp_ctx_t *context) {
	double num, fracpart;
	int intpart;

	if(lisp_list_length(list) != 1)
		return lisp_make_error("ROUND -- Expected one operand", context);
	if((list = lisp_car(list)) == NULL)
		return lisp_make_error("ROUND -- Expected number", context);

	if(list->type == lisp_type_integer)
		return lisp_make_int(list->integer, context);

	if(list->type == lisp_type_decimal) {
		num = list->decimal;
		fracpart = num - floor(num);
		if(fracpart < .5)
			return lisp_make_int((int)(num - fracpart), context);
		if(fracpart > .5)
			return lisp_make_int((int)(num - fracpart + 1), context);
		intpart = (int)(num - fracpart);
		if(intpart % 2)
			return lisp_make_int(intpart + 1, context);
		return lisp_make_int(intpart, context);
	}

	return lisp_make_error("ROUND -- Expected number", context);
}

static lisp_data_t *prim_max(const lisp_data_t *list, lisp_ctx_t *context) {
	int ival, imax = 0;
	double dval, dmax = 0.0f;
	lisp_data_t *val;

	if(!lisp_list_length(list))
		return lisp_make_error("MAX -- No operands", context);

	while(list) {
		if(list->type != lisp_type_pair)
			return lisp_make_error("MAX -- Expected pair", context);
		val = lisp_car(list);
		if(val->type == lisp_type_integer) {
			ival = val->integer;
			if(ival > imax)
				imax = ival;
		} else if(val->type == lisp_type_decimal) {
			dval = val->decimal;
			if(dval > dmax)
				dmax = dval;
		} else
			return lisp_make_error("MAX -- Expected number", context);
		list = lisp_cdr(list);
	}

	if((double)imax > dmax)
		return lisp_make_int(imax, context);
	return lisp_make_decimal(dmax, context);
}

static lisp_data_t *prim_min(const lisp_data_t *list, lisp_ctx_t *context) {
	int ival, imin = INT_MAX;
	double dval, dmin = DBL_MAX;
	lisp_data_t *val;

	if(!lisp_list_length(list))
		return lisp_make_error("MIN -- No operands", context);

	while(list) {
		if(list->type != lisp_type_pair)
			return lisp_make_error("MIN -- Expected pair", context);
		val = lisp_car(list);
		if(val->type == lisp_type_integer) {
			ival = val->integer;
			if(ival < imin)
				imin = ival;
		} else if(val->type == lisp_type_decimal) {
			dval = val->decimal;
			if(dval < dmin)
				dmin = dval;
		}
		list = lisp_cdr(list);
	}

	if((double)imin < dmin)
		return lisp_make_int(imin, context);
	return lisp_make_decimal(dmin, context);
}

static lisp_data_t *prim_eq(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *first, *second;

	if(lisp_list_length(list) != 2)
		return lisp_make_error("EQ? -- No operands", context);

	first = lisp_car(list);
	second = lisp_car(lisp_cdr(list));
	
	if(lisp_is_equal(first, second))
		return lisp_make_symbol("#t", context);
	return lisp_make_symbol("#f", context);
}

static lisp_data_t *prim_not(const lisp_data_t *list, lisp_ctx_t *context) {
	if(lisp_list_length(list) != 1)
		return lisp_make_error("NOT -- Expected one operand", context);
	if((list = lisp_car(list)) == NULL)
		return lisp_make_error("NOT -- Expected boolean", context);
	
	if(!strcmp(list->symbol, "#f"))
		return lisp_make_symbol("#t", context);
	return lisp_make_symbol("#f", context);
}

static lisp_data_t *prim_car(const lisp_data_t *list, lisp_ctx_t *context) {
	if(lisp_list_length(list) != 1)
		return lisp_make_error("CAR -- Expected one operand", context);
	
	list = lisp_car(list);
	
	if(list && list->type == lisp_type_pair)
		return lisp_car(list);
	return NULL;
}

static lisp_data_t *prim_cdr(const lisp_data_t *list, lisp_ctx_t *context) {
	if(lisp_list_length(list) != 1)
		return lisp_make_error("CDR -- Expected one operand", context);
		
	list = lisp_car(list);
	
	if(list && list->type == lisp_type_pair)
		return lisp_cdr(list);
	return NULL;
}

static lisp_data_t *prim_cons(const lisp_data_t *list, lisp_ctx_t *context) {
	if(lisp_list_length(list) != 2)
		return lisp_make_error("CONS -- Expected two operands", context);
	
	return lisp_cons(lisp_car(list), lisp_car(lisp_cdr(list)));
}

static lisp_data_t *prim_list(const lisp_data_t *list, lisp_ctx_t *context) {
	if(!list)
		return NULL;
	return lisp_cons(lisp_car(list), prim_list(lisp_cdr(list), context));
}

static lisp_data_t *prim_set_car(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *head, *newcar;
	
	if(lisp_list_length(list) != 2)
		return lisp_make_error("SET-CAR -- Expected two operands", context);
	if((head = lisp_car(list)) == NULL)
		return lisp_make_error("SET-CAR -- Expected pair", context);

	newcar = lisp_car(lisp_cdr(list));
	if(head->type != lisp_type_pair)
		return lisp_make_error("SET-CAR -- Expected pair", context);

	head->pair->l = newcar;

	return head;
}

static lisp_data_t *prim_set_cdr(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *head, *newcdr;
	
	if(lisp_list_length(list) != 2)
		return lisp_make_error("SET-CDR -- Expected two operands", context);
	if((head = lisp_car(list)) == NULL)
		return lisp_make_error("SET-CDR -- Expected pair", context);

	newcdr = lisp_car(lisp_cdr(list));
	if(head->type != lisp_type_pair)
		return lisp_make_error("SET-CDR -- Expected pair", context);

	head->pair->r = newcdr;

	return head;
}

static lisp_data_t *prim_sym_to_str(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *sym;

	if(lisp_list_length(list) != 1)
		return lisp_make_error("SYMBOL->STRING -- Expected one operand", context);
	sym = lisp_car(list);

	if(!sym || sym->type != lisp_type_symbol)
		return lisp_make_error("SYMBOL->STRING -- Expected symbol", context);

	return lisp_make_string(sym->symbol, context);
}

static lisp_data_t *prim_str_to_sym(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *str;

	if(lisp_list_length(list) != 1)
		return lisp_make_error("STRING->SYMBOL -- Expected one operand", context);
	str = lisp_car(list);

	if(!str || str->type != lisp_type_string)
		return lisp_make_error("STRING->SYMBOL -- Expected string", context);

	return lisp_make_symbol(str->string, context);
}

static lisp_data_t *is_type(const lisp_data_t *list, lisp_type_t type, lisp_ctx_t *context) {
	lisp_data_t *sym;
	if(lisp_list_length(list) != 1)
		return lisp_make_error("IS-TYPE -- Expected one operand", context);

	sym = lisp_car(list);
	if(sym && (sym->type == type))
		return lisp_make_symbol("#t", context);
	return lisp_make_symbol("#f", context);
}

static lisp_data_t *prim_is_sym(const lisp_data_t *list, lisp_ctx_t *context) { return is_type(list, lisp_type_symbol, context); }

static lisp_data_t *prim_is_str(const lisp_data_t *list, lisp_ctx_t *context) { return is_type(list, lisp_type_string, context); }
	
static lisp_data_t *prim_is_pair(const lisp_data_t *list, lisp_ctx_t *context) { return is_type(list, lisp_type_pair, context); }

static lisp_data_t *prim_is_int(const lisp_data_t *list, lisp_ctx_t *context) { return is_type(list, lisp_type_integer, context); }

static lisp_data_t *prim_is_num(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *head;
	lisp_type_t type;
	
	if(lisp_list_length(list) != 1)
		return lisp_make_error("IS-NUM -- Expected one operand", context);

	if((head = lisp_car(list)) == NULL)
		return lisp_make_symbol("#f", context);

	type = head->type;
	if((type == lisp_type_integer) || (type == lisp_type_decimal))
		return lisp_make_symbol("#t", context);
	return lisp_make_symbol("#f", context);
}

static lisp_data_t *prim_is_proc(const lisp_data_t *list, lisp_ctx_t *context) {
	if(lisp_list_length(list) != 1)
		return lisp_make_error("IS-PROC -- Expected one operand", context);

	list = lisp_car(list);
	if(!list || list->type != lisp_type_pair)
		return lisp_make_symbol("#f", context);
	
	list = lisp_car(list);
	if(!list || (list->type != lisp_type_symbol))
		return lisp_make_symbol("#f", context);
	
	if((!strcmp(list->symbol, "closure")) || (!strcmp(list->symbol, "primitive")))
		return lisp_make_symbol("#t", context);
	return lisp_make_symbol("#f", context);
}

static lisp_data_t *mathfn(const lisp_data_t *list, double (*func)(double), lisp_ctx_t *context) {
	lisp_data_t *val;
	
	if(lisp_list_length(list) != 1)
		return lisp_make_error("MATHFN -- Expected one operand", context);
	if((val = lisp_car(list)) == NULL)
		return lisp_make_error("MATHFN -- Expected number", context);

	if(val->type == lisp_type_integer)
		return lisp_make_decimal(func((double)val->integer), context);
	if(val->type == lisp_type_decimal)
		return lisp_make_decimal(func(val->decimal), context);
	return lisp_make_error("MATHFN -- Expected number", context);
}

static lisp_data_t *prim_sin(const lisp_data_t *list, lisp_ctx_t *context) { return mathfn(list, sin, context); }

static lisp_data_t *prim_cos(const lisp_data_t *list, lisp_ctx_t *context) { return mathfn(list, cos, context); }

static lisp_data_t *prim_tan(const lisp_data_t *list, lisp_ctx_t *context) { return mathfn(list, tan, context); }

static lisp_data_t *prim_asin(const lisp_data_t *list, lisp_ctx_t *context) { return mathfn(list, asin, context); }

static lisp_data_t *prim_acos(const lisp_data_t *list, lisp_ctx_t *context) { return mathfn(list, acos, context); }

static lisp_data_t *prim_atan(const lisp_data_t *list, lisp_ctx_t *context) { return mathfn(list, atan, context); }

static lisp_data_t *prim_log(const lisp_data_t *list, lisp_ctx_t *context) { return mathfn(list, log, context); }

static lisp_data_t *prim_exp(const lisp_data_t *list, lisp_ctx_t *context) { return mathfn(list, exp, context); }

static lisp_data_t *prim_expt(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_data_t *base, *ex;
	double dbase, dex;
	
	if(lisp_list_length(list) != 2)
		return lisp_make_error("EXPT -- Expected one operand", context);
	if((base = lisp_car(list)) == NULL)
		return lisp_make_error("EXPT -- Expected number", context);
	if((ex = lisp_car(lisp_cdr(list))) == NULL)
		return lisp_make_error("EXPT -- Expected number", context);

	if(base->type == lisp_type_integer)
		dbase = (double)base->integer;
	else if(base->type == lisp_type_decimal)
		dbase = base->decimal;
	else
		return lisp_make_error("EXPT -- Expected number", context);

	if(ex->type == lisp_type_integer)
		dex = (double)ex->integer;
	else if(ex->type == lisp_type_decimal)
		dex = ex->decimal;
	else
		return lisp_make_error("EXPT -- Expected number", context);

	return lisp_make_decimal(pow(dbase, dex), context);
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

static lisp_data_t *cumulfn(const lisp_data_t *list, int (*func)(const int, const int), lisp_ctx_t *context)  {
	int cumul, n;
	lisp_data_t *head;

	if(lisp_list_length(list) == 0)
		return lisp_make_int(0, context);
		
	head = lisp_car(list);
	if(!head || (head->type != lisp_type_integer))
		return lisp_make_error("CUMULFN -- Expected integer", context);
	cumul = head->integer;

	list = lisp_cdr(list);
	while(list) {
		head = lisp_car(list);
		if(!head || (head->type != lisp_type_integer))
			return lisp_make_error("CUMULFN -- Expected integer", context);

		n = head->integer;
		cumul = func(cumul, n);

		list = lisp_cdr(list);
	}

	return lisp_make_int(cumul, context);
}

static lisp_data_t *prim_gcd(const lisp_data_t *list, lisp_ctx_t *context) {
	return cumulfn(list, gcd, context);
}

static lisp_data_t *prim_lcm(const lisp_data_t *list, lisp_ctx_t *context) {
	return cumulfn(list, lcm, context);
}

static lisp_data_t *prim_set_cvar(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_cvar_list_t *cvar = context->the_cvars;
	lisp_data_t *var, *val;
	char *var_name;
	int value;

	if(lisp_list_length(list) != 2)
		return lisp_make_error("SET-CVAR -- Expected two operands", context);

	var = lisp_car(list);
	val = lisp_car(lisp_cdr(list));

	if(!var || (var->type != lisp_type_symbol))
		return lisp_make_error("SET-CVAR -- Expected identifier", context);
	var_name = var->symbol;

	if(!val || (val->type != lisp_type_integer))
		return lisp_make_error("SET-CVAR -- Expected integer", context);
	value = val->integer;

	while(cvar) {
		if(!strcmp(cvar->name, var_name)) {
			if(cvar->access == LISP_CVAR_RO)
				return lisp_make_error("SET-CVAR -- Read only", context);
			*(cvar->value) = value;
			return lisp_make_symbol("ok", context);
		}
		cvar = cvar->next;
	}

	return lisp_make_error("SET-CVAR -- Unknown CVAR", context);
}

static lisp_data_t *prim_get_cvar(const lisp_data_t *list, lisp_ctx_t *context) {
	lisp_cvar_list_t *cvar = context->the_cvars;
	lisp_data_t *var;
	char *var_name;

	if(lisp_list_length(list) != 1)
		return lisp_make_error("GET-CVAR -- Expected one operand", context);
	if((var = lisp_car(list)) == NULL)
		return lisp_make_error("GET-CVAR -- Expected identifier", context);

	if(var->type != lisp_type_symbol)
		return lisp_make_error("GET-CVAR -- Expected identifier", context);
	var_name = var->symbol;

	while(cvar) {
		if(!strcmp(cvar->name, var_name))
			return lisp_make_int(*(cvar->value), context);
		cvar = cvar->next;
	}
	
	return lisp_make_error("GET-CVAR -- Unknown CVAR", context);
}

/* --- */

static lisp_data_t *primitive_procedure_names(lisp_ctx_t *context) {
	lisp_prim_proc_list_t *curr_proc = context->the_last_prim_proc;
	lisp_data_t *out = NULL;

	while(curr_proc) {
		out = lisp_cons(lisp_make_symbol(curr_proc->name, context), out);
		curr_proc = curr_proc->prev;
	}

	return out;
}

static lisp_data_t *primitive_procedure_objects(lisp_ctx_t *context) {
	lisp_prim_proc_list_t *curr_proc = context->the_last_prim_proc;
	lisp_data_t *out = NULL;

	while(curr_proc) {
		out = lisp_cons(lisp_cons(lisp_make_symbol("primitive", context), lisp_cons(lisp_make_prim(curr_proc->proc, context), NULL)), out);
		curr_proc = curr_proc->prev;
	}

	return out;
}

void lisp_add_prim_proc(char *name, lisp_prim_proc proc, lisp_ctx_t *context) {
	lisp_prim_proc_list_t *curr_proc;

	if(context->the_last_prim_proc == NULL) {
		context->the_prim_procs = malloc(sizeof(lisp_prim_proc_list_t));
		context->the_prim_procs->name = malloc(strlen(name) + 1);
		strcpy(context->the_prim_procs->name, name);
		context->the_prim_procs->proc = proc;
		context->the_prim_procs->next = NULL;
		context->the_prim_procs->prev = NULL;
		context->the_last_prim_proc = context->the_prim_procs;
		return;
	}
	
	curr_proc = malloc(sizeof(lisp_prim_proc_list_t));
	curr_proc->name = malloc(strlen(name) + 1);
	strcpy(curr_proc->name, name);
	curr_proc->proc = proc;
	curr_proc->prev = context->the_last_prim_proc;
	curr_proc->next = NULL;

	context->the_last_prim_proc->next = curr_proc;
	context->the_last_prim_proc = curr_proc;
}

void lisp_add_cvar(const char *name, const size_t *valptr, const int access, lisp_ctx_t *context) {
	lisp_cvar_list_t *curr_var;

	if(context->the_last_cvar == NULL) {
		context->the_cvars = malloc(sizeof(lisp_cvar_list_t));
		context->the_cvars->name = malloc(strlen(name) + 1);
		strcpy(context->the_cvars->name, name);
		context->the_cvars->value = (size_t*)valptr;
		context->the_cvars->access = (int)access;
		context->the_cvars->next = NULL;
		context->the_last_cvar = context->the_cvars;
		return;
	}
	
	curr_var = malloc(sizeof(lisp_cvar_list_t));
	curr_var->name = malloc(strlen(name) + 1);
	strcpy(curr_var->name, name);
	curr_var->value = (size_t*)valptr;
	curr_var->access = (int)access;
	curr_var->next = NULL;

	context->the_last_cvar->next = curr_var;
	context->the_last_cvar = curr_var;
}

static void add_builtin_prim_procs(lisp_ctx_t *context) {
	lisp_add_prim_proc("+", prim_add, context);
	lisp_add_prim_proc("*", prim_mul, context);
	lisp_add_prim_proc("-", prim_sub, context);
	lisp_add_prim_proc("/", prim_div, context);
	lisp_add_prim_proc("=", prim_comp_eq, context);
	lisp_add_prim_proc("<", prim_comp_less, context);
	lisp_add_prim_proc(">", prim_comp_more, context);
	lisp_add_prim_proc("or", prim_or, context);
	lisp_add_prim_proc("and", prim_and, context);
	lisp_add_prim_proc("not", prim_not, context);
	lisp_add_prim_proc("floor", prim_floor, context);
	lisp_add_prim_proc("ceiling", prim_ceiling, context);
	lisp_add_prim_proc("truncate", prim_trunc, context);
	lisp_add_prim_proc("round", prim_round, context);
	lisp_add_prim_proc("max", prim_max, context);
	lisp_add_prim_proc("min", prim_min, context);
	lisp_add_prim_proc("eq?", prim_eq, context);
	lisp_add_prim_proc("car", prim_car, context);
	lisp_add_prim_proc("cdr", prim_cdr, context);
	lisp_add_prim_proc("set-car!", prim_set_car, context);
	lisp_add_prim_proc("set-cdr!", prim_set_cdr, context);
	lisp_add_prim_proc("cons", prim_cons, context);
	lisp_add_prim_proc("list", prim_list, context);
	lisp_add_prim_proc("number?", prim_is_num, context);
	lisp_add_prim_proc("real?", prim_is_num, context);
	lisp_add_prim_proc("integer?", prim_is_int, context);
	lisp_add_prim_proc("procedure?", prim_is_proc, context);
	lisp_add_prim_proc("symbol->string", prim_sym_to_str, context);
	lisp_add_prim_proc("string->symbol", prim_str_to_sym, context);
	lisp_add_prim_proc("symbol?", prim_is_sym, context);
	lisp_add_prim_proc("string?", prim_is_str, context);
	lisp_add_prim_proc("pair?", prim_is_pair, context);
	lisp_add_prim_proc("gcd", prim_gcd, context);
	lisp_add_prim_proc("lcm", prim_lcm, context);

	lisp_add_prim_proc("sin", prim_sin, context);
	lisp_add_prim_proc("cos", prim_cos, context);
	lisp_add_prim_proc("tan", prim_tan, context);
	lisp_add_prim_proc("asin", prim_asin, context);
	lisp_add_prim_proc("acos", prim_acos, context);
	lisp_add_prim_proc("atan", prim_atan, context);
	lisp_add_prim_proc("log", prim_log, context);
	lisp_add_prim_proc("exp", prim_exp, context);
	lisp_add_prim_proc("expt", prim_expt, context);

	lisp_add_prim_proc("set-cvar!", prim_set_cvar, context);
	lisp_add_prim_proc("get-cvar", prim_get_cvar, context);
}

void lisp_setup_env(lisp_ctx_t *context) {
	lisp_data_t *the_empty_environment = lisp_cons(lisp_cons(NULL, NULL), NULL);

	lisp_add_cvar("mem_lim_hard", &context->mem_lim_hard, LISP_CVAR_RO, context);
	lisp_add_cvar("mem_lim_soft", &context->mem_lim_soft, LISP_CVAR_RO, context);
	lisp_add_cvar("mem_list_entries", &context->mem_list_entries, LISP_CVAR_RO, context);
	lisp_add_cvar("mem_verbosity", &context->mem_verbosity, LISP_CVAR_RW, context);
	lisp_add_cvar("mem_allocated", &context->mem_allocated, LISP_CVAR_RO, context);
	lisp_add_cvar("thread_timeout", &context->thread_timeout, LISP_CVAR_RW, context);

	context->the_global_environment = 
		extend_environment(primitive_procedure_names(context), 
						   primitive_procedure_objects(context),
						   the_empty_environment, context);

	lisp_run("(define (caar pair) (car (car pair)))", context);
	lisp_run("(define (cadr pair) (car (cdr pair)))", context);
	lisp_run("(define (cdar pair) (cdr (car pair)))", context);
	lisp_run("(define (cddr pair) (cdr (cdr pair)))", context);

	lisp_run("(define (caaar pair) (car (car (car pair))))", context);
	lisp_run("(define (caadr pair) (car (car (cdr pair))))", context);
	lisp_run("(define (cadar pair) (car (cdr (car pair))))", context);
	lisp_run("(define (caddr pair) (car (cdr (cdr pair))))", context);
	lisp_run("(define (cdaar pair) (cdr (car (car pair))))", context);
	lisp_run("(define (cdadr pair) (cdr (car (cdr pair))))", context);
	lisp_run("(define (cddar pair) (cdr (cdr (car pair))))", context);
	lisp_run("(define (cdddr pair) (cdr (cdr (cdr pair))))", context);

	lisp_run("(define (caaaar pair) (car (car (car (car pair)))))", context);
	lisp_run("(define (caaadr pair) (car (car (car (cdr pair)))))", context);
	lisp_run("(define (caadar pair) (car (car (cdr (car pair)))))", context);
	lisp_run("(define (caaddr pair) (car (car (cdr (cdr pair)))))", context);
	lisp_run("(define (cadaar pair) (car (cdr (car (car pair)))))", context);
	lisp_run("(define (cadadr pair) (car (cdr (car (cdr pair)))))", context);
	lisp_run("(define (caddar pair) (car (cdr (cdr (car pair)))))", context);
	lisp_run("(define (cadddr pair) (car (cdr (cdr (cdr pair)))))", context);
	lisp_run("(define (cdaaar pair) (cdr (car (car (car pair)))))", context);
	lisp_run("(define (cdaadr pair) (cdr (car (car (cdr pair)))))", context);
	lisp_run("(define (cdadar pair) (cdr (car (cdr (car pair)))))", context);
	lisp_run("(define (cdaddr pair) (cdr (car (cdr (cdr pair)))))", context);
	lisp_run("(define (cddaar pair) (cdr (cdr (car (car pair)))))", context);
	lisp_run("(define (cddadr pair) (cdr (cdr (car (cdr pair)))))", context);
	lisp_run("(define (cdddar pair) (cdr (cdr (cdr (car pair)))))", context);
	lisp_run("(define (cddddr pair) (cdr (cdr (cdr (cdr pair)))))", context);

	lisp_run("(define nil '())", context);
	lisp_run("(define (zero? exp) (= 0 exp))", context);
	lisp_run("(define (null? exp) (eq? exp nil))", context);
	lisp_run("(define (negative? exp) (< exp 0))", context);
	lisp_run("(define (positive? exp) (> exp 0))", context);
	lisp_run("(define (boolean? exp) (or (eq? exp '#t) (eq? exp '#f)))", context);
	lisp_run("(define (abs n) (if (negative? n) (- 0 n) n))", context);
	lisp_run("(define (<= a b) (not (> a b)))", context);
	lisp_run("(define (>= a b) (not (< a b)))", context);
	lisp_run("(define (map proc items) (if (null? items) nil (cons (proc (car items)) (map proc (cdr items)))))", context);
	lisp_run("(define (fact n) (if (= n 1) 1 (* n (fact (- n 1)))))", context);
	lisp_run("(define (delay proc) (lambda () proc))", context);
	lisp_run("(define (force proc) (proc))", context);
	lisp_run("(define (length list) (define (list-loop part count) (if (null? part) count (list-loop (cdr part) (+ count 1)))) (list-loop list 0))", context);
	lisp_run("(define (modulo num div) (- num (* (floor (/ num div)) div)))", context);
	lisp_run("(define (quotient num div) (truncate (/ num div)))", context);
	lisp_run("(define (remainder num div) (+ (* (quotient num div) div -1) num))", context);
	lisp_run("(define (odd? n) (if (= 1 (modulo n 2)) '#t '#f))", context);
	lisp_run("(define (even? n) (not (odd? n)))", context);
	lisp_run("(define (square n) (* n n))", context);
	lisp_run("(define (average a b) (/ (+ a b) 2))", context);
	lisp_run("(define (sqrt x) (define (good-enough? guess) (< (abs (- (square guess) x)) 0.000001)) (define (improve guess) (average guess (/ x guess))) (define (sqrt-iter guess) (if (good-enough? guess) (abs guess) (sqrt-iter (improve guess)))) (sqrt-iter 1.0))", context);
	lisp_run("(define (append list1 list2) (if (null? list1) list2 (cons (car list1) (append (cdr list1) list2))))", context);

	lisp_gc(LISP_GC_FORCE, context);
}

void lisp_free_context(lisp_ctx_t *context) {
	lisp_prim_proc_list_t *current_proc = context->the_prim_procs, *procbuf;
	lisp_cvar_list_t *current_var = context->the_cvars, *varbuf;

	lisp_gc(LISP_GC_FORCE, context);
	lisp_free_data_rec(context->the_global_environment, context);

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

lisp_ctx_t *lisp_make_context(const size_t mem_lim_soft, const size_t mem_lim_hard, const size_t mem_verbosity, const size_t thread_timeout) {
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

void lisp_destroy_context(lisp_ctx_t *context) {
	if(context == NULL)
		return;

	lisp_free_context(context);
	lisp_gc_stats(stderr, context);

	free(context);
}