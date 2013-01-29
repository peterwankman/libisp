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

#ifndef BUILTIN_H_
#define BUILTIN_H_

data_t *the_global_env;

void add_prim_proc(char *name, prim_proc proc);
void setup_environment(void);
void cleanup_lisp(void);

#endif
