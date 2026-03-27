#ifndef _GULL_TEXT_H
#define _GULL_TEXT_H 1

#include "gull_page.h"
#include "gull_cache.h"
#include "gull_text_printable.h"

static atom * _atom_text_build(
        page * _thread, size_t _size, size_t _protect, char * _string) {
    atom * _text;
    if (_size <= 15) {
        #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
            _text = _page_allocate_single(_thread);
            _atom_set_atom_info(_text, _TYPE_INFO_HALF);
            _atom_set_atom_reference(_text, 0);
            _atom_set_atom_type(_text, _TYPE_BRIEF);
        #else
            _text = _page_allocate_single(_thread);
            _atom_set_atom_info(_text, _TYPE_INFO_BRIEF_32);
            _atom_set_atom_reference(_text, 0);
        #endif
        _atom_set_brief_protect(_text, _protect);
        void * _address = _atom_get_brief_address(_text);
        memcpy(_address, _string, _size);
        memset(_address + _size, 0, 15 - _size);
    } else {
        _text = _page_allocate_bind(_thread, _TYPE_TEXT, _size);
        _atom_set_text_protect(_text, _protect);
        _atom_set_extra_used(_text, _size);
        void * _address = _atom_get_extra_address(_text);
        memcpy(_address, _string, _size);
        memset(_address + _size, 0, _atom_get_extra_size(_text) - _size);
    }
    return _text;
}

static inline size_t _text_brief_count(char * _address) {
    for (size_t _c = 0; _c < 15; _c ++) {
        if (_address[_c] == 0) {
            return _c;
        }
    }
    return 15;
}

static atom * _atom_2_text(page * _thread, atom * _atom) {
    return 0;
}

static atom * _text_clone(page * _thread, atom * _text) {
    size_t _info = _atom_get_atom_info(_text);
    char * _address;
    size_t _size, _protect;

    #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
        if (_info == _TYPE_INFO_HALF) {
            if (_atom_get_atom_type(_text) == _TYPE_BRIEF) {
                _protect = _atom_get_brief_protect(_text);
                _address = _atom_get_brief_address(_text);
                _size = _text_brief_count(_address);
                return _atom_text_build(_thread, _size, _protect, _address);
            }
        } else
    #else
        if (_info == _TYPE_INFO_BRIEF_32) {
            _protect = _atom_get_brief_protect(_text);
            _address = _atom_get_brief_address(_text);
            _size = _text_brief_count(_address);
            return _atom_text_build(_thread, _size, _protect, _address);
        } else
    #endif
    if (_info == _TYPE_INFO_FULL) {
        if (_atom_get_atom_type(_text) == _TYPE_TEXT) {
            _protect = _atom_get_text_protect(_text);
            _address = _atom_get_extra_address(_text);
            _size = _atom_get_extra_used(_text);
            return _atom_text_build(_thread, _size, _protect, _address);
        }
    }
    return _text_clone(_thread, _atom_2_text(_thread, _text));
}

static size_t _text_printable_contain(
        const unsigned * _charset, unsigned _size, unsigned _one) {
    for (int _start = 0, _end = _size - 1; ; ) {
        if (_start == _end) {
            return _one == _charset[_start];
        } else if (_start < _end) {
            unsigned _cursor = (_start + _end) >> 1;
            unsigned _target = _charset[_cursor];
            if (_one == _target) {
                return 1;
            } else if (_one < _target) {
                _end = _cursor - 1;
            } else {
                _start = _cursor + 1;
            }
        } else {
            return 0;
        }
    }
}

static inline unsigned char _text_read_hex(unsigned _one) {
    if (_one < 10) {
        return (unsigned char) (_one + '0');
    } else {
        return (unsigned char) (_one - 10 + 'a');
    }
}

static inline unsigned char _text_read_HEX(unsigned _one) {
    if (_one < 10) {
        return (unsigned char) (_one + '0');
    } else {
        return (unsigned char) (_one - 10 + 'A');
    }
}

/* 结果范围 -1 ~ 255 */
static inline int _text_read_byte(atom * _bytes, FILE * _file) {
    /* 结果范围 -1 ~ 255 */
    if (_file) return fgetc(_file);
    /* 结果范围 0 ~ 255 */
    if (_atom_get_extra_used(_bytes)) return _cache_pop_uint8(_bytes);
    /* 结果范围 -1 */
    return -1; // EOF
}

/* 结果范围 -1 ~ 0x10ffff */
static unsigned _text_read_unicode(
        page * _thread, atom * _unicodes, atom * _bytes, FILE * _file) {
    if (_atom_get_extra_used(_unicodes)) {
        return _cache_pop_uint32(_unicodes);
    }
    /* 读取第 1 个数据 */
    int _first = _text_read_byte(_bytes, _file);
    if ((_first & 0b11100000) == 0b11000000) { /* UTF8-2 */
        /* 读取第 2 个数据 */
        int _second = _text_read_byte(_bytes, _file);
        if ((_second & 0b11000000) != 0b10000000) { /* 符合要求，转换数据 */
            if (_second >= 0) _cache_push_uint8(_thread, _bytes, _second);
            return _first;
        }
        return ((_first & 0b11111) << 6) | (_second & 0b111111);

    } else if ((_first & 0b11110000) == 0b11100000) { /* UTF8-3 */
        /* 读取第 2 个数据 */
        int _second = _text_read_byte(_bytes, _file);
        if ((_second & 0b11000000) != 0b10000000) {
            if (_second >= 0) _cache_push_uint8(_thread, _bytes, _second);
            return _first;
        }
        /* 读取第 3 个数据 */
        int _third = _text_read_byte(_bytes, _file);
        if ((_third & 0b11000000) != 0b10000000) {
            if (_third >= 0) _cache_push_uint8(_thread, _bytes, _third);
            _cache_push_uint8(_thread, _bytes, _second);
            return _first;
        }
        return ((_first & 0b1111) << 12) |
            ((_second & 0b111111) << 6) | (_third & 0b111111);

    } else if ((_first & 0b11111000) == 0b11110000) { /* UTF8-4 */
        /* 读取第 2 个数据 */
        int _second = _text_read_byte(_bytes, _file);
        if ((_second & 0b11000000) != 0b10000000) {
            if (_second >= 0) _cache_push_uint8(_thread, _bytes, _second);
            return _first;
        }
        /* 读取第 3 个数据 */
        int _third = _text_read_byte(_bytes, _file);
        if ((_third & 0b11000000) != 0b10000000) {
            if (_third >= 0) _cache_push_uint8(_thread, _bytes, _third);
            _cache_push_uint8(_thread, _bytes, _second);
            return _first;
        }
        /* 读取第 4 个数据 */
        int _fourth = _text_read_byte(_bytes, _file);
        if ((_fourth & 0b11000000) != 0b10000000) {
            if (_fourth >= 0) _cache_push_uint8(_thread, _bytes, _fourth);
            _cache_push_uint8(_thread, _bytes, _third);
            _cache_push_uint8(_thread, _bytes, _second);
            return _first;
        }
        return ((_first & 0b111) << 18) | ((_second & 0b111111) << 12) |
            ((_third & 0b111111) << 6) | (_fourth & 0b111111);
        // if (_result <= 0x10ffff) { /* 但是 Unicode 有上限 */
        // return _result;  /* UTF8-4 实际的范围是 0 ~ 0x1fffff */
        // }
        /* 超限当作普通字符返回 */
        // _cache_push_uint8(_thread, _bytes, _fourth);
        // _cache_push_uint8(_thread, _bytes, _third);
        // _cache_push_uint8(_thread, _bytes, _second);
        // return _first;
    } else {
        return _first;
    }
}


/* 读取转译字符，结果范围为 -1 ~ 0x10ffff */
static unsigned _text_read_escpae(page * _thread,
        atom * _unicodes, atom * _bytes, FILE * _file) {
    unsigned _a1 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
    if (_a1 == '\\') {
        unsigned _a2 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
        unsigned _a3, _a4, _a5, _a6, _a7, _a8, _a9, _ax, _ar;
        switch (_a2) {
            case '\\': return '\\';
            case 'a': return '\a';
            case 'b': return '\b';
            case 'f': return '\f';
            case 'n': return '\n';
            case 'r': return '\r';
            case 't': return '\t';
            case 'v': return '\v';

            /* 最大不超过 \377 也就是 255 */
            case '0': case '1': case '2': case '3':
                _ar = _a2 - '0';
                _a3 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a3 >= '0' && _a3 <= '7') {
                    _ar = (_ar << 3) | (_a3 - '0');
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    return _ar;
                }
                _a4 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a4 >= '0' && _a4 <= '7') {
                    _ar = (_ar << 3) | (_a4 - '0');
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    return _ar;
                }
                return _ar;

            /* 高位只有两位，不超过 \77 也就是 63 */
            case '4': case '5': case '6': case '7':
                _ar = _a2 - '0';
                _a3 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a3 >= '0' && _a3 <= '7') {
                    _ar = (_ar << 3) | (_a3 - '0');
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    return _ar;
                }
                return _ar;

            case 'x': /* 两位，结果不超过 \xff 也就是 255 */
                _a3 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a3 >= '0' && _a3 <= '9') {
                    _ar = _a3 - '0';
                } else if (_a3 >= 'a' && _a3 <= 'f') {
                    _ar = _a3 - 'a' + 10;
                } else if (_a3 >= 'A' && _a3 <= 'F') {
                    _ar = _a3 - 'A' + 10;
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a4 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a4 >= '0' && _a4 <= '9') {
                    _ar = (_ar << 4) | (_a4 - '0');
                } else if (_a4 >= 'a' && _a4 <= 'f') {
                    _ar = (_ar << 4) | (_a4 - 'a' + 10);
                } else if (_a4 >= 'A' && _a4 <= 'F') {
                    _ar = (_ar << 4) | (_a4 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                return _ar;

            case 'X': /* 八位，结果比较大需要转为 utf8 返回 */
                _a3 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a3 >= '0' && _a3 <= '9') {
                    _ar = _a3 - '0';
                } else if (_a3 >= 'a' && _a3 <= 'f') {
                    _ar = _a3 - 'a' + 10;
                } else if (_a3 >= 'A' && _a3 <= 'F') {
                    _ar = _a3 - 'A' + 10;
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a4 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a4 >= '0' && _a4 <= '9') {
                    _ar = (_ar << 4) | (_a4 - '0');
                } else if (_a4 >= 'a' && _a4 <= 'f') {
                    _ar = (_ar << 4) | (_a4 - 'a' + 10);
                } else if (_a4 >= 'A' && _a4 <= 'F') {
                    _ar = (_ar << 4) | (_a4 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a5 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a5 >= '0' && _a5 <= '9') {
                    _ar = (_ar << 4) | (_a5 - '0');
                } else if (_a5 >= 'a' && _a5 <= 'f') {
                    _ar = (_ar << 4) | (_a5 - 'a' + 10);
                } else if (_a5 >= 'A' && _a5 <= 'F') {
                    _ar = (_ar << 4) | (_a5 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a6 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a6 >= '0' && _a6 <= '9') {
                    _ar = (_ar << 4) | (_a6 - '0');
                } else if (_a6 >= 'a' && _a6 <= 'f') {
                    _ar = (_ar << 4) | (_a6 - 'a' + 10);
                } else if (_a6 >= 'A' && _a6 <= 'F') {
                    _ar = (_ar << 4) | (_a6 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a7 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a7 >= '0' && _a7 <= '9') {
                    _ar = (_ar << 4) | (_a7 - '0');
                } else if (_a7 >= 'a' && _a7 <= 'f') {
                    _ar = (_ar << 4) | (_a7 - 'a' + 10);
                } else if (_a7 >= 'A' && _a7 <= 'F') {
                    _ar = (_ar << 4) | (_a7 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a7);
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a8 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a8 >= '0' && _a8 <= '9') {
                    _ar = (_ar << 4) | (_a8 - '0');
                } else if (_a8 >= 'a' && _a8 <= 'f') {
                    _ar = (_ar << 4) | (_a8 - 'a' + 10);
                } else if (_a8 >= 'A' && _a8 <= 'F') {
                    _ar = (_ar << 4) | (_a8 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a8);
                    _cache_push_uint32(_thread, _unicodes, _a7);
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a9 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a9 >= '0' && _a9 <= '9') {
                    _ar = (_ar << 4) | (_a9 - '0');
                } else if (_a9 >= 'a' && _a9 <= 'f') {
                    _ar = (_ar << 4) | (_a9 - 'a' + 10);
                } else if (_a9 >= 'A' && _a9 <= 'F') {
                    _ar = (_ar << 4) | (_a9 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a9);
                    _cache_push_uint32(_thread, _unicodes, _a8);
                    _cache_push_uint32(_thread, _unicodes, _a7);
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _ax = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_ax >= '0' && _ax <= '9') {
                    _ar = (_ar << 4) | (_ax - '0');
                } else if (_ax >= 'a' && _ax <= 'f') {
                    _ar = (_ar << 4) | (_ax - 'a' + 10);
                } else if (_ax >= 'A' && _ax <= 'F') {
                    _ar = (_ar << 4) | (_ax - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _ax);
                    _cache_push_uint32(_thread, _unicodes, _a9);
                    _cache_push_uint32(_thread, _unicodes, _a8);
                    _cache_push_uint32(_thread, _unicodes, _a7);
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }

                return _ar;

            case 'u': /* 四位，结果需要切分 */
                _a3 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a3 >= '0' && _a3 <= '9') {
                    _ar = _a3 - '0';
                } else if (_a3 >= 'a' && _a3 <= 'f') {
                    _ar = _a3 - 'a' + 10;
                } else if (_a3 >= 'A' && _a3 <= 'F') {
                    _ar = _a3 - 'A' + 10;
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a4 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a4 >= '0' && _a4 <= '9') {
                    _ar = (_ar << 4) | (_a4 - '0');
                } else if (_a4 >= 'a' && _a4 <= 'f') {
                    _ar = (_ar << 4) | (_a4 - 'a' + 10);
                } else if (_a4 >= 'A' && _a4 <= 'F') {
                    _ar = (_ar << 4) | (_a4 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a5 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a5 >= '0' && _a5 <= '9') {
                    _ar = (_ar << 4) | (_a5 - '0');
                } else if (_a5 >= 'a' && _a5 <= 'f') {
                    _ar = (_ar << 4) | (_a5 - 'a' + 10);
                } else if (_a5 >= 'A' && _a5 <= 'F') {
                    _ar = (_ar << 4) | (_a5 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a6 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a6 >= '0' && _a6 <= '9') {
                    _ar = (_ar << 4) | (_a6 - '0');
                } else if (_a6 >= 'a' && _a6 <= 'f') {
                    _ar = (_ar << 4) | (_a6 - 'a' + 10);
                } else if (_a6 >= 'A' && _a6 <= 'F') {
                    _ar = (_ar << 4) | (_a6 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }

                return _ar;

            case 'U': /* 六位 */
                _a3 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a3 >= '0' && _a3 <= '9') {
                    _ar = _a3 - '0';
                } else if (_a3 >= 'a' && _a3 <= 'f') {
                    _ar = _a3 - 'a' + 10;
                } else if (_a3 >= 'A' && _a3 <= 'F') {
                    _ar = _a3 - 'A' + 10;
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a4 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a4 >= '0' && _a4 <= '9') {
                    _ar = (_ar << 4) | (_a4 - '0');
                } else if (_a4 >= 'a' && _a4 <= 'f') {
                    _ar = (_ar << 4) | (_a4 - 'a' + 10);
                } else if (_a4 >= 'A' && _a4 <= 'F') {
                    _ar = (_ar << 4) | (_a4 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a5 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a5 >= '0' && _a5 <= '9') {
                    _ar = (_ar << 4) | (_a5 - '0');
                } else if (_a5 >= 'a' && _a5 <= 'f') {
                    _ar = (_ar << 4) | (_a5 - 'a' + 10);
                } else if (_a5 >= 'A' && _a5 <= 'F') {
                    _ar = (_ar << 4) | (_a5 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a6 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a6 >= '0' && _a6 <= '9') {
                    _ar = (_ar << 4) | (_a6 - '0');
                } else if (_a6 >= 'a' && _a6 <= 'f') {
                    _ar = (_ar << 4) | (_a6 - 'a' + 10);
                } else if (_a6 >= 'A' && _a6 <= 'F') {
                    _ar = (_ar << 4) | (_a6 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a7 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a7 >= '0' && _a7 <= '9') {
                    _ar = (_ar << 4) | (_a7 - '0');
                } else if (_a7 >= 'a' && _a7 <= 'f') {
                    _ar = (_ar << 4) | (_a7 - 'a' + 10);
                } else if (_a7 >= 'A' && _a7 <= 'F') {
                    _ar = (_ar << 4) | (_a7 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a7);
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                _a8 = _text_read_unicode(_thread, _unicodes, _bytes, _file);
                if (_a8 >= '0' && _a8 <= '9') {
                    _ar = (_ar << 4) | (_a8 - '0');
                } else if (_a8 >= 'a' && _a8 <= 'f') {
                    _ar = (_ar << 4) | (_a8 - 'a' + 10);
                } else if (_a8 >= 'A' && _a8 <= 'F') {
                    _ar = (_ar << 4) | (_a8 - 'A' + 10);
                } else {
                    _cache_push_uint32(_thread, _unicodes, _a8);
                    _cache_push_uint32(_thread, _unicodes, _a7);
                    _cache_push_uint32(_thread, _unicodes, _a6);
                    _cache_push_uint32(_thread, _unicodes, _a5);
                    _cache_push_uint32(_thread, _unicodes, _a4);
                    _cache_push_uint32(_thread, _unicodes, _a3);
                    _cache_push_uint32(_thread, _unicodes, _a2);
                    return _a1;
                }
                return _ar;

            default:
                _cache_push_uint32(_thread, _unicodes, _a2);
                return _a1;
        }
    }
    return _a1;
}

static int _text_read_char(
        page * _thread, atom * _texts, atom * _unicodes,
        atom * _bytes, FILE * _file, const unsigned * _charset,
        unsigned _charset_size, unsigned * _range, size_t _range_size) {
    if (_atom_get_extra_used(_texts)) {
        return _cache_pop_uint8(_texts);
    }
    unsigned _one = _text_read_escpae(_thread, _unicodes, _bytes, _file);
    if (_one == 0xffffffff) return -1;
    if (_one == '\\') {
        _cache_push_uint8(_thread, _texts, '\\');
        return '\\';
    }
    if (_one >= 32 && _one <= 126) { /* ASCII 可显示字符 */
        return _one;
    }
    for (int _i = 0; _i < _range_size;) {
        unsigned _start = _range[_i++];
        unsigned _end = _range[_i++];
        if (_one >= _start && _one <= _end) {
            goto _match;
        }
    }
    /* 用户自定义可显示字符，转为 utf8 */
    if (_charset ? _text_printable_contain(
            _charset, _charset_size, _one) : _text_printable_contain(
                _text_printable, _TEXT_PRINTABLE, _one)) {
        goto _match;
    } else {
        goto _failure;
    }
    _match:
    if (_one < 0x7f) { // 0b1111111 (7)
        return _one;
    }
    if (_one < 0x7ff) { // 0b11111111111 (11)
        _cache_push_uint8(
            _thread, _texts, 0b10000000 | (_one & 0b111111));
        /* 返回前五位 */
        return 0b11000000 | (_one >> 6);
    }
    if (_one < 0xffff) { // 0b1111111111111111 (16)
        _cache_push_uint8(
            _thread, _texts, 0b10000000 | (_one & 0b111111));
        _cache_push_uint8(
            _thread, _texts, 0b10000000 | ((_one >> 6) & 0b111111));
        return 0b11100000 | (_one >> 12);
    }
    // 用 0x1fffff 也行吧，不过 unicode 最大是 0x10ffff
    if (_one < 0x10ffff) { // 0b100001111111111111111 (21)
        _cache_push_uint8(
            _thread, _texts, 0b10000000 | (_one & 0b111111));
        _cache_push_uint8(
            _thread, _texts, 0b10000000 | ((_one >> 6) & 0b111111));
        _cache_push_uint8(
            _thread, _texts, 0b10000000 | ((_one >> 12) & 0b111111));
        return 0b11110000 | (_one >> 18);
    }
    _failure:
    if (_one > 0xffffff) { /* 八位 */
        _cache_push_uint8(_thread, _texts, _text_read_HEX(_one & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 4) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 8) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 12) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 16) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 20) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 24) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 28) & 15));
        _cache_push_uint8(_thread, _texts, 'X');
        return '\\';
    }
    if (_one > 0xffff) {
        _cache_push_uint8(_thread, _texts, _text_read_HEX(_one & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 4) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 8) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 12) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 16) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_HEX((_one >> 20) & 15));
        _cache_push_uint8(_thread, _texts, 'U');
        return '\\';
    }
    if (_one > 0xff) {
        _cache_push_uint8(_thread, _texts, _text_read_hex(_one & 15));
        _cache_push_uint8(_thread, _texts, _text_read_hex((_one >> 4) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_hex((_one >> 8) & 15));
        _cache_push_uint8(_thread, _texts, _text_read_hex((_one >> 12) & 15));
        _cache_push_uint8(_thread, _texts, 'u');
        return '\\';
    }
    switch (_one) {
        case '\a':
            _cache_push_uint8(_thread, _texts, 'a'); // alarm
            return '\\';
        case '\b':
            _cache_push_uint8(_thread, _texts, 'b'); // backspace
            return '\\';
        case '\f':
            _cache_push_uint8(_thread, _texts, 'f'); // front page
            return '\\';
        case '\n':
            _cache_push_uint8(_thread, _texts, 'n'); // new line
            return '\\';
        case '\r':
            _cache_push_uint8(_thread, _texts, 'r'); // return first
            return '\\';
        case '\t':
            _cache_push_uint8(_thread, _texts, 't'); // table normal
            return '\\';
        case '\v':
            _cache_push_uint8(_thread, _texts, 'v'); // vertical table
            return '\\';
        default:
            _cache_push_uint8(
                _thread, _texts, _text_read_hex(_one & 15));
            _cache_push_uint8(
                _thread, _texts, _text_read_hex((_one >> 4) & 15));
            _cache_push_uint8(_thread, _texts, 'x');
            return '\\';
    }
}

#endif
