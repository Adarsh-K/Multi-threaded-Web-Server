#include <bits/stdc++.h>

#ifndef INC_THREADPOOL_H
#define INC_THREADPOOL_H
#define MAX_THREADS 100

typedef void* threadpool;

typedef void (* dispatch_fn)(void*);

// Creates a given no of threads return a threadpool, else return NULL
threadpool create_threadpool(int num_threads_in_pool);

// Takes a threadpool as well as function pointer which to call and a (void*)arg to pass it to the function
void dispatch(threadpool from_me,dispatch_fn dispatch_to_here,void* arg);

// This will destroy all threads and memory will be cleaned up after that.
void destroy_threadpool(threadpool destroyme);

#endif