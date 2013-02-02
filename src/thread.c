#include <Windows.h>
#include <WinBase.h>
#include <time.h>
#include <stdio.h>

#include "defs.h"
#include "eval.h"
#include "mem.h"

typedef struct {
	int running;
	data_t *exp, *env, *result;
} threadparam_t;

int thread_timeout = 2;

static DWORD WINAPI thread(LPVOID in) {
	threadparam_t *param = (threadparam_t*)in;
	
	param->result = eval(param->exp, param->env);
	param->running = 0;
}

data_t *eval_thread(data_t *exp, data_t *env) {
	HANDLE thread_handle;
	threadparam_t info;
	time_t starttime = time(NULL);
	
	info.exp = exp;
	info.env = env;
	info.running = 1;
	thread_handle = CreateThread(NULL, 0, thread, &info, 0, NULL);

	while(info.running) {
		if(thread_timeout && (time(NULL) - starttime > thread_timeout)) {
			TerminateThread(thread_handle, 0); 
			printf("ERROR: eval() timed out. (%ds)\n", thread_timeout);
			info.result = NULL;
			info.running = 0;
			run_gc(GC_FORCE);
		}
		if(n_bytes_allocated + sizeof(data_t) >= mem_lim_hard) {
			TerminateThread(thread_handle, 0); 
			printf("ERROR: Hard memory limit reached (%d bytes). eval() aborted.\n", mem_lim_hard);
			info.result = NULL;
			info.running = 0;
			run_gc(GC_FORCE);
		}
	}

	return info.result;
}