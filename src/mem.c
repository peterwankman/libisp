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
#include <stdlib.h>
#include <string.h>

#include "libisp/builtin.h"
#include "libisp/data.h"
#include "libisp/eval.h"
#include "libisp/mem.h"
#include "libisp/thread.h"

typedef struct alloclist_t {
	lisp_data_t *memory;
	char *file;
	int line;
	size_t size;
	char mark;
	struct alloclist_t *next;
} alloclist_t;

/* ALLOCATOR */

static alloclist_t *make_entry(lisp_data_t *memory, const char *file, const int line, const size_t size) {
	alloclist_t *out;
	
	if((out = malloc(sizeof(alloclist_t))) == NULL)
		return NULL;	

	out->memory = memory;
	out->line = line;
	out->file = (char*)file;	
	out->mark = 0;
	out->size = size;
	out->next = NULL;

	return out;
}

static void addtolist(alloclist_t *entry, lisp_ctx_t *context) {
	alloclist_t *current = context->alloc_list, *last = NULL;
	if(!current) {
		context->alloc_list = entry;
		context->mem_list_entries++;		
		return;
	}

	while(current && ((void*)current->memory < (void*)entry->memory)) {
		last = current;		
		current = current->next;
	}

	if(last)
		last->next = entry;
	else
		context->alloc_list = entry;		
	
	entry->next = current;

	context->mem_list_entries++;	
}

lisp_data_t *lisp_dalloc(const size_t size, const char *file, const int line, lisp_ctx_t *context) {
	lisp_data_t *memory;
	size_t newsize = context->mem_allocated + size;
	alloclist_t *newentry;

	if(newsize > context->mem_lim_hard) {
		if(context->thread_running)
			for(;;);
		return NULL;
	} else if(!(context->warned) && (newsize > context->mem_lim_soft)) {
		if(context->mem_verbosity == LISP_GC_VERBOSE)
			fprintf(stderr, "-- WARNING: Soft memory limit reached.\n");
		context->warned = 1;
	} else if((context->warned) && (newsize < context->mem_lim_soft))
		context->warned = 0;

	memory = malloc(size);

	if(memory) {
		if((newentry = make_entry(memory, file, line, size))) {
			addtolist(newentry, context);

			context->mem_allocated += size;
			if(context->mem_allocated > context->n_bytes_peak)
				context->n_bytes_peak = context->mem_allocated;

			context->n_allocs++;
		} else {
			fprintf(stderr, "ERROR: malloc() failed for new memory list entry.\n");
			free(memory);
			return NULL;
		}
	}
	return memory;
}

/* GARBAGE COLLECTOR */

static int delfromlist(const void *memory, lisp_ctx_t *context) {
	alloclist_t *current = context->alloc_list, *last = NULL;

	if(!memory)
		return 0;

	while(current) {
		if((void*)current->memory < (void*)memory) {
			 last = current;
			 current = current->next;
		} else {
			if(last)
				last->next = current->next;
			else
				context->alloc_list = current->next;

			context->mem_allocated -= current->size;
			context->mem_list_entries--;
			free(current);
			return 1;
		}		
	}

	return 0;
}

void lisp_free_data(lisp_data_t *in, lisp_ctx_t *context) {
	if(!in)
		return;

	if(delfromlist(in, context)) {
		if(in->type == lisp_type_string)
			free(in->string);
		if(in->type == lisp_type_symbol)
			free(in->symbol);
		if(in->type == lisp_type_error)
			free(in->error);
		if(in->type == lisp_type_pair)
			free(in->pair);

		free(in);
		context->n_frees++;
	} else {
		fprintf(stderr, "-- WARNING: Called free() on unknown pointer.\n");
	}
}

static void clear_mark(lisp_ctx_t *context) {
	alloclist_t *current = context->alloc_list;

	while(current) {
		current->mark = 0;			
		current = current->next;
	}
}

static alloclist_t *find_in_list(const void *memory, lisp_ctx_t *context) {
	alloclist_t *current =  context->alloc_list;

	if(!memory)
		return NULL;

	while(current) {
		if(current->memory == memory)
			return current;
		current = current->next;
	}

	return NULL;
}

static void mark(lisp_data_t *start, lisp_ctx_t *context) {
	alloclist_t *list_entry;
	lisp_data_t *head, *tail;

	if(!start)
		return;

	list_entry = find_in_list(start, context);

	if(!list_entry) {
		fprintf(stderr, "ERROR: %p not found in memory list.\n", start);
		return;
	}

	if(list_entry->mark == 0) {
		list_entry->mark = 1;
		
		if(start->type == lisp_type_pair) {
			head = lisp_car(start);
			tail = lisp_cdr(start);
			mark(head, context);
			mark(tail, context);
		}
	} 
}

static void sweep(const int req_mark, lisp_ctx_t *context) {
	alloclist_t *current = context->alloc_list, *buf;

	while(current) {
		buf = current->next;
		if(current->mark == req_mark)
			lisp_free_data(current->memory, context);		
		current = buf;
	}
}

size_t lisp_gc(const int force, lisp_ctx_t *context) {
	size_t old_mem = context->mem_allocated;

	if((force == LISP_GC_FORCE) || (context->mem_allocated > context->mem_lim_soft)) {
		clear_mark(context);
		mark(context->the_global_environment, context);
		sweep(0, context);
	}

	return old_mem - context->mem_allocated;
}

/* FREE */

void lisp_free_data_rec(lisp_data_t *in, lisp_ctx_t *context) {
	clear_mark(context);
	mark(in, context);
	sweep(1, context);
}

/* INFO */

void lisp_gc_stats(FILE *fp, lisp_ctx_t *context) {
	alloclist_t *current = context->alloc_list, *buf;

	if((context->n_allocs != context->n_frees) || (context->mem_verbosity == LISP_GC_VERBOSE)) {
		printf("\n--- Memory usage summary ---\n");
		if(context->n_frees < context->n_allocs) {
			fprintf(fp, "Showing unfreed memory:\n");
			while(current) {
				fprintf(fp, "%s, %d\n", current->file, current->line);
				buf = current;
				current = current->next;
				free(buf);
			}
		}

		fprintf(fp, "%lu allocs; %lu frees.\n", context->n_allocs, context->n_frees);
		if(context->mem_list_entries)
			printf("%lu list entries left.\n", context->mem_list_entries);
		printf("--- End summary ---\n");
	}

	if(context->mem_allocated)
		fprintf(fp, "Bytes left allocated: %lu out of ", context->mem_allocated);
	if((context->mem_verbosity == LISP_GC_VERBOSE) || context->mem_allocated)
		fprintf(fp, "%lu bytes peak memory usage.\n", context->n_bytes_peak);
}
