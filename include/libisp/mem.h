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

#include <stdio.h>

#include "libisp/defs.h"

#ifndef LIBISP_MEM_H_
#define LIBISP_MEM_H_

#define MEM_SILENT	0
#define MEM_VERBOSE	1
#define GC_LOWMEM	0
#define GC_FORCE	1
#define lisp_data_alloc(n, c) _dalloc(n, __FILE__, __LINE__, c)

/*
size_t mem_lim_soft;
size_t mem_lim_hard;
size_t mem_list_entries;
size_t mem_allocated;
size_t mem_verbosity;
*/

data_t *_dalloc(const size_t size, const char *file, const int line, lisp_ctx_t *context);
void showmemstats(FILE *fp, lisp_ctx_t *context);
void free_data(data_t *in, lisp_ctx_t *context);
void free_data_rec(data_t *in, lisp_ctx_t *context);
size_t run_gc(const int force, lisp_ctx_t *context);

#endif
