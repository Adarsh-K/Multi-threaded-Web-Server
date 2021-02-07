#include <bits/stdc++.h>
#include <unistd.h>
#include "threadPool.h"
using namespace std;

typedef struct work{
	void (*routine) (void*);   // Address of function
	void *arg;                 // Argument of function
	struct work* next;		   // Pointer to structure of work
}workUtil;

typedef struct thread_pooled {
	int num_threads;			// No. of threads
	int qsize;					// Size of queue
	pthread_t *threads;			// Pointer to threads
	workUtil* qHead;			// Head pointer of queue
	workUtil* qTail;			// Tail pointer of queue
	pthread_cond_t q_NonEmpty;	// Condition vairiables for empty and non-empty for queue
	pthread_cond_t q_Empty;
	pthread_mutex_t qlock;		// Lock on the queue 
	int shutdown;
	int dont_accept;
} threadpoolUtil;

/* This is the work function of the thread */
void* do_work(void *arg) {
	threadpoolUtil *pool = (threadpoolUtil *) arg;
	workUtil* wor;	
	while(1) {	
		pthread_mutex_lock(&pool->qlock);  // Acquire the lock.
		while( pool->qsize == 0) {		   // If the size is 0 then wait.  
			if(pool->shutdown) {
				pthread_mutex_unlock(&(pool->qlock));
				pthread_exit(NULL);
			}
			// Wait until the condition says its not empty and give up the lock. 
			pthread_cond_wait(&(pool->q_NonEmpty),&(pool->qlock));
			// If destroy function is called
			if(pool->shutdown) {
				pthread_mutex_unlock(&(pool->qlock));
				pthread_exit(NULL);
			}
		}
		wor = pool->qHead;	// pool->qHead is set in dispatch function
		pool->qsize--;
		if(pool->qsize == 0) {
			pool->qHead = NULL;
			pool->qTail = NULL;
		}
		else {
			pool->qHead = wor->next;
		}
		if(pool->qsize == 0 && !pool->shutdown) {
			// Signal that q its empty.
			pthread_cond_signal(&(pool->q_Empty));
		}
		pthread_mutex_unlock(&(pool->qlock));
		(wor->routine)(wor->arg);   // Function call
		free(wor);					// Free the work storage.  	
	}
}

//***************************************** Create_threadpool  ***************************************************** 
threadpool create_threadpool(int num_threads_in_pool){
  	if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAX_THREADS))
    	return NULL;
  	threadpoolUtil *pool = (threadpoolUtil *) malloc(sizeof(threadpoolUtil));
	if (pool == NULL) {
	    fprintf(stderr, "Out of Memory !!!\n");
	    return NULL;
	  }
  	pool->threads = (pthread_t*) malloc (sizeof(pthread_t) * num_threads_in_pool);

  	if(pool->threads ==NULL) {
    	fprintf(stderr, "Out of Memory !!!\n");
    	return NULL;	
  	}

	// Initialization of all struct variables
	pool->num_threads = num_threads_in_pool; 
	pool->qsize = 0;
	pool->qHead = NULL;
	pool->qTail = NULL;
	pool->shutdown = 0;
	pool->dont_accept = 0;

	// Initialize mutex and condition variables. 
	pthread_mutex_init(&pool->qlock,NULL);
	pthread_cond_init(&pool->q_Empty,NULL);
	pthread_cond_init(&pool->q_NonEmpty,NULL);

	// Make threads
	for (int i = 0;i < num_threads_in_pool;i++) {
	  if(pthread_create(&(pool->threads[i]),NULL,do_work,pool)) {
	    	fprintf(stderr, "Thread initiation error!\n");	
		    return NULL;	
	  	}
	}
	return (threadpool)pool;
}


//********************************************** DISPATCH   ************************************************************
void dispatch(threadpool from_me,dispatch_fn dispatch_to_here,void* arg){
	threadpoolUtil *pool=(threadpoolUtil*)from_me;
	// Make a work queue element.  
	workUtil* wor = (workUtil*) malloc(sizeof(workUtil));
	if(wor == NULL) {
		fprintf(stderr, "Out of memory creating a work struct!\n");
		return;	
	}
	wor->routine = dispatch_to_here;
	wor->arg = arg;
	wor->next = NULL;
	pthread_mutex_lock(&(pool->qlock));
	// It will helpful when the destroy function is already called and Just incase someone is trying to queue more function
	if(pool->dont_accept) { 
		free(wor); // Work structs.  
		return;
	}
	if(pool->qsize == 0) {
		pool->qHead = wor; 
		pool->qTail = wor;
		pthread_cond_signal(&(pool->q_NonEmpty)); 
	}
	else {
		pool->qTail->next = wor;	// Add to end;
		pool->qTail = wor;
	}
	pool->qsize++;
	pthread_mutex_unlock(&(pool->qlock));  // Unlock the queue.

}

//*************************************************  DESTROY ****************************************************
void destroy_threadpool(threadpool destroyme){
	threadpoolUtil *pool = (threadpoolUtil *) destroyme;
	pthread_mutex_lock(&(pool->qlock));
	pool->dont_accept = 1;
	while(pool->qsize != 0) {
		pthread_cond_wait(&(pool->q_Empty),&(pool->qlock));  // Wait until the q is empty.
	}
	pool->shutdown = 1;  // Allow shutdown
	pthread_cond_broadcast(&(pool->q_NonEmpty));  
	pthread_mutex_unlock(&(pool->qlock));
	for(int i=0;i < pool->num_threads;i++) {
		pthread_join(pool->threads[i],NULL);
	}
	pthread_mutex_destroy(&(pool->qlock));
	pthread_cond_destroy(&(pool->q_Empty));
	pthread_cond_destroy(&(pool->q_NonEmpty));
	free(pool->threads);
	return;
}