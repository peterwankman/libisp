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

static data_t *eval(const data_t *exp, data_t *env, lisp_ctx_t *context);
static data_t *set_variable_value(data_t *var, const data_t *val, data_t *env, lisp_ctx_t *context);
static data_t *lookup_variable_value(const data_t *var, data_t *env, lisp_ctx_t *context);

/* HELPER PROCEDURES */

static int is_tagged_list(const data_t *exp, const char *tag) {
	data_t *head;
	if(!exp)
		return 0;
	if(exp->type == pair) {
		head = car(exp);
		if(!head)
			return 0;
		if(head->type != symbol)
			return 0;
		if(!strcmp(car(exp)->symbol , tag))
			return 1;
	}
	return 0;
}
static int is_self_evaluating(const data_t *exp) { return (!exp || (exp->type == integer) || (exp->type == decimal) || (exp->type == string)); }
static int is_symbol(const data_t *exp) { return (exp->type == symbol); }
static int is_variable(const data_t *exp) { return is_symbol(exp); }
static int is_error(const data_t *exp) { return (exp && (exp->type == error)); }

/* SEQUENCES */

int is_begin(const data_t *exp) { return is_tagged_list(exp, "begin"); }
static data_t *get_begin_actions(const data_t *exp) { return cdr(exp); }
int is_last_exp(const data_t *seq) { return cdr(seq) == NULL; }
static data_t *get_first_exp(const data_t *seq) { return car(seq); }
static data_t *get_rest_exps(const data_t *seq) { return cdr(seq); }
static data_t *make_begin(const data_t *seq, lisp_ctx_t *context) { return cons(make_symbol("begin", context), seq); }
static data_t *sequence_to_exp(const data_t *seq, lisp_ctx_t *context) {
	if(seq == NULL)
		return NULL;
	if(is_last_exp(seq))
		return get_first_exp(seq);
	return make_begin(seq, context);
}
int has_no_operands(const data_t *ops) { return ops == NULL; }
static data_t *get_first_operand(const data_t *ops) { return car(ops); }
static data_t *get_rest_operands(const data_t *ops) { return cdr(ops); }
static data_t *eval_sequence(const data_t *exps, data_t *env, lisp_ctx_t *context) {
	if(is_last_exp(exps))
		return eval(get_first_exp(exps), env, context);
	eval(get_first_exp(exps), env, context);
	return eval_sequence(get_rest_exps(exps), env, context);
}

/* LAMBDA */

static int is_lambda(const data_t *exp) { return is_tagged_list(exp, "lambda"); }
static data_t *get_lambda_parameters(const data_t *exp) { return cadr(exp); }
static data_t *get_lambda_body(const data_t *exp) { return cddr(exp); }
static data_t *make_lambda(const data_t *parameters, const data_t *body, lisp_ctx_t *context) {
	return cons(make_symbol("lambda", context), cons(parameters, body));
}

/* IF */

static int is_if(const data_t *exp) { return is_tagged_list(exp, "if"); }
static data_t *get_if_predicate(const data_t *exp) { return cadr(exp); }
static data_t *get_if_consequent(const data_t *exp) { return caddr(exp); }
static data_t *get_if_alternative(const data_t *exp) {
	if(cdddr(exp))
		return car(cdddr(exp));
	return NULL;
}
static data_t *make_if(const data_t *pred, const data_t *conseq, const data_t *alt, lisp_ctx_t *context) {
	return cons(make_symbol("if", context), cons(pred, cons(conseq, cons(alt, NULL))));
}
static int is_true(const data_t *x) { return !strcmp(x->symbol, "#t"); }
static int is_false(const data_t *x) { return strcmp(x->symbol, "#t"); }
static data_t *eval_if(const data_t *exp, data_t *env, lisp_ctx_t *context) {
	if(is_true(eval(get_if_predicate(exp), env, context)))
		return eval(get_if_consequent(exp), env, context);
	return eval(get_if_alternative(exp), env, context);
}

/* COND */

static int is_cond(const data_t *exp) { return is_tagged_list(exp, "cond"); }
static data_t *get_cond_clauses(const data_t *exp) { return cdr(exp); }
static data_t *get_cond_predicate(const data_t *clause) { return car(clause); }
static int is_cond_else_clause(const data_t *clause, lisp_ctx_t *context) { return is_equal(get_cond_predicate(clause), make_symbol("else", context)); }
static data_t *get_cond_actions(const data_t *clause) { return cdr(clause); }
static data_t *expand_clauses(const data_t *clauses, lisp_ctx_t *context) {
	data_t *first, *rest;

	if(clauses == NULL)
		return make_symbol("#f", context);

	first = car(clauses);
	rest = cdr(clauses);

	if(is_cond_else_clause(first, context)) {
		if(rest == NULL)
			return sequence_to_exp(get_cond_actions(first), context);
		else
			return make_error("COND-IF -- ELSE clause isn't last", context);
	} 
	return make_if(get_cond_predicate(first), sequence_to_exp(get_cond_actions(first), context), expand_clauses(rest, context), context);
}
static data_t *cond_to_if(const data_t *exp, lisp_ctx_t *context) {
	return expand_clauses(get_cond_clauses(exp), context);
}

/* APPLICATIONS */

int is_application(const data_t *exp) { return exp->type == pair; }
static data_t *get_operator(const data_t *exp) { return car(exp); }
static data_t *get_operands(const data_t *exp) { return cdr(exp); }
static data_t *get_list_of_values(const data_t *exps, data_t *env, lisp_ctx_t *context) {
	if(has_no_operands(exps))
		return NULL;
	return cons(eval(get_first_operand(exps), env, context), get_list_of_values(get_rest_operands(exps), env, context));
}

/* PROCEDURES */

int is_compound_procedure(const data_t *exp) { return is_tagged_list(exp, "closure"); }
static int is_primitive_procedure(const data_t *proc) { return is_tagged_list(proc, "primitive"); }
static data_t *get_primitive_implementation(const data_t *proc) { return cadr(proc); }
static data_t *get_procedure_body(const data_t *proc) { return caddr(proc); }
static data_t *get_procedure_parameters(const data_t *proc) { return cadr(proc); }
static data_t *get_procedure_environment(const data_t *proc) { return car(cdddr(proc)); }
static data_t *make_procedure(data_t *parameters, data_t *body, data_t *env, lisp_ctx_t *context) {
	return cons(make_symbol("closure", context), cons(parameters, cons(body, cons(env, NULL))));
}
static data_t *apply_primitive_procedure(const data_t *proc, const data_t *args, lisp_ctx_t *context) { return get_primitive_implementation(proc)->proc(args, context); }

/* QUOTATIONS */

static int is_quoted_expression(const data_t *exp) { return is_tagged_list(exp, "quote"); }
static data_t *get_text_of_quotation(const data_t *exp) { return cadr(exp); }

/* VARIABLE LOOKUP */

static data_t *get_enclosing_env(data_t *env) { return cdr(env); }
static data_t *get_first_frame(data_t *env) { return car(env); }
static data_t *get_frame_variables(data_t *frame) { return car(frame); }
static data_t *get_frame_values(data_t *frame) { return cdr(frame); }
static data_t *scan_lookup(data_t *env, const data_t *vars, const data_t *vals, const data_t *var, lisp_ctx_t *context) {
	if(vars == NULL)
		return lookup_variable_value(var, get_enclosing_env(env), context);
	if(is_equal(var, car(vars)))
		return car(vals);
	return scan_lookup(env, cdr(vars), cdr(vals), var, context);
}
static data_t *lookup_variable_value(const data_t *var, data_t *env, lisp_ctx_t *context) {
	data_t *current_frame;

	if(env == NULL)
		return make_error("LOOKUP -- Unbound variable", context);
		
	current_frame = get_first_frame(env);
	return scan_lookup(env, get_frame_variables(current_frame), get_frame_values(current_frame), var, context);
}

/* ASSIGNMENT */

static int is_assignment(const data_t *exp) { return is_tagged_list(exp, "set!"); }
static data_t *get_assignment_variable(const data_t *exp) { return cadr(exp); }
static data_t *get_assignment_value(const data_t *exp) { return caddr(exp); }
static data_t *scan_assignment(data_t *env, const data_t *vars, data_t *vals, data_t *var, const data_t *val, lisp_ctx_t *context) {
	if(vars == NULL)
		return set_variable_value(var, val, get_enclosing_env(env), context);
	if(is_equal(var, car(vars)))
		return set_car(vals, val);
	return scan_assignment(env, cdr(vars), cdr(vals), var, val, context);
}
static data_t *set_variable_value(data_t *var, const data_t *val, data_t *env, lisp_ctx_t *context) {
	data_t *current_frame;

	if(env == NULL)
		return make_error("SET -- Unbound variable", context);
		
	current_frame = get_first_frame(env);
	return scan_assignment(env, get_frame_variables(current_frame), get_frame_values(current_frame), var, val, context);
}
static data_t *make_frame(const data_t *vars, const data_t *vals, lisp_ctx_t *context) { return cons(vars, vals); }
static data_t *eval_assignment(const data_t *exp, data_t *env, lisp_ctx_t *context) {
	return set_variable_value(get_assignment_variable(exp), eval(get_assignment_value(exp), env, context), env, context);
}

/* DEFINITION */

static int is_definition(const data_t *exp) { return is_tagged_list(exp, "define"); }
static data_t *get_definition_variable(const data_t *exp) {
	if(is_symbol(cadr(exp)))
		return cadr(exp);
	return caadr(exp);
}
static data_t *get_definition_value(const data_t *exp, lisp_ctx_t *context) {
	if(is_symbol(cadr(exp)))
		return caddr(exp);
	return make_lambda(cdadr(exp), cddr(exp), context);
}
static data_t *add_binding_to_frame(data_t *var, const data_t *val, data_t *frame, lisp_ctx_t *context) {
	set_car(frame, (cons(var, car(frame))));
	set_cdr(frame, (cons(val, cdr(frame))));
	return (data_t*)val;
}
static data_t *scan_define(data_t *vars, data_t *vals, data_t *var, const data_t *val, data_t *frame, lisp_ctx_t *context) {
	if(vars == NULL) {
		return add_binding_to_frame(var, val, frame, context);
	} if(is_equal(var, car(vars))) {
		set_car(vals, val);
		return (data_t*)val;
	}
	return scan_define(cdr(vars), cdr(vals), var, val, frame, context);
}
static data_t *define_variable(data_t *var, const data_t *val, data_t *env, lisp_ctx_t *context) {
	data_t *frame = get_first_frame(env);
	return scan_define(
		get_frame_variables(frame), 
		get_frame_values(frame), 
		var, 
		val, 
		frame, 
		context);
}
static data_t *eval_definition(const data_t *exp, data_t *env, lisp_ctx_t *context) {	
	return define_variable(get_definition_variable(exp), eval(get_definition_value(exp, context), env, context), env, context);
}

/* LET */

static int is_let(const data_t *exp) { return is_tagged_list(exp, "let"); }
static data_t *get_let_assignment(const data_t *exp) { return cadr(exp); }
static data_t *get_let_body(const data_t *exp) { return cddr(exp); }
static data_t *get_let_exp(const data_t *assignment, lisp_ctx_t *context) {
	if(assignment == NULL)
		return NULL;
	return cons(cadar(assignment), get_let_exp(cdr(assignment), context));
}
static data_t *get_let_var(const data_t *assignment, lisp_ctx_t *context) {
	if(assignment == NULL)
		return NULL;
	return cons(caar(assignment), get_let_var(cdr(assignment), context));
}
static data_t *transform_let(const data_t *assignment, const data_t *body, lisp_ctx_t *context) {
	return cons(make_lambda(get_let_var(assignment, context), body, context), get_let_exp(assignment, context));
}
static data_t *let_to_combination(const data_t *exp, lisp_ctx_t *context) {
	return transform_let(get_let_assignment(exp), get_let_body(exp), context);
}

/* LET* */

static int is_let_star(const data_t *exp) { return is_tagged_list(exp, "let*"); }
static data_t *get_let_star_assignment(const data_t *exp) { return cadr(exp); }
static data_t *get_let_star_body(const data_t *exp) { return cddr(exp); }
static data_t *transform_let_star(const data_t *assignment, const data_t *body, lisp_ctx_t *context) {
	if(cdr(assignment) == NULL)
		return cons(make_symbol("let", context), cons(assignment, body));
	return cons(make_symbol("let", context), 
				cons(cons(car(assignment), NULL),
				cons(transform_let_star(cdr(assignment), body, context), NULL)));
}
static data_t *let_star_to_nested_lets(const data_t *exp, lisp_ctx_t *context) {
	return transform_let_star(get_let_star_assignment(exp), get_let_star_body(exp), context);
}

/* LETREC */

static int is_letrec(const data_t *exp) { return is_tagged_list(exp, "letrec"); }
static data_t *make_unassigned_letrec(const data_t *vars, lisp_ctx_t *context) {
	if(vars == NULL)
		return NULL;
	return cons(cons(car(vars), cons(cons(make_symbol("quote", context), cons(make_symbol("unassigned", context), NULL)), NULL)), make_unassigned_letrec(cdr(vars), context));
}
static data_t *make_set_letrec(const data_t *vars, const data_t *exps, lisp_ctx_t *context) {
	if(vars == NULL)
		return NULL;
	return cons(cons(make_symbol("set!", context), cons(car(vars), cons(car(exps), NULL))), make_set_letrec(cdr(vars), cdr(exps), context));
}
static data_t *letrec_to_let(const data_t *exp, lisp_ctx_t *context) {
	data_t *assignment = get_let_assignment(exp);
	data_t *lvars = get_let_var(assignment, context);
	data_t *lexps = get_let_exp(assignment, context);
	return cons(make_symbol("let", context), cons(make_unassigned_letrec(lvars, context), append(make_set_letrec(lvars, lexps, context), get_let_body(exp))));
}

/* EVALUATOR PROPER */

data_t *extend_environment(const data_t *vars, const data_t *vals, data_t *env, lisp_ctx_t *context) {
	int lvars = length(vars), lvals = length(vals);
	if(lvars == lvals)
		return cons(make_frame(vars, vals, context), env);

	if(lvars < lvals)
		return make_error("EXTEND -- Too many arguments", context);
	else
		return make_error("EXTEND -- Too few arguments", context);
}

data_t *apply(const data_t *proc, const data_t *args, lisp_ctx_t *context) {
	data_t *out;
	data_t *argl = args, *currarg;

	while(argl) {
		currarg = car(argl);
		if(currarg && (currarg->type == error))
			return currarg;
		argl = cdr(argl);
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
	return make_error("APPLY -- Unknown procedure type", context);
}

static data_t *eval(const data_t *exp, data_t *env, lisp_ctx_t *context) {
	if(context->eval_plz_die) {
		context->eval_plz_die = 0;
		ExitThread(0);
	}

	if(is_error(exp))
		return (data_t*)exp;
	if(is_self_evaluating(exp))
		return (data_t*)exp;
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
	
	return make_error("EVAL -- Unknown expression type", context);
}

data_t *eval_in_context(const data_t *exp, lisp_ctx_t *context) {
	return eval(exp, context->the_global_environment, context);
}

int run_exp(const char *exp, lisp_ctx_t *context) {
	int error = 0;
	data_t *exp_list = read_exp(exp, NULL, &error, context);

	if(error)
		return error;

	eval_thread(exp_list, context);

	return 0;
}
