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

#include "libisp/defs.h"

#ifndef LISP_DATA_H_
#define LISP_DATA_H_

lisp_data_t *lisp_make_int(const int i, lisp_ctx_t *context);
lisp_data_t *lisp_make_decimal(const double d, lisp_ctx_t *context);
lisp_data_t *lisp_make_string(const char *str, lisp_ctx_t *context);
lisp_data_t *lisp_make_symbol(const char *ident, lisp_ctx_t *context);
lisp_data_t *lisp_make_prim(prim_proc in, lisp_ctx_t *context);
lisp_data_t *lisp_make_error(const char *error, lisp_ctx_t *context);

#define cons(l, r) cons_in_context(l, r, context)

lisp_data_t *cons_in_context(const lisp_data_t *l, const lisp_data_t *r, lisp_ctx_t *context);
lisp_data_t *car(const lisp_data_t *in);
lisp_data_t *cdr(const lisp_data_t *in);

int is_equal(const lisp_data_t *d1, const lisp_data_t *d2);
int length(const lisp_data_t *list);

lisp_data_t *set_car(lisp_data_t *pair, const lisp_data_t *val);
lisp_data_t *set_cdr(lisp_data_t *pair, const lisp_data_t *val);

lisp_data_t *make_copy(const lisp_data_t *in);
lisp_data_t *append(const lisp_data_t *list1, const lisp_data_t *list2);

#endif
