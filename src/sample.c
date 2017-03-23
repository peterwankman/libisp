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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libisp.h"

static size_t sample_cvar = 42;

lisp_data_t *sample_proc(const lisp_data_t *args, lisp_ctx_t *context) {
	if(sample_cvar == 17)
		return lisp_make_string("You guessed correctly.", context);
	return lisp_make_string("Try again", context);
}

int main(void) {
	lisp_ctx_t *context;
	lisp_data_t *exp, *ret;
	size_t readto;
	int errcode;

	/************************************************
	 * STEP 1: Initializing the interpreter context *
	 ************************************************/

	/* Create a new empty context */
	context = lisp_make_context(1024 * 768, 1024 * 1024, LISP_GC_VERBOSE, 60);

	/* Add a CVAR and a primitive procedure */
	lisp_add_cvar("my-guess", &sample_cvar, LISP_CVAR_RW, context);
	lisp_add_prim_proc("right?", sample_proc, context);

	/* Finalize the creation of the context */
	lisp_setup_env(context);

	/*******************************
	 * STEP 2: Use the interpreter *
	 *******************************/

	/* Read an expression as a string into a lisp data structure */
	exp = lisp_read("(right?)", &readto, &errcode, context);
	if(errcode == 0) {
		ret = lisp_eval_thread(exp, context);
		lisp_print(ret, context);
		printf("\n");
	} else {
		fprintf(stderr, "lisp_read() failed.\n");
		goto end;
	}

	/* Evaluate some expressions and discard their results */
	lisp_run("(define (sum-of-squares x y) (+ (* x x) (* y y)))", context);
	lisp_run("(set-cvar! 'my-guess 17)", context);

	/* exp still contains the above expression "(right?)" and can be reused. */
	ret = lisp_eval_thread(exp, context);
	lisp_print(ret, context);
	printf("\n");

	/* Builtin and user defined procedures are used absolutely identically. */
	exp = lisp_read("(sqrt (sum-of-squares 3 4))", &readto, &errcode, context);
	if(errcode == 0) {
		ret = lisp_eval_thread(exp, context);
		lisp_print(ret, context);
		printf("\n");
	} else {
		fprintf(stderr, "lisp_read() failed.\n");
	}

	/************************************
	 * STEP 3: Clean up after yourself. *
	 ************************************/
end:
	lisp_destroy_context(context);

	return EXIT_SUCCESS;
}