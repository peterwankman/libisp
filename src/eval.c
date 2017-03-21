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

int eval_plz_die = 0;

data_t *eval(const data_t *exp, data_t *env);
static data_t *set_variable_value(data_t *var, const data_t *val, data_t *env);
static data_t *lookup_variable_value(const data_t *var, data_t *env);

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

/* SEQUENCES */

int is_begin(const data_t *exp) { return is_tagged_list(exp, "begin"); }
static data_t *get_begin_actions(const data_t *exp) { return cdr(exp); }
int is_last_exp(const data_t *seq) { return cdr(seq) == NULL; }
static data_t *get_first_exp(const data_t *seq) { return car(seq); }
static data_t *get_rest_exps(const data_t *seq) { return cdr(seq); }
static data_t *make_begin(const data_t *seq) { return cons(make_symbol("begin"), seq); }
static data_t *sequence_to_exp(const data_t *seq) {
	if(seq == NULL)
		return NULL;
	if(is_last_exp(seq))
		return get_first_exp(seq);
	return make_begin(seq);
}
int has_no_operands(const data_t *ops) { return ops == NULL; }
static data_t *get_first_operand(const data_t *ops) { return car(ops); }
static data_t *get_rest_operands(const data_t *ops) { return cdr(ops); }
static data_t *eval_sequence(const data_t *exps, data_t *env) {
	if(is_last_exp(exps))
		return eval(get_first_exp(exps), env);
	eval(get_first_exp(exps), env);
	return eval_sequence(get_rest_exps(exps), env);
}

/* LAMBDA */

static int is_lambda(const data_t *exp) { return is_tagged_list(exp, "lambda"); }
static data_t *get_lambda_parameters(const data_t *exp) { return cadr(exp); }
static data_t *get_lambda_body(const data_t *exp) { return cddr(exp); }
static data_t *make_lambda(const data_t *parameters, const data_t *body) {
	return cons(make_symbol("lambda"), cons(parameters, body));
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
static data_t *make_if(const data_t *pred, const data_t *conseq, const data_t *alt) {
	return cons(make_symbol("if"), cons(pred, cons(conseq, cons(alt, NULL))));
}
static int is_true(const data_t *x) { return !strcmp(x->symbol, "#t"); }
static int is_false(const data_t *x) { return strcmp(x->symbol, "#t"); }
static data_t *eval_if(const data_t *exp, data_t *env) {
	if(is_true(eval(get_if_predicate(exp), env)))
		return eval(get_if_consequent(exp), env);
	return eval(get_if_alternative(exp), env);
}

/* COND */

static int is_cond(const data_t *exp) { return is_tagged_list(exp, "cond"); }
static data_t *get_cond_clauses(const data_t *exp) { return cdr(exp); }
static data_t *get_cond_predicate(const data_t *clause) { return car(clause); }
static int is_cond_else_clause(const data_t *clause) { return is_equal(get_cond_predicate(clause), make_symbol("else")); }
static data_t *get_cond_actions(const data_t *clause) { return cdr(clause); }
static data_t *expand_clauses(const data_t *clauses) {
	data_t *first, *rest;

	if(clauses == NULL)
		return make_symbol("#f");

	first = car(clauses);
	rest = cdr(clauses);

	if(is_cond_else_clause(first)) {
		if(rest == NULL) {
			return sequence_to_exp(get_cond_actions(first));
		} else {
			printf("ELSE clause isn't last -- COND-IF");
			return make_symbol("error");
		}
	} 
	return make_if(get_cond_predicate(first), sequence_to_exp(get_cond_actions(first)), expand_clauses(rest));
}
static data_t *cond_to_if(const data_t *exp) {
	return expand_clauses(get_cond_clauses(exp));
}

/* APPLICATIONS */

int is_application(const data_t *exp) { return exp->type == pair; }
static data_t *get_operator(const data_t *exp) { return car(exp); }
static data_t *get_operands(const data_t *exp) { return cdr(exp); }
static data_t *get_list_of_values(const data_t *exps, data_t *env) {
	if(has_no_operands(exps))
		return NULL;
	return cons(eval(get_first_operand(exps), env), get_list_of_values(get_rest_operands(exps), env));
}

/* PROCEDURES */

int is_compound_procedure(const data_t *exp) { return is_tagged_list(exp, "closure"); }
static int is_primitive_procedure(const data_t *proc) { return is_tagged_list(proc, "primitive"); }
static data_t *get_primitive_implementation(const data_t *proc) { return cadr(proc); }
static data_t *get_procedure_body(const data_t *proc) { return caddr(proc); }
static data_t *get_procedure_parameters(const data_t *proc) { return cadr(proc); }
static data_t *get_procedure_environment(const data_t *proc) { return car(cdddr(proc)); }
static data_t *make_procedure(data_t *parameters, data_t *body, data_t *env) {
	return cons(make_symbol("closure"), cons(parameters, cons(body, cons(env, NULL))));
}
static data_t *apply_primitive_procedure(const data_t *proc, const data_t *args) { return get_primitive_implementation(proc)->proc(args); }

/* QUOTATIONS */

static int is_quoted_expression(const data_t *exp) { return is_tagged_list(exp, "quote"); }
static data_t *get_text_of_quotation(const data_t *exp) { return cadr(exp); }

/* VARIABLE LOOKUP */

static data_t *get_enclosing_env(data_t *env) { return cdr(env); }
static data_t *get_first_frame(data_t *env) { return car(env); }
static data_t *get_frame_variables(data_t *frame) { return car(frame); }
static data_t *get_frame_values(data_t *frame) { return cdr(frame); }
static data_t *scan_lookup(data_t *env, const data_t *vars, const data_t *vals, const data_t *var) {
	if(vars == NULL)
		return lookup_variable_value(var, get_enclosing_env(env));
	if(is_equal(var, car(vars))) {		
		return car(vals);
	}	
	return scan_lookup(env, cdr(vars), cdr(vals), var);
}
static data_t *lookup_variable_value(const data_t *var, data_t *env) {
	data_t *current_frame;

	if(env == NULL) {
		printf("Unbound variable -- LOOKUP ");
		print_data(var);
		printf("\n");
		return make_symbol("error");
	}
		
	current_frame = get_first_frame(env);
	return scan_lookup(env, get_frame_variables(current_frame), get_frame_values(current_frame), var);
}

/* ASSIGNMENT */

static int is_assignment(const data_t *exp) { return is_tagged_list(exp, "set!"); }
static data_t *get_assignment_variable(const data_t *exp) { return cadr(exp); }
static data_t *get_assignment_value(const data_t *exp) { return caddr(exp); }
static data_t *scan_assignment(data_t *env, const data_t *vars, data_t *vals, data_t *var, const data_t *val) {
	if(vars == NULL)
		return set_variable_value(var, val, get_enclosing_env(env));
	if(is_equal(var, car(vars)))
		return set_car(vals, val);
	return scan_assignment(env, cdr(vars), cdr(vals), var, val);
}
static data_t *set_variable_value(data_t *var, const data_t *val, data_t *env) {
	data_t *current_frame;

	if(env == NULL) {
		printf("Unbound variable -- SET!\n");
		return make_symbol("error");
	}
		
	current_frame = get_first_frame(env);
	return scan_assignment(env, get_frame_variables(current_frame), get_frame_values(current_frame), var, val);
}
static data_t *make_frame(const data_t *vars, const data_t *vals) { return cons(vars, vals); }
static data_t *eval_assignment(const data_t *exp, data_t *env) {
	return set_variable_value(get_assignment_variable(exp), eval(get_assignment_value(exp), env), env);
}

/* DEFINITION */

static int is_definition(const data_t *exp) { return is_tagged_list(exp, "define"); }
static data_t *get_definition_variable(const data_t *exp) {
	if(is_symbol(cadr(exp)))
		return cadr(exp);
	return caadr(exp);
}
static data_t *get_definition_value(const data_t *exp) {
	if(is_symbol(cadr(exp)))
		return caddr(exp);
	return make_lambda(cdadr(exp), cddr(exp));
}
static data_t *add_binding_to_frame(data_t *var, const data_t *val, data_t *frame) {
	set_car(frame, (cons(var, car(frame))));
	set_cdr(frame, (cons(val, cdr(frame))));
	return (data_t*)val;
}
static data_t *scan_define(data_t *vars, data_t *vals, data_t *var, const data_t *val, data_t *frame) {
	if(vars == NULL) {
		return add_binding_to_frame(var, val, frame);
	} if(is_equal(var, car(vars))) {
		set_car(vals, val);
		return (data_t*)val;
	}
	return scan_define(cdr(vars), cdr(vals), var, val, frame);
}
static data_t *define_variable(data_t *var, const data_t *val, data_t *env) {
	data_t *frame = get_first_frame(env);
	return scan_define(
		get_frame_variables(frame), 
		get_frame_values(frame), 
		var, 
		val, 
		frame);
}
static data_t *eval_definition(const data_t *exp, data_t *env) {	
	return define_variable(get_definition_variable(exp), eval(get_definition_value(exp), env), env);
}

/* LET */

static int is_let(const data_t *exp) { return is_tagged_list(exp, "let"); }
static data_t *get_let_assignment(const data_t *exp) { return cadr(exp); }
static data_t *get_let_body(const data_t *exp) { return cddr(exp); }
static data_t *get_let_exp(const data_t *assignment) {
	if(assignment == NULL)
		return NULL;
	return cons(cadar(assignment), get_let_exp(cdr(assignment)));
}
static data_t *get_let_var(const data_t *assignment) {
	if(assignment == NULL)
		return NULL;
	return cons(caar(assignment), get_let_var(cdr(assignment)));
}
static data_t *transform_let(const data_t *assignment, const data_t *body) {
	return cons(make_lambda(get_let_var(assignment), body), get_let_exp(assignment));
}
static data_t *let_to_combination(const data_t *exp) {
	return transform_let(get_let_assignment(exp), get_let_body(exp));
}

/* LET* */

static int is_let_star(const data_t *exp) { return is_tagged_list(exp, "let*"); }
static data_t *get_let_star_assignment(const data_t *exp) { return cadr(exp); }
static data_t *get_let_star_body(const data_t *exp) { return cddr(exp); }
static data_t *transform_let_star(const data_t *assignment, const data_t *body) {
	if(cdr(assignment) == NULL)
		return cons(make_symbol("let"), cons(assignment, body));
	return cons(make_symbol("let"), 
				cons(cons(car(assignment), NULL),
				cons(transform_let_star(cdr(assignment), body), NULL)));
}
static data_t *let_star_to_nested_lets(const data_t *exp) {
	return transform_let_star(get_let_star_assignment(exp), get_let_star_body(exp));
}

/* LETREC */

static int is_letrec(const data_t *exp) { return is_tagged_list(exp, "letrec"); }
static data_t *make_unassigned_letrec(const data_t *vars) {
	if(vars == NULL)
		return NULL;
	return cons(cons(car(vars), cons(cons(make_symbol("quote"), cons(make_symbol("unassigned"), NULL)), NULL)), make_unassigned_letrec(cdr(vars)));
}
static data_t *make_set_letrec(const data_t *vars, const data_t *exps) {
	if(vars == NULL)
		return NULL;
	return cons(cons(make_symbol("set!"), cons(car(vars), cons(car(exps), NULL))),	make_set_letrec(cdr(vars), cdr(exps)));
}
static data_t *letrec_to_let(const data_t *exp) {
	data_t *assignment = get_let_assignment(exp);
	data_t *lvars = get_let_var(assignment);
	data_t *lexps = get_let_exp(assignment);
	return cons(make_symbol("let"), cons(make_unassigned_letrec(lvars), append(make_set_letrec(lvars, lexps), get_let_body(exp))));
}

/* EVALUATOR PROPER */

data_t *extend_environment(const data_t *vars, const data_t *vals, data_t *env) {
	int lvars = length(vars), lvals = length(vals);
	if(lvars == lvals)
		return cons(make_frame(vars, vals), env);

	if(lvars < lvals) {
		printf("Too many arguments supplied. (Expected %d, got %d)\n", lvars, lvals);
		return make_symbol("error");
	} else {
		printf("Too few arguments supplied (Expected %d, got %d)\n", lvars, lvals);
		return make_symbol("error");
	}
}

data_t *apply(const data_t *proc, const data_t *args) {
	data_t *out;
	data_t *argl = args, *currarg;

	while(argl) {
		currarg = car(argl);
		if(currarg && (currarg->type == symbol) && (!strcmp(currarg->symbol, "error"))) {
			printf("Trying to apply an error -- APPLY\n");
			return make_symbol("error");
		}
		argl = cdr(argl);
	}

	if(is_primitive_procedure(proc))
		return apply_primitive_procedure(proc, args);
	if(is_compound_procedure(proc)) {		
		out = eval_sequence(
			get_procedure_body(proc),
			extend_environment(get_procedure_parameters(proc),
				args,
				get_procedure_environment(proc)));		
		return out;
	}
	printf("Unknown procedure type -- APPLY\n");
	return make_symbol("error");
}

data_t *eval(const data_t *exp, data_t *env) {
	if(eval_plz_die) {
		eval_plz_die = 0;
		ExitThread(0);
	}

	if(is_self_evaluating(exp))
		return (data_t*)exp;
	if(is_variable(exp))
		return lookup_variable_value(exp, env);
	if(is_quoted_expression(exp))
		return get_text_of_quotation(exp);
	if(is_assignment(exp))
		return eval_assignment(exp, env);
	if(is_definition(exp))
		return eval_definition(exp, env);
	if(is_if(exp))
		return eval_if(exp, env);
	if(is_lambda(exp))
		return make_procedure(get_lambda_parameters(exp), get_lambda_body(exp), env);
	if(is_begin(exp))
		return eval_sequence(get_begin_actions(exp), env);
	if(is_cond(exp))
		return eval(cond_to_if(exp), env);
	if(is_letrec(exp))
		return eval(letrec_to_let(exp), env);
	if(is_let_star(exp))
		return eval(let_star_to_nested_lets(exp), env);
	if(is_let(exp))
		return eval(let_to_combination(exp), env);
	if(is_application(exp))		
		return apply(
			eval(get_operator(exp), env),
			get_list_of_values(get_operands(exp), env));
	
	printf("Unknown expression type -- EVAL '");
	return make_symbol("error");
}

int run_exp(const char *exp) {
	int error = 0;
	data_t *exp_list = read_exp(exp, NULL, &error);

	if(error)
		return error;

	eval(exp_list, the_global_env);

	return 0;
}
