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
#else
#include <pthread.h>
#define ExitThread pthread_exit
#endif

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "libisp/builtin.h"
#include "libisp/data.h"
#include "libisp/mem.h"
#include "libisp/print.h"
#include "libisp/read.h"
#include "libisp/thread.h"

static lisp_data_t *eval(const lisp_data_t *exp, lisp_data_t *env, lisp_ctx_t *context);
static lisp_data_t *set_variable_value(lisp_data_t *var, const lisp_data_t *val, lisp_data_t *env, lisp_ctx_t *context);
static lisp_data_t *lookup_variable_value(const lisp_data_t *var, lisp_data_t *env, lisp_ctx_t *context);

/* HELPER PROCEDURES */

static int is_tagged_list(const lisp_data_t *exp, const char *tag) {
	lisp_data_t *head;
	if(!exp)
		return 0;
	if(exp->type == lisp_type_pair) {
		head = lisp_car(exp);
		if(!head)
			return 0;
		if(head->type != lisp_type_symbol)
			return 0;
		if(!strcmp(lisp_car(exp)->symbol , tag))
			return 1;
	}
	return 0;
}
static int is_self_evaluating(const lisp_data_t *exp) { return (!exp || (exp->type == lisp_type_integer) || (exp->type == lisp_type_decimal) || (exp->type == lisp_type_string)); }
static int is_symbol(const lisp_data_t *exp) { return (exp->type == lisp_type_symbol); }
static int is_variable(const lisp_data_t *exp) { return is_symbol(exp); }
static int is_error(const lisp_data_t *exp) { return (exp && (exp->type == lisp_type_error)); }

/* SEQUENCES */

int is_begin(const lisp_data_t *exp) { return is_tagged_list(exp, "begin"); }
static lisp_data_t *get_begin_actions(const lisp_data_t *exp) { return lisp_cdr(exp); }
int is_last_exp(const lisp_data_t *seq) { return lisp_cdr(seq) == NULL; }
static lisp_data_t *get_first_exp(const lisp_data_t *seq) { return lisp_car(seq); }
static lisp_data_t *get_rest_exps(const lisp_data_t *seq) { return lisp_cdr(seq); }
static lisp_data_t *make_begin(const lisp_data_t *seq, lisp_ctx_t *context) { return lisp_cons(lisp_make_symbol("begin", context), seq); }
static lisp_data_t *sequence_to_exp(const lisp_data_t *seq, lisp_ctx_t *context) {
	if(seq == NULL)
		return NULL;
	if(is_last_exp(seq))
		return get_first_exp(seq);
	return make_begin(seq, context);
}
int has_no_operands(const lisp_data_t *ops) { return ops == NULL; }
static lisp_data_t *get_first_operand(const lisp_data_t *ops) { return lisp_car(ops); }
static lisp_data_t *get_rest_operands(const lisp_data_t *ops) { return lisp_cdr(ops); }
static lisp_data_t *eval_sequence(const lisp_data_t *exps, lisp_data_t *env, lisp_ctx_t *context) {
	if(is_last_exp(exps))
		return eval(get_first_exp(exps), env, context);
	eval(get_first_exp(exps), env, context);
	return eval_sequence(get_rest_exps(exps), env, context);
}

/* LAMBDA */

static int is_lambda(const lisp_data_t *exp) { return is_tagged_list(exp, "lambda"); }
static lisp_data_t *get_lambda_parameters(const lisp_data_t *exp) { return lisp_cadr(exp); }
static lisp_data_t *get_lambda_body(const lisp_data_t *exp) { return lisp_cddr(exp); }
static lisp_data_t *make_lambda(const lisp_data_t *parameters, const lisp_data_t *body, lisp_ctx_t *context) {
	return lisp_cons(lisp_make_symbol("lambda", context), lisp_cons(parameters, body));
}

/* IF */

static int is_if(const lisp_data_t *exp) { return is_tagged_list(exp, "if"); }
static lisp_data_t *get_if_predicate(const lisp_data_t *exp) { return lisp_cadr(exp); }
static lisp_data_t *get_if_consequent(const lisp_data_t *exp) { return lisp_caddr(exp); }
static lisp_data_t *get_if_alternative(const lisp_data_t *exp) {
	if(lisp_cdddr(exp))
		return lisp_car(lisp_cdddr(exp));
	return NULL;
}
static lisp_data_t *make_if(const lisp_data_t *pred, const lisp_data_t *conseq, const lisp_data_t *alt, lisp_ctx_t *context) {
	return lisp_cons(lisp_make_symbol("if", context), lisp_cons(pred, lisp_cons(conseq, lisp_cons(alt, NULL))));
}
static int is_true(const lisp_data_t *x) { return !strcmp(x->symbol, "#t"); }
static int is_false(const lisp_data_t *x) { return strcmp(x->symbol, "#t"); }
static lisp_data_t *eval_if(const lisp_data_t *exp, lisp_data_t *env, lisp_ctx_t *context) {
	if(is_true(eval(get_if_predicate(exp), env, context)))
		return eval(get_if_consequent(exp), env, context);
	return eval(get_if_alternative(exp), env, context);
}

/* COND */

static int is_cond(const lisp_data_t *exp) { return is_tagged_list(exp, "cond"); }
static lisp_data_t *get_cond_clauses(const lisp_data_t *exp) { return lisp_cdr(exp); }
static lisp_data_t *get_cond_predicate(const lisp_data_t *clause) { return lisp_car(clause); }
static int is_cond_else_clause(const lisp_data_t *clause, lisp_ctx_t *context) { return lisp_is_equal(get_cond_predicate(clause), lisp_make_symbol("else", context)); }
static lisp_data_t *get_cond_actions(const lisp_data_t *clause) { return lisp_cdr(clause); }
static lisp_data_t *expand_clauses(const lisp_data_t *clauses, lisp_ctx_t *context) {
	lisp_data_t *first, *rest;

	if(clauses == NULL)
		return lisp_make_symbol("#f", context);

	first = lisp_car(clauses);
	rest = lisp_cdr(clauses);

	if(is_cond_else_clause(first, context)) {
		if(rest == NULL)
			return sequence_to_exp(get_cond_actions(first), context);
		else
			return lisp_make_error("COND-IF -- ELSE clause isn't last", context);
	} 
	return make_if(get_cond_predicate(first), sequence_to_exp(get_cond_actions(first), context), expand_clauses(rest, context), context);
}
static lisp_data_t *cond_to_if(const lisp_data_t *exp, lisp_ctx_t *context) {
	return expand_clauses(get_cond_clauses(exp), context);
}

/* APPLICATIONS */

int is_application(const lisp_data_t *exp) { return exp->type == lisp_type_pair; }
static lisp_data_t *get_operator(const lisp_data_t *exp) { return lisp_car(exp); }
static lisp_data_t *get_operands(const lisp_data_t *exp) { return lisp_cdr(exp); }
static lisp_data_t *get_list_of_values(const lisp_data_t *exps, lisp_data_t *env, lisp_ctx_t *context) {
	if(has_no_operands(exps))
		return NULL;
	return lisp_cons(eval(get_first_operand(exps), env, context), get_list_of_values(get_rest_operands(exps), env, context));
}

/* PROCEDURES */

int is_compound_procedure(const lisp_data_t *exp) { return is_tagged_list(exp, "closure"); }
static int is_primitive_procedure(const lisp_data_t *proc) { return is_tagged_list(proc, "primitive"); }
static lisp_data_t *get_primitive_implementation(const lisp_data_t *proc) { return lisp_cadr(proc); }
static lisp_data_t *get_procedure_body(const lisp_data_t *proc) { return lisp_caddr(proc); }
static lisp_data_t *get_procedure_parameters(const lisp_data_t *proc) { return lisp_cadr(proc); }
static lisp_data_t *get_procedure_environment(const lisp_data_t *proc) { return lisp_car(lisp_cdddr(proc)); }
static lisp_data_t *make_procedure(lisp_data_t *parameters, lisp_data_t *body, lisp_data_t *env, lisp_ctx_t *context) {
	return lisp_cons(lisp_make_symbol("closure", context), lisp_cons(parameters, lisp_cons(body, lisp_cons(env, NULL))));
}
static lisp_data_t *apply_primitive_procedure(const lisp_data_t *proc, const lisp_data_t *args, lisp_ctx_t *context) { return get_primitive_implementation(proc)->proc(args, context); }

/* QUOTATIONS */

static int is_quoted_expression(const lisp_data_t *exp) { return is_tagged_list(exp, "quote"); }
static lisp_data_t *get_text_of_quotation(const lisp_data_t *exp) { return lisp_cadr(exp); }

/* VARIABLE LOOKUP */

static lisp_data_t *get_enclosing_env(lisp_data_t *env) { return lisp_cdr(env); }
static lisp_data_t *get_first_frame(lisp_data_t *env) { return lisp_car(env); }
static lisp_data_t *get_frame_variables(lisp_data_t *frame) { return lisp_car(frame); }
static lisp_data_t *get_frame_values(lisp_data_t *frame) { return lisp_cdr(frame); }
static lisp_data_t *scan_lookup(lisp_data_t *env, const lisp_data_t *vars, const lisp_data_t *vals, const lisp_data_t *var, lisp_ctx_t *context) {
	if(vars == NULL)
		return lookup_variable_value(var, get_enclosing_env(env), context);
	if(lisp_is_equal(var, lisp_car(vars)))
		return lisp_car(vals);
	return scan_lookup(env, lisp_cdr(vars), lisp_cdr(vals), var, context);
}
static lisp_data_t *lookup_variable_value(const lisp_data_t *var, lisp_data_t *env, lisp_ctx_t *context) {
	lisp_data_t *current_frame;

	if(env == NULL)
		return lisp_make_error("LOOKUP -- Unbound variable", context);
		
	current_frame = get_first_frame(env);
	return scan_lookup(env, get_frame_variables(current_frame), get_frame_values(current_frame), var, context);
}

/* ASSIGNMENT */

static int is_assignment(const lisp_data_t *exp) { return is_tagged_list(exp, "set!"); }
static lisp_data_t *get_assignment_variable(const lisp_data_t *exp) { return lisp_cadr(exp); }
static lisp_data_t *get_assignment_value(const lisp_data_t *exp) { return lisp_caddr(exp); }
static lisp_data_t *scan_assignment(lisp_data_t *env, const lisp_data_t *vars, lisp_data_t *vals, lisp_data_t *var, const lisp_data_t *val, lisp_ctx_t *context) {
	if(vars == NULL)
		return set_variable_value(var, val, get_enclosing_env(env), context);
	if(lisp_is_equal(var, lisp_car(vars)))
		return lisp_set_car(vals, val);
	return scan_assignment(env, lisp_cdr(vars), lisp_cdr(vals), var, val, context);
}
static lisp_data_t *set_variable_value(lisp_data_t *var, const lisp_data_t *val, lisp_data_t *env, lisp_ctx_t *context) {
	lisp_data_t *current_frame;

	if(env == NULL)
		return lisp_make_error("SET -- Unbound variable", context);
		
	current_frame = get_first_frame(env);
	return scan_assignment(env, get_frame_variables(current_frame), get_frame_values(current_frame), var, val, context);
}
static lisp_data_t *make_frame(const lisp_data_t *vars, const lisp_data_t *vals, lisp_ctx_t *context) { return lisp_cons(vars, vals); }
static lisp_data_t *eval_assignment(const lisp_data_t *exp, lisp_data_t *env, lisp_ctx_t *context) {
	return set_variable_value(get_assignment_variable(exp), eval(get_assignment_value(exp), env, context), env, context);
}

/* DEFINITION */

static int is_definition(const lisp_data_t *exp) { return is_tagged_list(exp, "define"); }
static lisp_data_t *get_definition_variable(const lisp_data_t *exp) {
	if(is_symbol(lisp_cadr(exp)))
		return lisp_cadr(exp);
	return lisp_caadr(exp);
}
static lisp_data_t *get_definition_value(const lisp_data_t *exp, lisp_ctx_t *context) {
	if(is_symbol(lisp_cadr(exp)))
		return lisp_caddr(exp);
	return make_lambda(lisp_cdadr(exp), lisp_cddr(exp), context);
}
static lisp_data_t *add_binding_to_frame(lisp_data_t *var, const lisp_data_t *val, lisp_data_t *frame, lisp_ctx_t *context) {
	lisp_set_car(frame, (lisp_cons(var, lisp_car(frame))));
	lisp_set_cdr(frame, (lisp_cons(val, lisp_cdr(frame))));
	return (lisp_data_t*)val;
}
static lisp_data_t *scan_define(lisp_data_t *vars, lisp_data_t *vals, lisp_data_t *var, const lisp_data_t *val, lisp_data_t *frame, lisp_ctx_t *context) {
	if(vars == NULL) {
		return add_binding_to_frame(var, val, frame, context);
	} if(lisp_is_equal(var, lisp_car(vars))) {
		lisp_set_car(vals, val);
		return (lisp_data_t*)val;
	}
	return scan_define(lisp_cdr(vars), lisp_cdr(vals), var, val, frame, context);
}
static lisp_data_t *define_variable(lisp_data_t *var, const lisp_data_t *val, lisp_data_t *env, lisp_ctx_t *context) {
	lisp_data_t *frame = get_first_frame(env);
	return scan_define(
		get_frame_variables(frame), 
		get_frame_values(frame), 
		var, 
		val, 
		frame, 
		context);
}
static lisp_data_t *eval_definition(const lisp_data_t *exp, lisp_data_t *env, lisp_ctx_t *context) {	
	return define_variable(get_definition_variable(exp), eval(get_definition_value(exp, context), env, context), env, context);
}

/* LET */

static int is_let(const lisp_data_t *exp) { return is_tagged_list(exp, "let"); }
static lisp_data_t *get_let_assignment(const lisp_data_t *exp) { return lisp_cadr(exp); }
static lisp_data_t *get_let_body(const lisp_data_t *exp) { return lisp_cddr(exp); }
static lisp_data_t *get_let_exp(const lisp_data_t *assignment, lisp_ctx_t *context) {
	if(assignment == NULL)
		return NULL;
	return lisp_cons(lisp_cadar(assignment), get_let_exp(lisp_cdr(assignment), context));
}
static lisp_data_t *get_let_var(const lisp_data_t *assignment, lisp_ctx_t *context) {
	if(assignment == NULL)
		return NULL;
	return lisp_cons(lisp_caar(assignment), get_let_var(lisp_cdr(assignment), context));
}
static lisp_data_t *transform_let(const lisp_data_t *assignment, const lisp_data_t *body, lisp_ctx_t *context) {
	return lisp_cons(make_lambda(get_let_var(assignment, context), body, context), get_let_exp(assignment, context));
}
static lisp_data_t *let_to_combination(const lisp_data_t *exp, lisp_ctx_t *context) {
	return transform_let(get_let_assignment(exp), get_let_body(exp), context);
}

/* LET* */

static int is_let_star(const lisp_data_t *exp) { return is_tagged_list(exp, "let*"); }
static lisp_data_t *get_let_star_assignment(const lisp_data_t *exp) { return lisp_cadr(exp); }
static lisp_data_t *get_let_star_body(const lisp_data_t *exp) { return lisp_cddr(exp); }
static lisp_data_t *transform_let_star(const lisp_data_t *assignment, const lisp_data_t *body, lisp_ctx_t *context) {
	if(lisp_cdr(assignment) == NULL)
		return lisp_cons(lisp_make_symbol("let", context), lisp_cons(assignment, body));
	return lisp_cons(lisp_make_symbol("let", context), 
				lisp_cons(lisp_cons(lisp_car(assignment), NULL),
				lisp_cons(transform_let_star(lisp_cdr(assignment), body, context), NULL)));
}
static lisp_data_t *let_star_to_nested_lets(const lisp_data_t *exp, lisp_ctx_t *context) {
	return transform_let_star(get_let_star_assignment(exp), get_let_star_body(exp), context);
}

/* LETREC */

static int is_letrec(const lisp_data_t *exp) { return is_tagged_list(exp, "letrec"); }
static lisp_data_t *make_unassigned_letrec(const lisp_data_t *vars, lisp_ctx_t *context) {
	if(vars == NULL)
		return NULL;
	return lisp_cons(lisp_cons(lisp_car(vars), lisp_cons(lisp_cons(lisp_make_symbol("quote", context), lisp_cons(lisp_make_symbol("unassigned", context), NULL)), NULL)), make_unassigned_letrec(lisp_cdr(vars), context));
}
static lisp_data_t *make_set_letrec(const lisp_data_t *vars, const lisp_data_t *exps, lisp_ctx_t *context) {
	if(vars == NULL)
		return NULL;
	return lisp_cons(lisp_cons(lisp_make_symbol("set!", context), lisp_cons(lisp_car(vars), lisp_cons(lisp_car(exps), NULL))), make_set_letrec(lisp_cdr(vars), lisp_cdr(exps), context));
}
static lisp_data_t *letrec_to_let(const lisp_data_t *exp, lisp_ctx_t *context) {
	lisp_data_t *assignment = get_let_assignment(exp);
	lisp_data_t *lvars = get_let_var(assignment, context);
	lisp_data_t *lexps = get_let_exp(assignment, context);
	return lisp_cons(lisp_make_symbol("let", context), lisp_cons(make_unassigned_letrec(lvars, context), lisp_append(make_set_letrec(lvars, lexps, context), get_let_body(exp))));
}

/* EVALUATOR PROPER */

lisp_data_t *extend_environment(const lisp_data_t *vars, const lisp_data_t *vals, lisp_data_t *env, lisp_ctx_t *context) {
	int lvars = lisp_list_length(vars), lvals = lisp_list_length(vals);
	if(lvars == lvals)
		return lisp_cons(make_frame(vars, vals, context), env);

	if(lvars < lvals)
		return lisp_make_error("EXTEND -- Too many arguments", context);
	else
		return lisp_make_error("EXTEND -- Too few arguments", context);
}

static lisp_data_t *apply(const lisp_data_t *proc, const lisp_data_t *args, lisp_ctx_t *context) {
	lisp_data_t *out;
	lisp_data_t *argl = args, *currarg;

	while(argl) {
		currarg = lisp_car(argl);
		if(currarg && (currarg->type == lisp_type_error))
			return currarg;
		argl = lisp_cdr(argl);
	}

	if(is_primitive_procedure(proc))
		return apply_primitive_procedure(proc, args, context);
	if(is_compound_procedure(proc)) {
		out = eval_sequence(
			get_procedure_body(proc),
			extend_environment(get_procedure_parameters(proc),
				args,
				get_procedure_environment(proc), context), context);		
		return out;
	}
	return lisp_make_error("APPLY -- Unknown procedure type", context);
}

static lisp_data_t *eval(const lisp_data_t *exp, lisp_data_t *env, lisp_ctx_t *context) {
	if(context->eval_plz_die) {
		context->eval_plz_die = 0;
		ExitThread(0);
	}

	if(is_error(exp))
		return (lisp_data_t*)exp;
	if(is_self_evaluating(exp))
		return (lisp_data_t*)exp;
	if(is_variable(exp))
		return lookup_variable_value(exp, env, context);
	if(is_quoted_expression(exp))
		return get_text_of_quotation(exp);
	if(is_assignment(exp))
		return eval_assignment(exp, env, context);
	if(is_definition(exp))
		return eval_definition(exp, env, context);
	if(is_if(exp))
		return eval_if(exp, env, context);
	if(is_lambda(exp))
		return make_procedure(get_lambda_parameters(exp), get_lambda_body(exp), env, context);
	if(is_begin(exp))
		return eval_sequence(get_begin_actions(exp), env, context);
	if(is_cond(exp))
		return eval(cond_to_if(exp, context), env, context);
	if(is_letrec(exp))
		return eval(letrec_to_let(exp, context), env, context);
	if(is_let_star(exp))
		return eval(let_star_to_nested_lets(exp, context), env, context);
	if(is_let(exp))
		return eval(let_to_combination(exp, context), env, context);
	if(is_application(exp))		
		return apply(
			eval(get_operator(exp), env, context),
			get_list_of_values(get_operands(exp), env, context), context);
	
	return lisp_make_error("EVAL -- Unknown expression type", context);
}

lisp_data_t *lisp_eval(const lisp_data_t *exp, lisp_ctx_t *context) {
	return eval(exp, context->the_global_environment, context);
}

int lisp_run(const char *exp, lisp_ctx_t *context) {
	int error = 0;
	lisp_data_t *exp_list = lisp_read(exp, NULL, &error, context);

	if(error)
		return error;

	lisp_eval_thread(exp_list, context);

	return 0;
}
