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

#ifndef LIBISP_READ_H_
#define LIBISP_READ_H_

data_t *read_exp(const char *exp, size_t *readto, int *error, lisp_ctx_t *context);

#endif
