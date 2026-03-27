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
    atom * _records = _atom_cache(_thread, sizeof(size_t));
    atom * _texts = _atom_cache(_thread, sizeof(size_t));
    atom * _unicodes = _atom_cache(_thread, sizeof(size_t));
    atom * _bytes = _atom_cache(_thread, sizeof(size_t));

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

    _page_release_extra(_thread, _records);
    _page_release_extra(_thread, _texts);
    _page_release_extra(_thread, _unicodes);
    _page_release_extra(_thread, _bytes);
    _page_release_double(_thread, _records);
    _page_release_double(_thread, _texts);
    _page_release_double(_thread, _unicodes);
    _page_release_double(_thread, _bytes);

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

    atom * _result = _page_allocate_bind(
        _thread, _TYPE_KEYWORDS, _KEYWORD_LIMIT * sizeof(size_t));
    memcpy(_atom_get_extra_address(_result),
        _list, _KEYWORD_LIMIT * sizeof(size_t));
    _atom_set_extra_used(_result, _max); /* 这是最大值 */
    _keyword_push(&_sorted, _result);
    return _result;
}

#endif
