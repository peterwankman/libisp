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

#include "libisp/defs.h"

#ifndef LIBISP_DATA_H_
#define LIBISP_DATA_H_

data_t *make_int(const int i, lisp_ctx_t *context);
data_t *make_decimal(const double d, lisp_ctx_t *context);
data_t *make_string(const char *str, lisp_ctx_t *context);
data_t *make_symbol(const char *ident, lisp_ctx_t *context);
data_t *make_primitive(prim_proc in, lisp_ctx_t *context);
data_t *make_error(const char *error, lisp_ctx_t *context);

#define cons(l, r) cons_in_context(l, r, context)

data_t *cons_in_context(const data_t *l, const data_t *r, lisp_ctx_t *context);
data_t *car(const data_t *in);
data_t *cdr(const data_t *in);

int is_equal(const data_t *d1, const data_t *d2);
int length(const data_t *list);

data_t *set_car(data_t *pair, const data_t *val);
data_t *set_cdr(data_t *pair, const data_t *val);

data_t *make_copy(const data_t *in);
data_t *append(const data_t *list1, const data_t *list2);

#endif
