#ifndef _THREADPOOL_H
#define _THREADPOOL_H
#include <iostream>
#include <list>
#include <cstdio>
#include <semaphore.h>
#include <exception>
#include <pthread.h>
#include "myhttp_coon.h"
#include "mylock.h"

template<typename T>

class threadpool {
private:
    int max_thread;
    int max_job;
    pthread_t *pthread_poll;
    std::list<T*> job_queue;
    my_lock queue_lock;
    sem job_sem;
    bool th_stop;
    static void *worker(void *arg);
    void run();
public:
    threadpool();
    ~threadpool();
    bool addjob(T *request);
}

template <typename T>
threadpool<T>::threadpool() {
    max_thread = 8;
    max_job = 1000;
    th_stop = false;
    pthread_poll = new pthread_t[max_thread];
    if (!pthread_poll) {
        throw std::exception();
    }
    for (int i = 0; i < max_thread; ++i) {
        cout << "Create the pthread: " << i << endl;
        if (pthread_create(pthread_poll+i, NULL, worker, this) != 0) {
            delete [] pthread_poll;
            throw std::exception();
        }
        if (pthread_detach(pthread_poll[i]) != 0) {
            delete [] pthread_poll;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool() {
    delete [] pthread_poll;
    th_stop = true;
}

template <typename T>
bool threadpool<T>::addjob(T *arg) {
    if (job_queue.size() > max_job) return false;
    queue_lock.lock();
    if (job_queue.size() > max_job) {
        queue_lock.unlock();
        return false;
    }
    job_queue.push_back(arg);
    queue_lock.unlock();
    job_sem.post();
    return true;
}

template <typename T>
void *threadpool<T>worker(void *arg) {
    threadpool *thraed = (threadpool *) arg;
    thread->run();
    return thread;
}

template <typename T>
void threadpool<T>::run() {
    while(!th_stop) {
        job_sem.wait();
        if (job_queue.empty()) continue;
        queue_lock.lock();
        if (job_queue.empty()) {
            queue_lock.unlock();
            continue;
        }
        T *request = job_queue.front();
        job_queue.pop_front();
        queue_lock.unlock();
        if (!request) continue;
        request->doit();
    }
}

#endif