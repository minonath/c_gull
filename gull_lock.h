#ifndef _GULL_LOCK
#define _GULL_LOCK 1

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

/* 这是一个依靠原子实现的自旋锁 */

typedef struct { /* 自旋锁由一个原子指针和一个计数器组成 */
    atomic_size_t _lock_atomic;
    size_t _lock_count; /* 计数操作是在上锁后执行的，无需额外保护 */
} lock;

/* 初始化锁，因为原子地址申请后，地址上的数据不一定是零，所以需要初始化一次 */
static inline void _lock_initial(lock * lock) {
    atomic_init(&lock->_lock_atomic, 0);
    lock->_lock_count = 0;
}

/* 上锁 */
static inline void _lock_start(lock * _lock, size_t _thread_id) {
    size_t _locked;
    _loop:
    _locked = atomic_load_explicit(
        &_lock->_lock_atomic, memory_order_acquire); /* 获取当前上锁的值 */
    /* 空线程或者相同线程可以得到锁，否则进入循环 */
    if (_locked != 0 && _locked != _thread_id) {
        sched_yield();
        goto _loop;
    }
    /* 如果 _lock->_lock_atomic == _locked 那么写入 _thread_id */
    if (atomic_compare_exchange_strong_explicit(
            &_lock->_lock_atomic, &_locked, _thread_id,
            memory_order_release, memory_order_acquire)) {
        _lock->_lock_count += 1;
    } else {
        goto _loop;
    }
}

/* 尝试上锁，失败立即返回 */
static inline size_t _lock_try(lock * _lock, size_t _thread_id) {
    size_t _locked;
    _loop:
    _locked = atomic_load_explicit(
        &_lock->_lock_atomic, memory_order_acquire); /* 获取当前上锁的值 */
    /* 空线程或者相同线程可以得到锁，否则记作失败 */
    if (_locked != 0 && _locked != _thread_id) {
        return 0;
    }
    /* 如果 _lock->_lock_atomic == _locked 那么写入 _thread_id */
    if (atomic_compare_exchange_strong_explicit(
            &_lock->_lock_atomic, &_locked, _thread_id,
            memory_order_release, memory_order_acquire)) {
        _lock->_lock_count += 1;
    } else {
        goto _loop;
    }
    return _thread_id; /* 成功后返回非零数值 */
}

/* 解锁，必须和上锁处于同一个线程下 */
static inline void _lock_end(lock * _lock) {
    _lock->_lock_count -= 1;
    if (_lock->_lock_count == 0) {
        atomic_store_explicit(&_lock->_lock_atomic, 0, memory_order_release);
    }
}

#endif
