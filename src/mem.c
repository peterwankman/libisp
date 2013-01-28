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

#include "builtin.h"
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

/* GARBAGE COLLECTOR */

static int delfromlist(const void *memory) {
	alloclist_t *current = alloc_list, *last = NULL;

	if(!memory)
		return 0;

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
void free_data(data_t *in) {
	if(!in)
		return;

	if(delfromlist(in)) {
		if(in->type == string)
			free(in->val.string);
		if(in->type == symbol)
			free(in->val.symbol);
		if(in->type == pair)
			free(in->val.pair);

		free(in);
		n_frees++;
	} else {
		fprintf(stderr, "WARNING: Called free() on unknown pointer.\n");
	}
}

static void clear_mark(void) {
	alloclist_t *current = alloc_list;

	while(current) {
		current->mark = 0;			
		current = current->next;
	}
}

static alloclist_t *find_in_list(const void *memory) {
	alloclist_t *current =  alloc_list;

	if(!memory)
		return NULL;

	while(current) {
		if(current->memory == memory)
			return current;
		current = current->next;
	}

	return NULL;
}

static void mark(data_t *start) {
	alloclist_t *list_entry = find_in_list(start);
	data_t *head, *tail;

	if(!start)
		return;

	if(list_entry->mark == 0) {
		list_entry->mark = 1;
		
		if(start->type == pair) {
			head = car(start);
			tail = cdr(start);
			mark(head);
			mark(tail);
		}
	} 
}

static void sweep(const int req_mark) {
	alloclist_t *current = alloc_list, *buf;

	while(current) {
		buf = current->next;
		if(current->mark == req_mark)
			free_data(current->memory);		
		current = buf;
	}
}

void run_gc(void) {
	clear_mark();
	mark(the_global_env);
	sweep(0);
}

/* FREE */

void free_data_rec(data_t *in) {
	clear_mark();
	mark(in);
	sweep(1);
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
