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

#ifdef _WIN32
#include <Windows.h>
#include <WinBase.h>
#else
#include <pthread.h>
#define HANDLE pthread_t
#endif

#include <time.h>
#include <stdio.h>

#include "libisp/defs.h"
#include "libisp/eval.h"
#include "libisp/mem.h"

typedef struct {
	lisp_data_t *exp, *result;
	lisp_ctx_t *context;
} threadparam_t;

#ifdef _WIN32
static DWORD WINAPI thread(LPVOID in) {
#else
static void *thread(void *in) {
#endif
	threadparam_t *param = (threadparam_t*)in;

	param->result = lisp_eval(param->exp, param->context);
	param->context->thread_running = 0;
#ifndef _WIN32
	pthread_exit(NULL);
#endif
	return 0;
}

static void kill_thread(threadparam_t *info, const char *msg, HANDLE thread_handle, lisp_ctx_t *context) {
	context->eval_plz_die = 1;
	fprintf(stderr, "%s", msg);
	info->result = NULL;
	context->thread_running = 0;
}

static HANDLE spawn_thread(threadparam_t *param) {
#ifdef _WIN32
	return CreateThread(NULL, 0, thread, param, 0, NULL);
#else
	pthread_t thread_handle;

	if(pthread_create(&thread_handle, NULL, thread, param)) {
		fprintf(stderr, "ERROR: Could not spawn eval() thread.\n");
		return 0;
	}
	pthread_detach(thread_handle);

	return thread_handle;
#endif
}

lisp_data_t *lisp_eval_thread(const lisp_data_t *exp, lisp_ctx_t *context) {
	HANDLE thread_handle;
	threadparam_t info;
	time_t starttime = time(NULL);
	size_t reclaimed;

	info.exp = (lisp_data_t*)exp;
	info.context = context;
	context->thread_running = 1;

	thread_handle = spawn_thread(&info);

	while(context->thread_running) {
		if(context->thread_timeout && (time(NULL) - starttime > context->thread_timeout))
			kill_thread(&info, "-- ERROR: eval() timed out.\n", thread_handle, context);
		if(context->mem_allocated + sizeof(lisp_data_t) >= context->mem_lim_hard) {
			kill_thread(&info, "-- ERROR: Hard memory limit reached.\n", 
				thread_handle, context);
			if((context->mem_verbosity == LISP_GC_VERBOSE) && (reclaimed = lisp_gc(LISP_GC_FORCE, context)))
				printf("-- GC: %zu bytes of memory reclaimed.\n", reclaimed);			
		}
	}

	return info.result;
}
