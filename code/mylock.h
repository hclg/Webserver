#ifndef __MYLOCK_H
#define __MYLOCK_H
#include <iostream>
#include <list>
#include <cstdio>
#include <semaphore.h>
#include <exception>
#include <pthread.h>
#include "myhttp_coon.h"

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

bool my_loc::unlock() {
    return pthread_mutex_unlock(&m_mutex) == 0;
}


/*封装条件变量*/
class mycond{
private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
public:
    mycond();
    ~mycond();
    bool wait();
    bool signal();
};
 
mycond::mycond()
{
    if(pthread_mutex_init(&m_mutex,NULL)!=0)
    {
        throw std::exception();
    }
    if(pthread_cond_init(&m_cond, NULL)!=0)
    {
        throw std::exception();
    }
}
 
mycond::~mycond()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}
 
/*等待条件变量*/
bool mycond::wait()
{
    int ret;
    pthread_mutex_lock(&m_mutex);
    ret = pthread_cond_wait(&m_cond,&m_mutex);
    pthread_mutex_unlock(&m_mutex);
    return ret == 0;
}
 
/*唤醒等待条件变量的线程*/
bool mycond::signal()
{
    return pthread_cond_signal(&m_cond) == 0;
}

#endif 