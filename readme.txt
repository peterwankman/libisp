
 '-._                  ___.....___
     `.__           ,-'        ,-.`-,
         `''-------'          ( p )  `._
                               `-'      (        Have you conjured the spirits
                                         \             of your computer today?
                                .         \
                                 \\---..,--'
         .............._           --...--,
                        `-.._         _.-'
                             `'-----''                     Read SICP for help.


libisp -- Lisp evaluator based on SICP
(C) 2013-2017 Martin Wolters

1. HOW TO USE
-------------

1.1. GENERAL
------------

libisp.h is the main header file for the whole library.

1.2. INITIALIZING A CONTEXT
---------------------------

You need to create a context for each instance of the interpreter you are going
to use. The function creating a context is

	lisp_ctx_t *lisp_make_context(const size_t mem_lim_soft, 
		const size_t mem_lim_hard, const size_t mem_verbosity, 
		const size_t thread_timeout);
	
where mem_lim_soft is the amount of allocated memory within the context, that
will trigger the garbage collector. mem_lim_hard will cause the allocator to
refuse allocating new memory.

mem_verbosity can be one of LISP_GC_SILENT or LISP_GC_VERBOSE. The latter will
make the allocator and garbage collector notify you of what they're doing.

thread_timeout is the number of seconds after which the evaluator thread will
be terminated.

1.2. PRIMITIVE PROCEDURES
-------------------------

You can add your own procedures as primitives with the supplied function
lisp_add_prim_proc(), by providing a name under which the procedure will be
known to Lisp, and a pointer to the procedure itself:

	void lisp_add_prim_proc(char *name, lisp_prim_proc proc, 
		lisp_ctx_t *context);

Where lisp_prim_proc is typedef'd to be

	typedef struct lisp_data_t* (*lisp_prim_proc)(const struct lisp_data_t*, 
		lisp_ctx_t *context);

lisp_data_t is a pointer to the following struct:

	typedef struct lisp_data_t {
		lisp_type_t type;
		union {
			int integer;
			double decimal;
			char *string;
			char *symbol;
			char *error;
			lisp_prim_proc proc;
			struct cons_t *pair;
		};
	} lisp_data_t;

and lisp_type_t and lisp_cons_t are 

	typedef enum lisp_type_t {
		lisp_type_integer, 
		lisp_type_decimal, 
		lisp_type_string, 
		lisp_type_symbol, 
		lisp_type_pair, 
		lisp_type_prim,
		lisp_type_error
	} lisp_type_t;
	
	typedef struct lisp_cons_t {
		struct lisp_data_t *l, *r;
	} cons_t;
	
When your primitive procedure is called, it receives a Lisp data structure in
the first parameter. First check the type and then use lisp_data_t->[type] as
you need.

1.3. CONFIG VARIABLES
---------------------

You can use variables to interface the interpreter with your program. Register
a variable as a config var with

	void lisp_add_cvar(const char *name, const size_t *valptr, 
		const int access, lisp_ctx_t *context);
		
where name is the identifier for the variable within Lisp, valptr a pointer to
the variable in C and access one of  LISP_CVAR_RO and LISP_CVAR_RW.

Config variables can be accessed from Lisp using

	(get-cvar <identifier>)
	(set-cvar! <identifier> <value>)

The following config variables are provided with every new context:

	mem_allocated		(LISP_CVAR_RO)
	mem_lim_hard		(LISP_CVAR_RO)
	mem_lim_soft		(LISP_CVAR_RO)
	mem_list_entries	(LISP_CVAR_RO)
	mem_verbosity		(LISP_CVAR_RW)
	thread_timeout		(LISP_CVAR_RW)
	
1.5. INITIALIZING THE ENVIRONMENT
---------------------------------

After adding the needed primitive procedures and config variables, finalize the
initialization of your context with

	lisp_setup_env(lisp_ctx_t *context);
	
This will register all primitive procedures and config variables with Lisp and
define some useful compound procedures. Finally the garbage collector will be
run and you can begin using your Lisp context.

1.5. EVALUATING AN EXPRESSION
-----------------------------

Before evaluating an expression, you must first read it into a Lisp data struc-
ture. lisp_read() will do that for you.

	lisp_data_t *lisp_read(const char *exp, size_t *readto, 
		int *error, lisp_ctx_t *context);
	
exp will be the expression to evaluate, *readto is a pointer to the last byte
read by the reader. You can use it to get to the next expression in the next
step of the REPL. *error will be 1 if the reader encountered a syntax error.

The returned data structure can now be used to evaluate the expression.

	lisp_data_t *lisp_eval(const lisp_data_t *exp, lisp_ctx_t *context);
	
lisp_eval() returns a Lisp data structure, which can be printed to the screen
(like any other Lisp structure, say from lisp_read()) with

	void lisp_print(const lisp_data_t *d, lisp_ctx_t *context);
	
or be manipulated however you like.

You can also evaluate an expression in the current context and discard the
result. This is useful for defining variables and non-primitive procedures,
that will be used by your program. The usage of the function should be trivial.

	void lisp_run(const char *exp, lisp_ctx_t *context);

1.6. MEMORY MANAGEMENT
----------------------

The memory management uses two variables to determine its behaviour:

	size_t mem_lim_soft
	size_t mem_lim_hard

After mem_lim_hard is reached, the allocator will refuse to allocate any more
memory and return an error. The soft limit tells the garbage collector, when to
actually reclaim memory.

You need to call the garbage collector manually. Just use

	size_t lisp_gc(int force, lisp_ctx_t *context);

The parameter can be LISP_GC_FORCE, which will always reclaim any unreachable
memory, or LISP_GC_LOWMEM, which will only reclaim memory, when more than
mem_lim_soft is in use. It will return the number of bytes reclaimed (if any).

You can also free data structures manually, using the functions

	void lisp_free_data(lisp_data_t *in, lisp_ctx_t *context);
	void lisp_free_data_rec(lisp_data_t *in, lisp_ctx_t *context);

The former will free just the data structure supplied, while the latter will
free a list structure recursively. After you are done with using your context,
destroy it with

	void lisp_destroy_context(lisp_ctx_t *context);
	
This will run the garbage collector, free the global environment and then free
the list of primitive procedures. After that, everything allocated by libisp
in that context is free()d.

There is a function which shows allocated instances of data_t that have not yet
been freed.

	void lisp_gc_stats(FILE *fp, lisp_ctx_t *context);
	
The argument allows for output to stderr or a log file.
