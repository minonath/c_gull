#ifndef _GULL_KEYWORD_H
#define _GULL_KEYWORD_H 1

#include <stdarg.h>
#include "gull_atom.h"
#include "gull_page.h"
#include "gull_cache.h"
#include "gull_text.h"

static inline atom * _keyword_push(atom ** _root, atom * _one) {
    _atom_set_keyword_next(_one, *_root);
    return *_root = _one;
}

static atom * _keyword_remove(atom ** _root, atom * _one) {
    if (*_root == _one) {
        if ((*_root = _atom_get_keyword_next(_one))) {
            _atom_set_keyword_next(_one, 0);
        }
        return _one;
    } else {
        atom * _cursor = *_root;
        _loop:
        if (_atom_get_keyword_next(_cursor) == _one) {
            _atom_set_keyword_next(_cursor, _atom_get_keyword_next(_one));
            _atom_set_keyword_next(_one, 0);
            return _one;
        } else {
            _cursor = _atom_get_keyword_next(_cursor); /* 不可为零 */
            goto _loop;
        }
    }
}

static void _keyword_default(
        page * _thread, atom ** _default, atom ** _link,
        size_t _type, size_t _count, ...) {
    va_list _keywords;
    va_start(_keywords, _count);
    for (int _i = 0; _i < _count; _i ++) {
        char * _keyword = va_arg(_keywords, char *);

        if (_default[_type] == 0) {
            size_t _length = strlen(_keyword);
            atom * _a = _page_allocate_bind(_thread, _type, _length);
            _atom_set_extra_used(_a, _length);
            void * _address = _atom_get_extra_address(_a);
            memcpy(_address, _keyword, _length);
            memset(_address + _length, 0, _atom_get_extra_size(_a) - _length);
            _keyword_push(_link, _a);
            _default[_type] = _a;
        } else {
            atom * _loop = *_link;
            _inner:
            if (strcmp(_atom_get_extra_address(_loop), _keyword)) {
                if (_atom_get_keyword_next(_loop)) {
                    _loop = _atom_get_keyword_next(_loop);
                    goto _inner;
                }
            } else {
                continue;
            }
            size_t _length = strlen(_keyword);
            atom * _a = _page_allocate_bind(_thread, _type, _length);
            _atom_set_extra_used(_a, _length);
            void * _address = _atom_get_extra_address(_a);
            memcpy(_address, _keyword, _length);
            memset(_address + _length, 0, _atom_get_extra_size(_a) - _length);
            _keyword_push(_link, _a);
        }
    }
    va_end(_keywords);
}

static atom * _keyword_initial(page * _thread,
        const unsigned * _charset, unsigned _charset_size,
        unsigned * _range, size_t _range_size,
        char ** _keyword, size_t _keyword_size) {

    atom * _list[_KEYWORD_LIMIT] = {0};
    atom * _link = 0;
    size_t _type, _max = 2;
    atom * _records = _cache_allocate(_thread, sizeof(size_t));
    atom * _texts = _cache_allocate(_thread, sizeof(size_t));
    atom * _unicodes = _cache_allocate(_thread, sizeof(size_t));
    atom * _bytes = _cache_allocate(_thread, sizeof(size_t));

    for (int _i = 0; _i < _keyword_size; _i ++) {
        char * _one = _keyword[_i];
        if ((size_t) _one < _KEYWORD_LIMIT) {
            _type = (size_t) _one;
        } else {
            _cache_push_multi(
                _thread, _bytes, 1, strlen(_one), _one);
            for (; ; ) {
                int _c = _text_read_char(
                    _thread, _texts, _unicodes, _bytes, 0,
                    _charset, _charset_size, _range, _range_size);
                if (_c == -1) break;
                _cache_push_uint8(_thread, _records, _c);
            }
            size_t _length = _atom_get_extra_used(_records);
            if (_max < _length) _max = _length;
            atom * _a = _page_allocate_bind(_thread, _type, _length);
            _atom_set_extra_used(_a, _length);
            void * _address = _atom_get_extra_address(_a);
            memcpy(_address, _atom_get_extra_address(_records), _length);
            memset(_address + _length, 0, _atom_get_extra_size(_a) - _length);
            _cache_clear(_records);
            _keyword_push(&_link, _a);
            if (_list[_type] == 0) _list[_type] = _a;
        }
    }

    _cache_release_direct(_thread, _records);
    _cache_release_direct(_thread, _texts);
    _cache_release_direct(_thread, _unicodes);
    _cache_release_direct(_thread, _bytes);

    _keyword_default(_thread, _list, &_link,
        _KEYWORD_SEPARATOR, 5, " ", "\\t", "\\n", "\\r", "\\r\\n");
    _keyword_default(_thread, _list, &_link, _KEYWORD_WORD, 1, "`");
    _keyword_default(_thread, _list, &_link, _KEYWORD_BASE, 1, "@");
    _keyword_default(_thread, _list, &_link, _KEYWORD_ACCESS, 1, ".");
    _keyword_default(_thread, _list, &_link, _KEYWORD_EXECUTE_L, 1, "(");
    _keyword_default(_thread, _list, &_link, _KEYWORD_EXECUTE_R, 1, ")");
    _keyword_default(_thread, _list, &_link, _KEYWORD_SEQUENCE_L, 1, "[");
    _keyword_default(_thread, _list, &_link, _KEYWORD_SEQUENCE_R, 1, "]");
    _keyword_default(_thread, _list, &_link, _KEYWORD_SYNCHRONIC_L, 1, "{");
    _keyword_default(_thread, _list, &_link, _KEYWORD_SYNCHRONIC_R, 1, "}");
    _keyword_default(_thread, _list, &_link, _KEYWORD_STATIC_L, 1, "<");
    _keyword_default(_thread, _list, &_link, _KEYWORD_STATIC_R, 1, ">");
    _keyword_default(_thread, _list, &_link, _KEYWORD_COMMENT, 1, "#");
    _keyword_default(_thread, _list, &_link, _KEYWORD_ASSIST, 1, "/");
    _keyword_default(_thread, _list, &_link, _KEYWORD_EXPLAIN, 1, ":");
    _keyword_default(_thread, _list, &_link, _KEYWORD_INTERPRET, 1, "=");
    _keyword_default(_thread, _list, &_link, _KEYWORD_EXTEND, 1, "$");
    _keyword_default(_thread, _list, &_link, _KEYWORD_MARK, 1, "+");
    _keyword_default(_thread, _list, &_link, _KEYWORD_JUMP, 1, "*");
    _keyword_default(_thread, _list, &_link, _KEYWORD_RETURN, 2, "~", "～");
    _keyword_default(_thread, _list, &_link, _KEYWORD_CANCEL, 1, "//");

    if (_max < 4) _max = 4;

    atom * _sorted = 0, * _loop, * _next;

    for (int _i = 0; _i < _max;) {
        _i += 1;
        _loop = _link;
        _inner:
        _next = _atom_get_keyword_next(_loop);
        if (_atom_get_extra_used(_loop) == _i) {
            _keyword_push( /* 排序 */
                &_sorted, _keyword_remove(&_link, _loop));
        }
        if (_next) {
            _loop = _next;
            goto _inner;
        }
    }

    atom * _result = _page_allocate_bind(_thread, _TYPE_KEYWORDS,
        (_KEYWORD_LIMIT - _KEYWORD_SEPARATOR) * sizeof(size_t));
    memcpy(_atom_get_extra_address(_result), _list + _KEYWORD_SEPARATOR,
        (_KEYWORD_LIMIT - _KEYWORD_SEPARATOR) * sizeof(size_t));
    _atom_set_extra_used(_result, _max); /* 这是最大值 */
    _keyword_push(&_sorted, _result);
    return _result;
}

/* 分析一段文本，获取下一个关键词类型， */
static size_t _keyword_analyze(
        page * _thread, atom * _texts, atom * _unicodes,
        atom * _bytes, FILE * _file, const unsigned * _charset,
        unsigned _charset_size, unsigned * _range, size_t _range_size,
        atom * _leywords, atom * _analyze, atom * _temp) {

    if (_atom_get_extra_used(_analyze) && !_atom_get_extra_used(_temp)) {
        return _cache_pop_uint8(_analyze);
    }
    int _first = _text_read_char(
        _thread, _texts, _unicodes, _bytes, _file,
        _charset, _charset_size, _range, _range_size);
    if (_first == -1) return _KEYWORD_EOF;
    _cache_push_uint8(_thread, _analyze, _first);
    atom * _k = _atom_get_keyword_next(_leywords);
    _match:
    if (_first == ((unsigned char *) _atom_get_extra_address(_k))[0]) {
        for (int _i = 1; _i < _atom_get_extra_used(_k); _i ++) {
            int _other = _text_read_char(
                _thread, _texts, _unicodes, _bytes, _file,
                _charset, _charset_size, _range, _range_size);
            if (_other == -1) {
                goto _next;
            } if (_other != ((unsigned char *)
                    _atom_get_extra_address(_k))[_i]) {
                _cache_push_uint8(_thread, _texts, _other);
                goto _next;
            } else {
                _cache_push_uint8(_thread, _analyze, _other);
            }
        }
        return _atom_get_type(_k);
    }
    _next:
    if (_atom_get_extra_used(_analyze)) {
        _cache_push_multi(
            _thread, _texts, 1, (size_t) _atom_get_extra_used(_analyze) - 1,
            _atom_get_extra_address(_analyze) + 1);
        _atom_set_extra_used(_analyze, 1);
    }
    if (_atom_get_keyword_next(_k)) {
        _k = _atom_get_keyword_next(_k);
        goto _match;
    }
    /* 说明是文本，将当前文本暂时记录在临时缓存里，继续读取下一个单位 */
    _cache_push_uint8(_thread, _temp,
        ((unsigned char *)_atom_get_extra_address(_analyze))[0]);
    _cache_clear(_analyze);
    size_t _toword = _keyword_analyze(
            _thread, _texts, _unicodes, _bytes, _file, _charset,
            _charset_size, _range, _range_size, _leywords, _analyze, _temp);
    if (_toword != _KEYWORD_TEXT) {
        // printf("text assemble %zu %p <%s>\n",
        //     _toword, _analyze->text_length, _analyze->extra_address);
        _cache_push_uint8(_thread, _analyze, _toword);
        // printf("              %p <%s>\n",
        //     _analyze->text_length, _analyze->extra_address);
    }
    return _KEYWORD_TEXT;
}

/* 将标准文本以转译文本的方式写入缓存，从 _buffer 读取到 _texts */
static size_t _keyword_analyze_buffer(
        page * _thread, atom * _texts, atom * _unicodes, atom * _bytes,
        char * _buffer, size_t _length, const unsigned * _charset,
        unsigned _charset_size, unsigned * _range, size_t _range_size,
        atom * _keywords, atom * _analyze, atom * _temp, atom * _result) {

    if (_length == 0) return 1;
    _cache_push_multi(_thread, _bytes, 1, _length, _buffer);
    size_t _type, _count = 0, _max = 0, _piece = 0;
    atom * _cache;
    for (; ; ) {
        _type = _keyword_analyze(_thread, _texts, _unicodes,
            _bytes, 0, _charset, _charset_size, _range, _range_size,
            _thread->_page_main->_uniform_keywords,
            _analyze, _temp);
        _cache = _type == _KEYWORD_TEXT ? _temp : _analyze;
        if (_type == _KEYWORD_EOF) {
            if (_count != -1 && _count > _max) _max = _count;
            return _piece == 1 ? 0 : _max + 1;
        } else if (_type == _KEYWORD_SEPARATOR) {
            _piece += 2;
            if (_count != -1 && _count > _max) _max = _count;
            _count = 0;
        } else if (_type == _KEYWORD_WORD && _count != -1) {
            _piece += 2;
            _count += 1;
        } else {
            if (_type == _KEYWORD_TEXT) {
                _piece |= 1;
            } else {
                _piece += 2;
            }
            if (_count != -1 && _count > _max) _max = _count;
            _count = -1;
        }
        _cache_clear(_cache);
    }
}

#endif
