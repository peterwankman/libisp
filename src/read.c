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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "libisp/data.h"

lisp_data_t *lisp_read(const char *exp, size_t *readto, int *error, lisp_ctx_t *context);

static size_t skip_whitespace(const char *exp) {
	const char *out = exp;

	while(*out && isspace(*out))
		out++;

	return out - exp;
}

static int is_string(const char *exp, size_t *len) {
	const char *end;

	if(*exp == '\"') {
		end = strchr(exp + 1, '\"');
		if(end) {
			*len = end - exp + 1;
			return 1;
		}
	}
	return 0;
}

static int is_symbol(const char *exp, size_t *len) {
	const char *allowed = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!$%&*+-./:<=>?@^_~'#";
	
	*len = 0;
	while(exp[*len] && !isspace(exp[*len]) && exp[*len] != ')') {
		if(!strchr(allowed, exp[*len]))			
			return 0;		
		(*len)++;
	}
	return 1;
}

static int is_quotation(const char *exp) {
	if(*exp == '\'')
		return 1;
	return 0;
}

static int is_decimal(const char *exp, size_t *len, double *out) {
	size_t pointfound = 0;

	*len = 0;
	if(exp[0] == '-')
		(*len)++;

	while(exp[*len] && !isspace(exp[*len]) && exp[*len] != ')') {
		if(exp[*len] == '.')
			pointfound = 1;
		else if(((exp[*len] < '0') || (exp[*len] > '9')) && exp[*len]) {
			return 0;
		}
		(*len)++;
	}

	if(pointfound) {		
		*out = strtod(exp, NULL);
		return 1;
	}
	return 0;
}

static int is_integer(const char *exp, size_t *len, int *out) {
	int fact = 1, numbers = 0;

	*len = 0;
	if(*exp == '-') {
		fact = -1;
		(*len)++;
	}

	*out = 0;

	while(exp[*len] && !isspace(exp[*len]) && exp[*len] != ')') {
		if(exp[*len] >= '0' && exp[*len] <= '9') {
			numbers++;
			*out *= 10;
			*out += exp[*len] - '0';
		} else
			return 0;
		(*len)++;
	}

	*out *= fact;

	if(numbers)
		return 1;
	return 0;
}

static int is_combination(const char *exp, size_t *len) {
	size_t parens = 1;

	if(*exp != '(')
		return 0;
		
	*len = 1;
	while(exp[*len]) {
		if(exp[*len] == '(')
			parens++;
		else if(exp[*len] == ')')
			parens--;
		(*len)++;
		if(parens == 0)
			return 1;
	}

	return 0;
}

void get_last_subexp(const char *exp, size_t *pos) {
	size_t paren = 0;

	if(strlen(exp) == 0) {
		*pos = 0;
		return;
	}

	for(*pos = strlen(exp) - 1; *pos > 0; (*pos)--)
		if(!isspace(exp[*pos]))
			break;

	for(; *pos > 0; (*pos)--) {
		if(exp[*pos] == ')')
			paren++;
		else if(exp[*pos] == '(')
			paren--;

		if(!paren) {
			if(isspace(exp[*pos]))
				break;
		}
	}

	*pos += skip_whitespace(exp + *pos);	
}

static int is_empty_combination(const char *exp) {
	size_t pos = skip_whitespace(exp);

	if(exp[pos] != '(')
		return 0;
	
	pos += skip_whitespace(exp + pos + 1) + 1;
	if(exp[pos] != ')')
		return 0;
	
	return 1;
}

static lisp_data_t *read_subexp(const char *exp, size_t already_quoted, size_t *readto, int *error, lisp_ctx_t *context) {
	char *buf, *newexp;
	int integer;
	double decimal;
	size_t skip, newread, exppos;
	lisp_data_t *newdata, *out = NULL;
	
	skip = skip_whitespace(exp);
	exp += skip;
	
	*readto = 0;

	if(is_quotation(exp) && !already_quoted) {
		out = lisp_cons(lisp_make_symbol("quote", context), lisp_cons(read_subexp(exp + 1, 1, &newread, error, context), NULL));
		*readto += newread + 1;
	} else if(is_decimal(exp, readto, &decimal)) {
		out = lisp_make_decimal(decimal, context);
	} else if(is_integer(exp, readto, &integer)) {		
		out = lisp_make_int(integer, context);
	} else if(is_string(exp, readto)) {
		if((buf = malloc(*readto - 1)) == NULL)
			return NULL;
		
		strncpy(buf, exp + 1, *readto - 2);
		buf[*readto - 2] = '\0';
		out = lisp_make_string(buf, context);		
		free(buf);		
	} else if(is_symbol(exp, readto)) {		
		if((buf = malloc(*readto + 1)) == NULL)
			return NULL;
		strncpy(buf, exp, *readto);
		buf[*readto] = '\0';
		out = lisp_make_symbol(buf, context);
		free(buf);
	} else if(is_combination(exp, readto)) {
		if(is_empty_combination(exp)) {			
			out = NULL;
		} else {
			if((newexp = malloc(*readto)) == NULL)
				return NULL;
			buf = newexp;

			strncpy(newexp, exp + 1, *readto - 2);
			newexp[*readto - 2] = '\0';
			newexp += skip_whitespace(newexp);

			out = NULL;
			do {
				get_last_subexp(newexp, &exppos);
				newdata = read_subexp(newexp + exppos, already_quoted, &newread, error, context);
				out = lisp_cons(newdata, out);
				newexp[exppos] = '\0';			
			} while(newexp[0]);

			free(buf);
		}
	} else {
		*error = 1;
		return NULL;
	}

	*readto += skip;
	
	if(!(*readto))
		return NULL;

	return out;	
}

lisp_data_t *lisp_read(const char *exp, size_t *readto, int *error, lisp_ctx_t *context) {
	size_t l = strlen(exp), int_readto;
	lisp_data_t *out;
	*error = 0;

	int_readto = 0;
	if(!l)		
		return NULL;
	
	out = read_subexp(exp, 0, &int_readto, error, context);

	if(readto)
		*readto = int_readto;

	return out;
}
