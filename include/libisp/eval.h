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

#ifndef LISP_EVAL_H_
#define LISP_EVAL_H_

#ifndef LISP_LIBISP_H_

int is_compound_procedure(const lisp_data_t *exp);
lisp_data_t *extend_environment(const lisp_data_t *vars, const lisp_data_t *vals, lisp_data_t *env, lisp_ctx_t *context);

#endif

lisp_data_t *lisp_eval(const lisp_data_t *exp, lisp_ctx_t *context);
int lisp_run(const char *exp, lisp_ctx_t *context);

#endif
