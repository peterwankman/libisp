#ifdef _WIN32
#include <Windows.h>
#include <WinBase.h>
#else
#include <pthread.h>
#define HANDLE pthread_t
#endif

#include <time.h>
#include <stdio.h>

#include "defs.h"
#include "eval.h"
#include "mem.h"

typedef struct {
	data_t *exp, *env, *result;
} threadparam_t;

int thread_running = 0;
int thread_timeout = 2;

#ifdef _WIN32
static DWORD WINAPI thread(LPVOID in) {
#else
static void *thread(void *in) {
#endif
	threadparam_t *param = (threadparam_t*)in;

	param->result = eval(param->exp, param->env);
	thread_running = 0;
#ifndef _WIN32
	pthread_exit(NULL);
#endif
	return 0;
}

void kill_thread(threadparam_t *info, const char *msg, HANDLE thread_handle) {
#ifdef _WIN32
	TerminateThread(thread_handle, 0);
#else
	pthread_cancel(thread_handle);
#endif
	fprintf(stderr, msg);
	info->result = NULL;
	thread_running = 0;
}

static HANDLE spawn_thread(threadparam_t *param) {
#ifdef _WIN32
	return CreateThread(NULL, 0, thread, param, 0, NULL);
#else
	pthread_t thread_handle;

	if(pthread_create(&thread_handle, NULL, thread, param)) {
		fprintf(stderr, "ERROR: Could not spawn eval() thread.\n");
		return 0;
	}
	pthread_detach(thread_handle);

	return thread_handle;
#endif
}

data_t *eval_thread(const data_t *exp, data_t *env) {
	HANDLE thread_handle;
	threadparam_t info;
	time_t starttime = time(NULL);

	info.exp = (data_t*)exp;
	info.env = env;
	thread_running = 1;

	thread_handle = spawn_thread(&info);

	while(thread_running) {
		if(thread_timeout && (time(NULL) - starttime > thread_timeout))
			kill_thread(&info, "ERROR: eval() timed out.\n", thread_handle);
		if(n_bytes_allocated + sizeof(data_t) >= mem_lim_hard) {
			kill_thread(&info, "ERROR: Hard memory limit reached.\n", 
				thread_handle);
			run_gc(GC_FORCE);
		}
	}

	return info.result;
}
