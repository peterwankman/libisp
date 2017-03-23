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

#ifndef LISP_DEFS_H_
#define LISP_DEFS_H_

#include <stdint.h>

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

typedef enum lisp_type_t {
	lisp_type_integer, lisp_type_decimal, lisp_type_string, lisp_type_symbol, lisp_type_pair, lisp_type_prim, lisp_type_error
} lisp_type_t;

typedef struct lisp_data_t lisp_data_t;
typedef struct lisp_ctx_t lisp_ctx_t;

typedef lisp_data_t* (*prim_proc)(const lisp_data_t*, lisp_ctx_t*);

struct lisp_data_t {
	lisp_type_t type;
	union {
		int integer;
		double decimal;
		char *string;
		char *symbol;
		char *error;
		prim_proc proc;
		struct lisp_cons_t *pair;
	};
};

typedef struct lisp_prim_proc_list_t {
	char *name;
	prim_proc proc;
	struct lisp_prim_proc_list_t *next;
	struct lisp_prim_proc_list_t *prev;
} lisp_prim_proc_list_t;

typedef struct lisp_cvar_list_t {
	char *name;
	int access;
	size_t *value;
	struct lisp_cvar_list_t *next;
} lisp_cvar_list_t;

typedef struct lisp_cons_t {
	struct lisp_data_t *l, *r;
} lisp_cons_t;

struct lisp_ctx_t {
	lisp_data_t *the_global_environment;
	lisp_prim_proc_list_t *the_prim_procs;
	lisp_prim_proc_list_t *the_last_prim_proc;

	lisp_cvar_list_t *the_cvars;
	lisp_cvar_list_t *the_last_cvar;
	
	size_t mem_lim_soft;
	size_t mem_lim_hard;
	size_t mem_list_entries;
	size_t mem_allocated;
	size_t mem_verbosity;
	size_t n_allocs;
	size_t n_frees;
	size_t n_bytes_peak;
	size_t warned;
	struct alloclist_t *alloc_list;

	size_t thread_timeout;
	int thread_running;
	int eval_plz_die;
};

#endif
