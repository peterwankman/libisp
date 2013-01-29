libisp -- Lisp evaluator based on SICP
(C) 2013 Martin Wolters

1. HOW TO USE
-------------

1.1. General
------------

libisp.h is the main header file for the whole library.

1.1. Primitive Procedures
-------------------------

You can add your own procedures as primitives with the supplied function
add_prim_proc(), by providing a name under which the procedure will be known to
lisp, and a pointer to the procedure itself:

	void add_prim_proc(char *name, prim_proc proc);

Where prim_proc is typedef'd to be

	typedef struct data_t* (*prim_proc)(const struct data_t*);

data_t is a pointer to the following struct:

	typedef struct data_t {
		dtype_t type;
		union {
			int integer;
			double decimal;
			char *string;
			char *symbol;
			prim_proc proc;
			struct cons_t *pair;
		} val;
	} data_t;

and dtype_t and cons_t are 

	typedef enum dtype_t {
		integer, decimal, string, symbol, pair, prim_procedure
	} dtype_t;
	
	typedef struct cons_t {
		struct data_t *l, *r;
	} cons_t;
	
When your primitive procedure is called, it receives a Lisp data structure in
the first parameter. First check the type and then use data_t->val.<type> as
you need.

1.2. SETUP
----------

After you have added all primitive procedures you will need (or none), you need
to initialize the global environment. To do it, just call setup_environment()
	
The interpreter can now be used.

1.3. EVALUATING AN EXPRESSION
-----------------------------

Before evaluating an expression, you must first read it into a Lisp data struc-
ture. lisp_read() will do that for you.

	data_t *lisp_read(const char *exp, size_t *readto, int *error);
	
exp will be the expression to evaluate, *readto is a pointer to the last byte
read by the reader. You can use it to get to the next expression in the next
step of the REPL. *error will be 1 if the reader encountered a syntax error.

The returned data structure can now be used to evaluate the expression.

	data_t *eval(const data_t *exp, data_t *env);
	
Use the_global_environment for the last parameter, unless you know what you do.
eval() returns a Lisp data structure, which can be printed to the screen with

	void lisp_print_data(const data_t *d);
	
or be manipulated however you like.

You can also evaluate an expression in the global environment an discard the
result. This is useful for defining variables and non-primitive procedures,
that will be used by your program. The usage of the function should be trivial.

	void run(const char *exp);

1.4. MEMORY MANAGEMENT
----------------------

You need to call the garbage collector manually. Just use

	void run_gc(void);
	
You can also free data structures manually, using the functions

	void free_data(data_t *in);
	void free_data_rec(data_t *in);

The former will free just the data structure supplied, while the latter will
free a list structure recursively. After you are done with using libisp, clean
up everything with

	void cleanup_lisp(void);
	
This will run the garbage collector, free the global environment and then free
the list of primitive procedures. After that, everything allocated by libisp
is free()d.

There is a function which shows allocated instances of data_t that have not yet
been freed.

	void showmemstats(FILE *fp);
	
The argument allows for output to stderr or a log file.