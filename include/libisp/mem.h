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

#include <stdio.h>

#include "libisp/defs.h"

#ifndef LISP_MEM_H_
#define LISP_MEM_H_

#define LISP_GC_SILENT	0
#define LISP_GC_VERBOSE	1
#define LISP_GC_LOWMEM	0
#define LISP_GC_FORCE	1
#define lisp_data_alloc(n, c) lisp_dalloc(n, __FILE__, __LINE__, c)

lisp_data_t *lisp_dalloc(const size_t size, const char *file, const int line, lisp_ctx_t *context);
void lisp_gc_stats(FILE *fp, lisp_ctx_t *context);
void lisp_free_data(lisp_data_t *in, lisp_ctx_t *context);
void lisp_free_data_rec(lisp_data_t *in, lisp_ctx_t *context);
size_t lisp_gc(const int force, lisp_ctx_t *context);

#endif
