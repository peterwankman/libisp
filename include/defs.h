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

#ifndef DEFS_H_
#define DEFS_H_

/* MY OTHER CAR IS A CDR */

#define caar(l)		    car(car(l))
#define cadr(l)		    car(cdr(l))
#define cdar(l)		    cdr(car(l))
#define cddr(l)		    cdr(cdr(l))
#define caaar(l)	car(car(car(l)))
#define caadr(l)	car(car(cdr(l)))
#define cadar(l)	car(cdr(car(l)))
#define caddr(l)	car(cdr(cdr(l)))
#define cdaar(l)	cdr(car(car(l)))
#define cdadr(l)	cdr(car(cdr(l)))
#define cddar(l)	cdr(cdr(car(l)))
#define cdddr(l)	cdr(cdr(cdr(l)))

typedef enum dtype_t {
	integer, decimal, string, symbol, pair, prim_procedure
} dtype_t;

typedef struct data_t* (*prim_proc)(const struct data_t*);

typedef struct data_t {
	dtype_t type;
	union {
		int integer;
		double decimal;
		char *string;
		char *symbol;
		prim_proc proc;
		struct cons_t *pair;
	} val;
} data_t;

typedef struct prim_procs {
	char *name;
	prim_proc proc;
	struct prim_procs *next;
	struct prim_procs *prev;
} prim_proc_list_t;

typedef struct cvars {
	char *name;
	int *value;
	struct cvars *next;
} cvar_list_t;

typedef struct cons_t {
	struct data_t *l, *r;
} cons_t;

#endif