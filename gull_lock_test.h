#ifndef _GULL_LOCK_TEST_H
#define _GULL_LOCK_TEST_H 1

#include <stdio.h>
#include "gull_lock.h"

void * _lock_test_thread(void * _args) {
    size_t _self = (size_t) pthread_self();
    struct timespec _request_timespec = {0, 50000000},
        _remain_timespec = {0}, _record_timespec = {0};

    for (int _i = 0; _i < 7; _i ++) {
        /* 可以尝试注释 _lock_start _lock_end 这两行看看乱序效果 */
        _lock_start((lock *) _args, _self);

        clock_gettime(CLOCK_REALTIME, &_record_timespec);
        printf("start %zx  %zu %zu\n",
            _self, _record_timespec.tv_sec, _record_timespec.tv_nsec);

        nanosleep(&_request_timespec, &_remain_timespec);

        clock_gettime(CLOCK_REALTIME, &_record_timespec);
        printf("end   %zx  %zu %zu\n\n\n",
            _self, _record_timespec.tv_sec, _record_timespec.tv_nsec);
        _lock_end((lock *) _args);

        /* 如果不主动调度，线程调度速度肯定是循环更快，注释此行则循环优先 */
        sched_yield();
    }

    return 0;
}

void gull_lock_test() {
    int SIZE = 7;
    lock _test;
    _lock_initial(&_test);

    if (_lock_try(&_test, (size_t) pthread_self()) == 0) {
        printf("第一次 _lock_try 成功\n");
        _lock_end(&_test);
    } else {
        printf("第一次 _lock_try 失败\n");
    }

    pthread_t _threads[SIZE];
    for (int _i = 0; _i < SIZE; _i ++)
        pthread_create(_threads + _i, 0, _lock_test_thread, &_test);

    struct timespec _request_timespec = {0, 100000000};
    struct timespec _remain_timespec = {0};
    nanosleep(&_request_timespec, &_remain_timespec);

    if (_lock_try(&_test, (size_t) pthread_self()) == 0) {
        printf("第二次 _lock_try 成功\n");
        _lock_end(&_test);
    } else {
        printf("第二次 _lock_try 失败\n");
    }

    for (int _i = 0; _i < SIZE; _i ++)
        pthread_join(_threads[_i], 0);

    for (int _i = 0; _i < SIZE; _i ++)
        printf("%d %zx\n", _i, (size_t) _threads[_i]);

    printf("\n");
}

#endif
