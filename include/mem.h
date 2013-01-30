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

#include "defs.h"

#ifndef MEM_H_
#define MEM_H_

#define GC_SILENT	0
#define GC_VERBOSE	1
#define GC_LOWMEM	0
#define GC_FORCE	1
#define lisp_data_alloc(n) _dalloc(n, __FILE__, __LINE__)

size_t mem_lim_soft;
size_t mem_lim_hard;
int mem_verbosity;

data_t *_dalloc(const size_t size, const char *file, const int line);
void showmemstats(FILE *fp);
void free_data(data_t *in);
void free_data_rec(data_t *in);
size_t run_gc(int force);

#endif