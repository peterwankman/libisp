#ifdef _WIN32
#include <Windows.h>
#include <WinBase.h>
#else
#include <pthread.h>
#endif

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

#ifdef _WIN32
static DWORD WINAPI thread(LPVOID in) {
#else
void *thread(void *in) {
#endif
	threadparam_t *param = (threadparam_t*)in;
	
	param->result = eval(param->exp, param->env);
	param->running = 0;

#ifdef _WIN32
	return NULL;
#else
	pthread_exit(NULL);
#endif
}

data_t *eval_thread(data_t *exp, data_t *env) {
#ifdef _WIN32
	HANDLE thread_handle;
#else
	pthread_t thread_handle;
#endif
	threadparam_t info;
	time_t starttime = time(NULL);
	
	info.exp = exp;
	info.env = env;
	info.running = 1;

#ifdef _WIN32
	thread_handle = CreateThread(NULL, 0, thread, &info, 0, NULL);
#else
	if(pthread_create(&thread_handle, NULL, thread, &info)) {
		printf("ERROR: Could not spawn eval() thread.\n");
		return NULL;
	}
#endif

	while(info.running) {
		if(thread_timeout && (time(NULL) - starttime > thread_timeout)) {
#ifdef _WIN32
			TerminateThread(thread_handle, 0); 
#else
			pthread_detach(thread_handle);
			pthread_cancel(thread_handle);
#endif
			printf("ERROR: eval() timed out. (%ds)\n", thread_timeout);
			info.result = NULL;
			info.running = 0;
		}
		if(n_bytes_allocated + sizeof(data_t) >= mem_lim_hard) {
#ifdef _WIN32
			TerminateThread(thread_handle, 0); 
#else
			pthread_detach(thread_handle);
			pthread_cancel(thread_handle);
#endif
			printf("ERROR: Hard memory limit reached (%d bytes). eval() aborted.\n", mem_lim_hard);
			info.result = NULL;
			info.running = 0;
		}
	}

#ifndef _WIN32
	pthread_detach(thread_handle);
#endif

	return info.result;
}
