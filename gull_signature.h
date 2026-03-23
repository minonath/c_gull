#ifndef _GULL_SIGNATURE_H
#define _GULL_SIGNATURE_H 1

#include <stddef.h>
#include <time.h>

typedef struct {
    size_t _signature_second;
    size_t _signature_nano; /* 0-999999999 */
    size_t _signature_count;
} signature;

static inline signature * _singature_initial(signature * _signature) {
    _signature->_signature_count = 0;
    struct timespec _time = {0};
    clock_gettime(CLOCK_REALTIME, &_time);
    _signature->_signature_second = _time.tv_sec;
    _signature->_signature_nano = _time.tv_nsec;
    return _signature;
}

/* 纳秒的上限是 999999999 也就是小于 2 的 29 次方 */
#define _SIGNATURE_NANO_LIMIT (1 << 30)

static signature * _singature_generate(signature * _signature) {
    if (_signature->_signature_count == -1) {
        _signature->_signature_count = 0; // 此时，需要重新记录时间
        struct timespec _time = {0};
        clock_gettime(CLOCK_REALTIME, &_time);
        if (_signature->_signature_second == _time.tv_sec &&
                (_signature->_signature_nano &
                    (_SIGNATURE_NANO_LIMIT - 1)) == _time.tv_nsec) {
            /* 当前时间没有变，可以利用 _signature_nano 空闲区域  */
            size_t _nano_increase =
                _signature->_signature_nano + _SIGNATURE_NANO_LIMIT;
            if (_nano_increase > _signature->_signature_nano) {
                _signature->_signature_nano = _nano_increase;
            } else { /* 纳秒越界，暂停一纳秒再来 */
                struct timespec _request = {0, 1}, _remain = {0};
                nanosleep(&_request, &_remain);
                _singature_generate(_signature);
            }
        } else {
            _signature->_signature_second = _time.tv_sec;
            _signature->_signature_nano = _time.tv_nsec;
        }
    } else {
        _signature->_signature_count += 1;
    }
    return _signature;
}

#define _SIGNATURE_SCAN (sizeof(size_t) * 8 / 6)

static char _signature_base64[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ-0123456789_abcdefghijklmnopqrstuvwxyz";

#define _SIGNATURE_PASTE                                                     \
    for (int _m = 0; _m < 5; _m ++) {                                        \
        size_t _n = _i >> 6;                                                 \
        _out[_k++] = _signature_base64[_i - (_n << 6)];                      \
        _i = _n;                                                             \
    }

/* 需要三个指针全部加起来才是 6 的倍数 */
static void _signature_string(signature * _signature, char * _out) {
    unsigned * _temp = (unsigned *) _signature;
    size_t _i, _j, _k = 0;

    _i = _temp[0]; _SIGNATURE_PASTE _j = _i << 4;
    _i = _temp[1]; _SIGNATURE_PASTE _j |= _i << 2;
    _i = _temp[2]; _SIGNATURE_PASTE _j |= _i;

    #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
        _out[_k++] = _signature_base64[_j];

        _i = _temp[3]; _SIGNATURE_PASTE _j = _i << 4;
        _i = _temp[4]; _SIGNATURE_PASTE _j |= _i << 2;
        _i = _temp[5]; _SIGNATURE_PASTE _j |= _i;
    #endif

    _out[_k] = _signature_base64[_j];
}

#undef _SIGNATURE_PASTE

/*
一个 signature 结构，占用三个指针，也就是 3*32 / 3*64 bits
每 6 个 bits 可以转换为一个 base64 字符，所以转换后的文本长度如下
*/
#define _SIGNATURE_SIZE (sizeof(size_t) * 8 * 3 / 6)

#endif
