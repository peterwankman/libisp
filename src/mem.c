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

#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "eval.h"

static int n_allocs = 0;
static int n_frees = 0;

typedef struct alloclist_t {
	data_t *memory;
	char *file;
	int line;
	char mark;
	struct alloclist_t *next;
} alloclist_t;

static alloclist_t *alloc_list = NULL;

/* ALLOCATOR */

static alloclist_t *make_entry(data_t *memory, const char *file, const int line) {
	alloclist_t *out;
	
	if((out = (alloclist_t*)malloc(sizeof(alloclist_t))) == NULL)
		return NULL;	

	out->memory = memory;
	out->line = line;
	out->file = (char*)file;	
	out->next = NULL;

	return out;
}

static void addtolist(const alloclist_t *entry) {
	alloclist_t *current = alloc_list, *last = NULL;
	if(!current) {
		alloc_list = (alloclist_t*)entry;
		return;
	}

	while(current) {		
		last = current;
		current = current->next;
	}
	last->next = (alloclist_t*)entry;
}

data_t *_dalloc(const size_t size, const char *file, const int line) {
	data_t *memory = (data_t*)malloc(size);
	alloclist_t *newentry;	

	if((newentry = make_entry(memory, file, line)))
		addtolist(newentry);

	n_allocs++;
	return memory;
}

/* FREE */

static int delfromlist(const void *memory) {
	alloclist_t *current = alloc_list, *last = NULL;

	while(current) {
		if(current->memory == memory) {
			if(last) {
				last->next = current->next;
			} else {
				alloc_list = current->next;
			}
			free(current);
			return 1;
		}
		last = current;
		current = current->next;
	}

	return 0;
}

static void dfree(void *memory) {
	if(delfromlist(memory)) {
		n_frees++;
		free(memory);
	} else {
		fprintf(stderr, "WARNING: Called free() on unknown pointer.\n");
	}
}

void free_data(data_t *in) {
	if(in->type == string)
		free(in->val.string);
	if(in->type == symbol)
		free(in->val.symbol);
	if(in->type == pair)
		free(in->val.pair);
	dfree(in);
}

void free_data_rec(data_t *in) {
	if(in->type != pair) {
		free_data(in);
		return;
	} else {
		free_data_rec(car(in));
		free_data_rec(cdr(in));

		free_data(in);
	}
}

/* GARBAGE COLLECTOR */

static void clear_mark(void) {
	alloclist_t *current = alloc_list;

	while(current) {
		current->mark = 0;			
		current = current->next;
	}
}

static void mark(void); /* TBI */

static void sweep(void) {
	alloclist_t *current = alloc_list, *buf;

	while(current) {
		buf = current;
		if(!current->mark)
			free_data(current->memory);		
		current = buf->next;
	}
}

void run_gc(void) {
	clear_mark();
/*	mark();		*/
	sweep();
}

/* INFO */

void showmemstats(FILE *fp) {
	alloclist_t *current = alloc_list, *buf;

	if(n_allocs != n_frees) {
		printf("\n--- memory.c summary ---\n");
		if(n_frees < n_allocs) {
			fprintf(fp, "Showing unfreed memory:\n");
			while(current) {
				fprintf(fp, "%s, %d\n", current->file, current->line);
				buf = current;
				current = current->next;
				free(buf);
			}
		}

		fprintf(fp, "%d allocs; %d frees.\n", n_allocs, n_frees);
		printf("--- end summary ---\n");
	}
}