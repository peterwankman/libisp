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

#ifndef EVAL_H_
#define EVAL_H_

data_t *extend_environment(const data_t *vars, const data_t *vals, data_t *env);
data_t *apply(const data_t *proc, const data_t *args);
data_t *eval(data_t *exp, data_t *env);

#endif