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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

data_t *lisp_read(const char *exp, size_t *readto, int *error);

static size_t skip_whitespace(const char *exp) {
	char *out = (char*)exp;

	while(out[0] && isspace(out[0]))
		out++;

	return out - exp;
}

static int is_string(const char *exp, size_t *len) {
	char *end;

	if(exp[0] == '\"') {
		end = (char*)strchr(exp + 1, '\"');
		if(end) {
			*len = end - exp + 1;
			return 1;
		}
	}
	return 0;
}

static int is_symbol(const char *exp, size_t *len) {
	char *allowed = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!$%&*+-./:<=>?@^_~'#";
	
	*len = 0;
	while(exp[*len] && !isspace(exp[*len]) && exp[*len] != ')') {
		if(!strchr(allowed, exp[*len]))			
			return 0;		
		(*len)++;
	}
	return 1;
}

static int is_quotation(const char *exp) {
	if(exp[0] == '\'')
		return 1;
	return 0;
}

static int is_decimal(const char *exp, size_t *len, double *out) {
	size_t pointfound = 0;
	char *end;

	*len = 0;
	if(exp[0] == '-')
		(*len)++;

	while(exp[*len] && !isspace(exp[*len]) && exp[*len] != ')') {
		if(exp[*len] == '.')
			pointfound = 1;
		else if((exp[*len] < '0') && (exp[*len] > '9') && exp[*len]) {			
			return 0;
		}
		(*len)++;
	}

	if(pointfound) {		
		end = (char*)exp + *len;
		*out = strtod(exp, NULL);
		return 1;
	}
	return 0;
}

static int is_integer(const char *exp, size_t *len, int *out) {
	int fact = 1, numbers = 0;

	*len = 0;
	if(exp[0] == '-') {
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

	if(exp[0] != '(')
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

static data_t *read_subexp(const char *exp, size_t already_quoted, size_t *readto, int *error) {
	char *buf, *newexp;
	int integer;
	double decimal;
	size_t skip, newread, exppos;
	data_t *newdata, *out = NULL;
	
	skip = skip_whitespace(exp);
	exp += skip;
	
	*error = 1;
	*readto = 0;

	if(is_quotation(exp) && !already_quoted) {
		out = cons(lisp_make_symbol("quote"), cons(read_subexp(exp + 1, 1, &newread, error), NULL));
		*readto += newread + 1;
	} else if(is_decimal(exp, readto, &decimal)) {
		out = lisp_make_decimal(decimal);
	} else if(is_integer(exp, readto, &integer)) {		
		out = lisp_make_int(integer);
	} else if(is_string(exp, readto)) {
		if((buf = (char*)malloc(*readto - 1)) == NULL)
			return NULL;
		
		strncpy(buf, exp + 1, *readto - 2);
		buf[*readto - 2] = '\0';
		out = lisp_make_string(buf);		
		free(buf);		
	} else if(is_symbol(exp, readto)) {		
		if((buf = (char*)malloc(*readto + 1)) == NULL)
			return NULL;
		strncpy(buf, exp, *readto);
		buf[*readto] = '\0';
		out = lisp_make_symbol(buf);
		free(buf);
	} else if(is_combination(exp, readto)) {
		if(!already_quoted && is_empty_combination(exp)) {
			out = NULL;
		} else {
			if((newexp = (char*)malloc(*readto)) == NULL)
				return NULL;
			buf = newexp;

			strncpy(newexp, exp + 1, *readto - 2);
			newexp[*readto - 2] = '\0';

			out = NULL;
			do {
				get_last_subexp(newexp, &exppos);
				newdata = read_subexp(newexp + exppos, already_quoted, &newread, error);
				out = cons(newdata, out);
				newexp[exppos] = '\0';			
			} while(newexp[0]);

			free(buf);
		}
	}

	*readto += skip;
	
	if(!(*readto))
		return NULL;

	*error = 0;
	return out;	
}

data_t *lisp_read(const char *exp, size_t *readto, int *error) {
	size_t l = strlen(exp);

	*readto = 0;
	if(!l)		
		return NULL;
	
	return read_subexp(exp, 0, readto, error);
}
