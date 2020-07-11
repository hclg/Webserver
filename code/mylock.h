#ifndef __MYLOCK_H
#define __MYLOCK_H
#include <iostream>
#include <list>
#include <cstdio>
#include <semaphore.h>
#include <exception>
#include <pthread.h>
#include "http_coon.h"

class sem {
private:
    sem_t m_sem;
public:
    sem();
    ~sem();
    bool post();
    bool wait();
};

sem::sem() {
    if (sem_init(&m_sem, 0, 0) != 0) {
        throw std::exception();
    }
}

sem::~sem() {
    sem_destroy(&m_sem);
}

bool sem::post() {
    return sem_post(&m_sem) == 0;
}

bool sem::wait() {
    return sem_wait(&m_sem) == 0;
}

class my_lock {
private:
    pthread_mutex_t m_mutex;
public:
    my_lock();
    ~my_lock();
    bool lock();
    bool unlock();
};

my_lock::my_lock() {
    if (pthread_mutex_init(&m_mutex, NULL) != 0) {
        throw std::exception();
    }
}

my_lock::~my_lock() {
    pthread_mutex_destroy(&m_mutex);
}

bool my_lock::lock() {
    return pthread_mutex_lock(&m_mutex) == 0;
}

bool my_lock::unlock() {
    return pthread_mutex_unlock(&m_mutex) == 0;
}



#endif 