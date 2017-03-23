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

#ifndef LIBISP_DEFS_H_
#define LIBISP_DEFS_H_

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
	integer, decimal, string, symbol, pair, prim_procedure, error
} dtype_t;

typedef struct data_t data_t;
typedef struct lisp_ctx_t lisp_ctx_t;

typedef data_t* (*prim_proc)(const data_t*, lisp_ctx_t*);

struct data_t {
	dtype_t type;
	union {
		int integer;
		double decimal;
		char *string;
		char *symbol;
		char *error;
		prim_proc proc;
		struct cons_t *pair;
	};
};

typedef struct prim_procs {
	char *name;
	prim_proc proc;
	struct prim_procs *next;
	struct prim_procs *prev;
} prim_proc_list_t;

typedef struct cvars {
	char *name;
	int access;
	size_t *value;
	struct cvars *next;
} cvar_list_t;

typedef struct cons_t {
	struct data_t *l, *r;
} cons_t;

struct lisp_ctx_t {
	data_t *the_global_environment;
	prim_proc_list_t *the_prim_procs;
	prim_proc_list_t *the_last_prim_proc;

	cvar_list_t *the_cvars;
	cvar_list_t *the_last_cvar;
	
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
