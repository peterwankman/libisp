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

data_t *sample_proc(const data_t *args, lisp_ctx_t *context) {
	if(sample_cvar == 17)
		return make_string("You guessed correctly.", context);
	return make_string("Try again", context);
}

int main(void) {
	lisp_ctx_t *context;
	data_t *exp, *ret;
	size_t readto;
	int errcode;

	/************************************************
	 * STEP 1: Initializing the interpreter context *
	 ************************************************/

	/* Create a new empty context */
	context = make_context(1024 * 768, 1024 * 1024, MEM_VERBOSE, 60);

	/* Add a CVAR and a primitive procedure */
	add_cvar("my-guess", &sample_cvar, CVAR_READWRITE, context);
	add_prim_proc("right?", sample_proc, context);

	/* Finalize the creation of the context */
	setup_environment(context);

	/*******************************
	 * STEP 2: Use the interpreter *
	 *******************************/

	/* Read an expression as a string into a lisp data structure */
	exp = read_exp("(right?)", &readto, &errcode, context);
	if(errcode == 0) {
		ret = eval_thread(exp, context);
		print_data(ret, context);
		printf("\n");
	} else {
		fprintf(stderr, "read_exp() failed.\n");
		goto end;
	}

	/* Evaluate an some expressions and discard their results */
	run_exp("(define (sum-of-squares x y) (+ (* x x) (* y y)))", context);
	run_exp("(set-cvar! 'my-guess 17)", context);

	/* exp still contains the above expression "(right?)" and can be reused. */
	ret = eval_thread(exp, context);
	print_data(ret, context);
	printf("\n");

	/* Builtin and user defined procedures are used absolutely identically. */
	exp = read_exp("(sqrt (sum-of-squares 3 4))", &readto, &errcode, context);
	if(errcode == 0) {
		ret = eval_thread(exp, context);
		print_data(ret, context);
		printf("\n");
	} else {
		fprintf(stderr, "read_exp() failed.\n");
	}

	/************************************
	 * STEP 3: Clean up after yourself. *
	 ************************************/
end:
	destroy_context(context);

	return EXIT_SUCCESS;
}