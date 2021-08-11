#include "threadPool.h"

/**
 * Destroy the thread pool
 * @param threadPool pointer to thread pool
 * @param shouldWaitForTasks flag 
 */
void* runThread(void* tp) {
    ThreadPool* threadPool = (ThreadPool*)tp;
    Task* task;
    while (!(threadPool->shouldStop)) {
        //start critical section
        if (pthread_mutex_lock(&threadPool->mutex) != 0) {
            perror("Error in pthread_mutex_lock");
            tpDestroy(threadPool, 0);
        }   

        // if queue is empty and no need to stop wait for new task without busy wating
        if(osIsQueueEmpty(threadPool->tasksQueue) && !(threadPool->shouldStop)) {
            pthread_cond_wait(&(threadPool->cond), &(threadPool->mutex));
        }

        // if queue isn't empty and no need to stop pop taks from queue and execute
        if(!(osIsQueueEmpty(threadPool->tasksQueue)) && !(threadPool->shouldStop)) {
            task = (Task*) osDequeue(threadPool->tasksQueue);
            // end of critical section
            pthread_mutex_unlock(&threadPool->mutex);  
            if(task != NULL) {
                task->compFunc(task->params);
                free(task);
            }
        }
        else { // release lock
            pthread_mutex_unlock(&threadPool->mutex);  
        }
    } 
}

/**
 * Destroy the thread pool
 * @param threadPool pointer to thread pool
 * @param shouldWaitForTasks flag 
 */
void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks){
   
    threadPool->destroyd = 1; 
    if (shouldWaitForTasks == 0) {
        threadPool->shouldStop = 1;
    }
    else {
        while (!osIsQueueEmpty(threadPool->tasksQueue)) {} // waits until all tasks finished 
        threadPool->shouldStop = 1;
    }
    // wake up all thread
    pthread_cond_broadcast(&(threadPool->cond));
    // join to each thread
    int i;
    for (i = 0; i < threadPool->threadsCounter; i++) {
        if (pthread_join(threadPool->threads[i], NULL) != 0) {
        perror("pthread_join operation failed ");
        exit(-1);
        }
    }

    // free the tasks remain in queue
	 if (shouldWaitForTasks == 0) {
         while(!(osIsQueueEmpty(threadPool->tasksQueue))){
             Task* t = (Task*) osDequeue(threadPool->tasksQueue);
             free(t);
         }
     } 
    osDestroyQueue(threadPool->tasksQueue);
    if ((pthread_cond_destroy(&threadPool->cond) != 0) || (pthread_mutex_destroy(&threadPool->mutex) != 0)) {
        perror("Error in destroy cond or mutex");
        exit(-1);
    }
    free(threadPool->threads);
    free(threadPool);
}
 
/**
 * Insert task into the thread pool queue of tasks
 * @param threadPool pointer to thread pool
 * @param computeFunc pointer to function
 * @param param param to the function
 * @return 0 if succsed -1 if failed
 */
int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param){
    if(threadPool == NULL || computeFunc == NULL || threadPool->destroyd) {
        return -1;
    }
    Task* task = (Task*) malloc(sizeof(Task));
    if (task == NULL) {
        perror("Error in malloc");
    }
    task->compFunc = computeFunc;
    task->params = param;
    // adding to task to queue in critical section
    if (pthread_mutex_lock(&(threadPool->mutex)) != 0) {
        tpDestroy(threadPool, 0);
        perror("Error in pthread_mutex_lock");
        exit(-1);
    }
    
    osEnqueue(threadPool->tasksQueue, task);
   // printf("inset task\n");
    // end of critical section
    if (pthread_mutex_unlock(&(threadPool->mutex)) != 0) {
        tpDestroy(threadPool, 0);
        perror("Error in pthread_mutex_unlock");
        exit(-1);
    }
    // signal to threads that new task added
    if (pthread_cond_signal(&threadPool->cond) != 0) {
        tpDestroy(threadPool, 0);
        perror("Error in pthread_cond_signal");
        exit(-1);
    }
    return 0;
}

/**
 * Create and initializes new thread pool 
 * @param numOfThreads number of thread in the pool
 * @return pointer to the thread pool
 */
ThreadPool* tpCreate(int numOfThreads){
    if (numOfThreads <= 0) {
        perror("illegal num of threads");
    }
    ThreadPool* threadPool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        perror("Error in malloc");
    }
    threadPool->threads = (pthread_t *) malloc(sizeof(pthread_t) * numOfThreads);
    if (threadPool->threads == NULL) {
        perror("Error in malloc");
        tpDestroy(threadPool,0);
    }
    if ((pthread_mutex_init(&threadPool->mutex, NULL) != 0) || (pthread_cond_init(&(threadPool->cond), NULL) != 0)) {
        perror("Error in init");
        tpDestroy(threadPool,0);
    }

	threadPool->tasksQueue = osCreateQueue();
    threadPool->threadsCounter = numOfThreads;
    threadPool->shouldStop = 0;
    threadPool->destroyd = 0;
    int i;
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(threadPool->threads[i]), NULL, runThread, (void*) threadPool) != 0) {
            perror("Error in pthread_create");
            tpDestroy(threadPool,0);
        }
    }
    return threadPool;
}

