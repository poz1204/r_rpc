#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include "rocket/net/io_thread.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"

namespace rocket {

IOThread::IOThread() {
    // 1.创建新线程 2.eventloop-进入新thread开始执行

    int rt = sem_init(&m_init_semaphore, 0, 0);
    assert(rt == 0);
    rt = sem_init(&m_start_semaphore, 0, 0);
    assert(rt == 0);

    // 可以在main函数中取出arg - to use sem_t
    pthread_create(&m_thread, NULL, &IOThread::Main, this);
    // 信号量 等到 Main函数执行完 <除了loop>
    rt = sem_wait(&m_init_semaphore);
    DEBUGLOG("io thread:[%d] create success", m_thread_id);
}

IOThread::~IOThread() {
    m_event_loop->stop();
    sem_destroy(&m_init_semaphore);
    sem_destroy(&m_start_semaphore);
    pthread_join(m_thread, NULL);

    if (m_event_loop) {
        delete m_event_loop;
        m_event_loop = NULL;
    }
}

void* IOThread::Main(void* arg) {
    IOThread* thread = static_cast<IOThread*> (arg);

    thread->m_event_loop = new EventLoop();
    thread->m_thread_id = getThreadId();

    // 唤醒等待的线程
    sem_post(&thread->m_init_semaphore);

    // 让IO 线程等待，直到我们主动的启动
    DEBUGLOG("IOThread %d created, wait start semaphore", thread->m_thread_id);
    sem_wait(&thread->m_start_semaphore);
    DEBUGLOG("IOThread %d start loop ", thread->m_thread_id);
    thread->m_event_loop->loop();

    DEBUGLOG("IOThread %d end loop ", thread->m_thread_id);

    return NULL;
}


void IOThread::start() {
    DEBUGLOG("Now invoke IOThread %d", m_thread_id);
    sem_post(&m_start_semaphore);
}


void IOThread::join() {
    pthread_join(m_thread, NULL);
}

}