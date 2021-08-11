#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct thread_pool
{
 OSQueue *tasksQueue;
 pthread_t* threads;
 pthread_mutex_t mutex;
 pthread_cond_t cond;
 int threadsCounter;
 int shouldStop;
 int destroyd;
}ThreadPool;

typedef struct {
	void (*compFunc)(void *);
	void *params;
} Task;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
