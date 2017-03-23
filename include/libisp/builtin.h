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

#include "defs.h"

#ifndef LISP_BUILTIN_H_
#define LISP_BUILTIN_H_

#define LISP_CVAR_RO	1
#define LISP_CVAR_RW	2

void lisp_add_prim_proc(char *name, lisp_prim_proc proc, lisp_ctx_t *context);
void lisp_add_cvar(const char *name, const size_t *valptr, const int access, lisp_ctx_t *context);
void lisp_setup_env(lisp_ctx_t *context);
void lisp_free_context(lisp_ctx_t *context);
lisp_ctx_t *lisp_make_context(const size_t mem_lim_soft, const size_t mem_lim_hard, const size_t mem_verbosity, const size_t thread_timeout);
void lisp_destroy_context(lisp_ctx_t *context);

#endif
