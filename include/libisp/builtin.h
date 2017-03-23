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

#ifndef LIBISP_BUILTIN_H_
#define LIBISP_BUILTIN_H_

#define CVAR_READONLY	1
#define CVAR_READWRITE	2

void add_prim_proc(char *name, prim_proc proc, lisp_ctx_t *context);
void add_cvar(const char *name, const size_t *valptr, const int access, lisp_ctx_t *context);
void setup_environment(lisp_ctx_t *context);
void cleanup_lisp(lisp_ctx_t *context);
void reset_lisp(lisp_ctx_t *context);
lisp_ctx_t *make_context(const size_t mem_lim_soft, const size_t mem_lim_hard, const size_t mem_verbosity, const size_t thread_timeout);
void destroy_context(lisp_ctx_t *context);

#endif
