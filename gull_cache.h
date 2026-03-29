#ifndef _GULL_CACHE_H
#define _GULL_CACHE_H 1

#include <stdarg.h>
#include "gull_page.h"

/* cache 是一个 filo 的内存区域 */
static inline atom * _cache_allocate(page * _thread, size_t _size) {
    atom * _cache = _page_allocate_bind(_thread, _TYPE_CACHE, _size);
    _atom_set_extra_used(_cache, 0);
    return _cache;
}

static inline atom * _cache_with(
        page * _thread, size_t _reserve, size_t _size, char * _initial) {
    atom * _cache = _page_allocate_bind(_thread, _TYPE_CACHE, _size);
    _atom_set_extra_used(_cache, _size);
    char * _address = _atom_get_extra_address(_cache);
    if (_reserve) {
        for (size_t _m = 0, _n = _size - 1; _n < _size;) {
            _address[_m ++] = _initial[_n --];
        }
    } else {
        for (size_t _m = 0, _n = 0; _n < _size;) {
            _address[_m ++] = _initial[_n ++];
        }
    }
    return _cache;
}

static inline void _cache_clear(atom * _cache) {
    _atom_set_extra_used(_cache, 0);
}

/* 缓存扩容 */
static void _cache_expand(page * _thread, atom * _cache, size_t _size) {

    size_t _old_len = _atom_get_extra_size(_cache);
    if (_size > _old_len) {
        void * _old_ptr = _atom_get_extra_address(_cache);
        size_t * _old = _old_ptr - sizeof(size_t);

        size_t * _new = _page_allocate_extra(_thread, _size);
        void * _new_ptr = _new + 1;
        size_t _new_len = *_new - sizeof(size_t);

        memcpy(_new_ptr, _old_ptr, _old_len);

        *_old = _old_len + sizeof(size_t); /* clean */

        page * _page = _page_head(_thread, _old);
        _page->_page_used -= *_old;

        _atom_set_extra_address(_cache, _new_ptr);
        _atom_set_extra_size(_cache, _new_len);

        *_new = ((size_t) _cache) | 1;
        _page_extra_test(_thread, _page);
    }
}

#define _cache_push(thread, cache, type, one) {                              \
    size_t _cache_push_old = _atom_get_extra_used(cache);                    \
    size_t _cache_push_new = _cache_push_old + sizeof(type);                 \
    _cache_expand(thread, cache, _cache_push_new);                           \
    *(type *)(_atom_get_extra_address(cache) + _cache_push_old) = one;       \
    _atom_set_extra_used(cache, _cache_push_new);                            \
}

#define _cache_pop(cache, type, result) {                                    \
    size_t _cache_push_off = _atom_get_extra_used(cache) - sizeof(type);     \
    _atom_set_extra_used(cache, _cache_push_off);                            \
    result = *(type *)(_atom_get_extra_address(cache) + _cache_push_off);    \
}

static inline unsigned char _cache_push_uint8(
        page * _thread, atom * _cache, unsigned char _value) {
    _cache_push(_thread, _cache, unsigned char, _value); return _value;
}

static inline unsigned _cache_push_uint32(
        page * _thread, atom * _cache, unsigned _value) {
    _cache_push(_thread, _cache, unsigned, _value); return _value;
}

static inline unsigned char _cache_pop_uint8(atom * _cache) {
    unsigned char _r; _cache_pop(_cache, unsigned char, _r); return _r;
}

static inline unsigned _cache_pop_uint32(atom * _cache) {
    unsigned _r; _cache_pop(_cache, unsigned, _r); return _r;
}

static inline void _cache_push_multi(page * _thread,
        atom * _cache, size_t _reserve, size_t _size, char * _buffer) {
    size_t _old = _atom_get_extra_used(_cache);
    size_t _new = _old + _size;
    _cache_expand(_thread, _cache, _new);
    _atom_set_extra_used(_cache, _new);
    char * _address = _atom_get_extra_address(_cache);

    if (_reserve) {
        for (size_t _i = _size - 1; _i < _size;) {
            _address[_old ++] = _buffer[_i --];
        }
    } else {
        for (size_t _i = 0; _i < _size;) {
            _address[_old ++] = _buffer[_i ++];
        }
    }
}

static inline void _cache_push_multi2(
        page * _thread, atom * _cache, size_t _reserve, size_t _size, ...) {
    size_t _old = _atom_get_extra_used(_cache);
    size_t _new = _old + _size;
    _cache_expand(_thread, _cache, _new);
    _atom_set_extra_used(_cache, _new);
    char * _address = _atom_get_extra_address(_cache);

    va_list _char;
    va_start(_char, _size);
    if (_reserve) {
        for (size_t _i = _size - 1; _i < _size;) {
            _address[_i --] = va_arg(_char, unsigned int);
        }
    } else {
        for (size_t _i = 0; _i < _size;) {
            _address[_i ++] = va_arg(_char, unsigned int);
        }
    }
    va_end(_char);
}

#define _CACHE_ROLLBACK(_j) {                                                \
    if (_i >= _size) {                                                       \
        _cache_push_multi2(_thread, _cache, 2, _first, _second);             \
        _i -= _j; break;                                                     \
    } else {                                                                 \
        unsigned _next = _buffer[_i];                                        \
        if (_next >= '0' && _next <= '9') {                                  \
            _result = (_result << 4) | (_next - '0'); _i += 1;               \
        } else if (_next >= 'a' && _next <= 'f') {                           \
            _result = (_result << 4) | (_next - 'a' + 10); _i += 1;          \
        } else if (_next >= 'A' && _next <= 'F') {                           \
            _result = (_result << 4) | (_next - 'A' + 10); _i += 1;          \
        } else {                                                             \
            _cache_push_multi2(_thread, _cache, 2, _first, _second);         \
            _i -= _j; break;                                                 \
        }                                                                    \
    }                                                                        \
}

#define _CACHE_PUSH_UTF_8(_j) {                                              \
    if (_result < 0x7f) {                                                    \
        _cache_push_uint8(_thread, _cache, _result);                         \
    } else if (_result < 0x7ff) {                                            \
        _cache_push_multi2(_thread, _cache, 2,                               \
            0b11000000 | (_result >> 6),                                     \
            0b10000000 | (_result & 0b111111));                              \
    } else if (_result < 0xffff) {                                           \
        _cache_push_multi2(_thread, _cache, 3,                               \
            0b11100000 | (_result >> 12),                                    \
            0b10000000 | ((_result >> 6) & 0b111111),                        \
            0b10000000 | (_result & 0b111111));                              \
    } else if (_result < 0x10ffff) {                                         \
        _cache_push_multi2(_thread, _cache, 4,                               \
            0b11110000 | (_result >> 18),                                    \
            0b10000000 | ((_result >> 12) & 0b111111),                       \
            0b10000000 | ((_result >> 6) & 0b111111),                        \
            0b10000000 | (_result & 0b111111));                              \
    } else {                                                                 \
        _cache_push_multi2(_thread, _cache, 2, _first, _second);             \
        _i -= _j; break;                                                     \
    }                                                                        \
}

/* 输入的 buffer 内容是转译后的，加入缓存变成原始 unicode 内容 */
static inline void _cache_push_escaped_2_unicode(
        page * _thread, atom * _cache,/* size_t _reserve,*/ char * _buffer) {
    size_t _size = strlen(_buffer);
    // if (_reserve) {
    //     for (size_t _i = 0; _i < _size; _i ++) {
    //         size_t _j = _size - _i - 1;
    //         if (_i != _j) {
    //             char _swap = _buffer[_i];
    //             _buffer[_i] = _buffer[_j];
    //             _buffer[_j] = _swap;
    //         }
    //     }
    // }
    for (size_t _i = 0; _i < _size;) {
        unsigned _first = _buffer[_i++];
        if (_first == '\\' && _i < _size) { /* 反向转译 */
            unsigned _second = _buffer[_i++];
            unsigned _result;
            switch (_second) {
                case '\\': _cache_push_uint8(_thread, _cache, '\\'); break;
                case 'a': _cache_push_uint8(_thread, _cache, '\a'); break;
                case 'b': _cache_push_uint8(_thread, _cache, '\b'); break;
                case 'f': _cache_push_uint8(_thread, _cache, '\f'); break;
                case 'n': _cache_push_uint8(_thread, _cache, '\n'); break;
                case 'r': _cache_push_uint8(_thread, _cache, '\r'); break;
                case 't': _cache_push_uint8(_thread, _cache, '\t'); break;
                case 'v': _cache_push_uint8(_thread, _cache, '\v'); break;

                case '0': case '1': case '2': case '3':
                    _result = _second - '0';
                    if (_i >= _size) {
                        _cache_push_uint8(_thread, _cache, _result);
                        break;
                    } else {
                        unsigned _next = _buffer[_i];
                        if (_next >= '0' && _next <= '7') {
                            _result = (_result << 3) | (_next - '0');
                            _i += 1;
                        } else {
                            _cache_push_uint8(_thread, _cache, _result);
                            break;
                        }
                    }
                    if (_i >= _size) {
                        _cache_push_uint8(_thread, _cache, _result);
                        break;
                    } else {
                        unsigned _next = _buffer[_i];
                        if (_next >= '0' && _next <= '7') {
                            _result = (_result << 3) | (_next - '0');
                            _i += 1;
                        } else {
                            _cache_push_uint8(_thread, _cache, _result);
                            break;
                        }
                    }
                    _cache_push_uint8(_thread, _cache, _result);
                    break;
                case '4': case '5': case '6': case '7':
                    _result = _second - '0';
                    if (_i >= _size) {
                        _cache_push_uint8(_thread, _cache, _result);
                        break;
                    } else {
                        unsigned _next = _buffer[_i];
                        if (_next >= '0' && _next <= '7') {
                            _result = (_result << 3) | (_next - '0');
                            _i += 1;
                        } else {
                            _cache_push_uint8(_thread, _cache, _result);
                            break;
                        }
                    }
                    _cache_push_uint8(_thread, _cache, _result);
                    break;

                case 'x':
                    if (_i >= _size) {
                        _cache_push_multi2(
                            _thread, _cache, 2, _first, _second);
                        break;
                    } else {
                        unsigned _next = _buffer[_i];
                        if (_next >= '0' && _next <= '9') {
                            _result = _next - '0';
                            _i += 1;
                        } else if (_next >= 'a' && _next <= 'f') {
                            _result = _next - 'a' + 10;
                            _i += 1;
                        } else if (_next >= 'A' && _next <= 'F') {
                            _result = _next - 'A' + 10;
                            _i += 1;
                        } else {
                            _cache_push_multi2(
                                _thread, _cache, 2, _first, _second);
                            break;
                        }
                    }
                    _CACHE_ROLLBACK(1);
                    _cache_push_uint8(_thread, _cache, _result);
                    break;

                case 'X':
                    if (_i >= _size) {
                        _cache_push_multi2(
                            _thread, _cache, 2, _first, _second);
                        break;
                    } else {
                        unsigned _next = _buffer[_i];
                        if (_next >= '0' && _next <= '9') {
                            _result = _next - '0';
                            _i += 1;
                        } else if (_next >= 'a' && _next <= 'f') {
                            _result = _next - 'a' + 10;
                            _i += 1;
                        } else if (_next >= 'A' && _next <= 'F') {
                            _result = _next - 'A' + 10;
                            _i += 1;
                        } else {
                            _cache_push_multi2(
                                _thread, _cache, 2, _first, _second);
                            break;
                        }
                    }
                    _CACHE_ROLLBACK(1);
                    _CACHE_ROLLBACK(2);
                    _CACHE_ROLLBACK(3);
                    _CACHE_ROLLBACK(4);
                    _CACHE_ROLLBACK(5);
                    _CACHE_ROLLBACK(6);
                    _CACHE_ROLLBACK(7);
                    _CACHE_PUSH_UTF_8(8);
                    break;

                case 'u':
                    if (_i >= _size) {
                        _cache_push_multi2(
                            _thread, _cache, 2, _first, _second);
                        break;
                    } else {
                        unsigned _next = _buffer[_i];
                        if (_next >= '0' && _next <= '9') {
                            _result = _next - '0';
                            _i += 1;
                        } else if (_next >= 'a' && _next <= 'f') {
                            _result = _next - 'a' + 10;
                            _i += 1;
                        } else if (_next >= 'A' && _next <= 'F') {
                            _result = _next - 'A' + 10;
                            _i += 1;
                        } else {
                            _cache_push_multi2(
                                _thread, _cache, 2, _first, _second);
                            break;
                        }
                    }
                    _CACHE_ROLLBACK(1);
                    _CACHE_ROLLBACK(2);
                    _CACHE_ROLLBACK(3);
                    _CACHE_PUSH_UTF_8(4);
                    break;

                case 'U':
                    if (_i >= _size) {
                        _cache_push_multi2(
                            _thread, _cache, 2, _first, _second);
                        break;
                    } else {
                        unsigned _next = _buffer[_i];
                        if (_next >= '0' && _next <= '9') {
                            _result = _next - '0';
                            _i += 1;
                        } else if (_next >= 'a' && _next <= 'f') {
                            _result = _next - 'a' + 10;
                            _i += 1;
                        } else if (_next >= 'A' && _next <= 'F') {
                            _result = _next - 'A' + 10;
                            _i += 1;
                        } else {
                            _cache_push_multi2(
                                _thread, _cache, 2, _first, _second);
                            break;
                        }
                    }
                    _CACHE_ROLLBACK(1);
                    _CACHE_ROLLBACK(2);
                    _CACHE_ROLLBACK(3);
                    _CACHE_ROLLBACK(4);
                    _CACHE_ROLLBACK(5);
                    _CACHE_PUSH_UTF_8(6);
                    break;

                default:
                    _cache_push_multi2(
                        _thread, _cache, 2, _first, _second);
                    break;
            }
        } else {
            _cache_push_uint8(_thread, _cache, _first);
        }
    }
}

static inline void _cache_release_direct(page * _thread, atom * _cache) {
    _page_release_extra(_thread, _cache);
    _page_release_double(_thread, _cache);
}

#endif
