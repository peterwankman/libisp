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

#include "defs.h"

#ifndef DATA_H_
#define DATA_H_

data_t *lisp_make_int(const int i);
data_t *lisp_make_decimal(const double d);
data_t *lisp_make_string(const char *str);
data_t *lisp_make_symbol(const char *ident);
data_t *lisp_make_primitive(prim_proc in);

data_t *cons(const data_t *l, const data_t *r);
data_t *car(const data_t *in);
data_t *cdr(const data_t *in);

int is_equal(const data_t *d1, const data_t *d2);
int length(const data_t *list);

data_t *set_car(data_t *pair, const data_t *val);
data_t *set_cdr(data_t *pair, const data_t *val);

#endif