#ifndef _GULL_CODE_H
#define _GULL_CODE_H 1

#include "gull_page.h"
#include "gull_text.h"
#include "gull_keyword.h"

static inline size_t _code_check(atom * _atom) {
    return _atom_get_type(_atom) >= _TYPE_LINE &&
        _atom_get_type(_atom) <= _TYPE_ACCESS;
}

/* 在没有指定的情况下，所有的 first 都指向自己 */
static inline atom * _code_allocate(page * _thread, size_t _type) {
    atom * _atom = _page_allocate_double(_thread);
    _atom_set_info(_atom, _TYPE_INFO_FULL);
    _atom_set_reference(_atom, 1); /* 立即使用 */
    _atom_set_type(_atom, _type);
    return _atom;
}

/* line 需要初始化 */
static inline atom * _code_line(page * _thread) {
    atom * _atom = _code_allocate(_thread, _TYPE_LINE);
    _atom_set_code_first(_atom, _atom);
    _atom_set_code_previous(_atom, 0);
    _atom_set_code_current(_atom, 0);
    _atom_set_code_next(_atom, 0);
    return _atom;
}

/* 需要继承 first 的模块 */
static inline atom * _code_inherit(
        page * _thread, size_t _type, atom * _previous) {
    atom * _atom = _page_allocate_double(_thread);
    _atom_set_info(_atom, _TYPE_INFO_FULL);
    _atom_set_reference(_atom, 1); /* 立即使用 */
    _atom_set_type(_atom, _type);
    _atom_set_code_first(_atom, _atom_get_code_first(_previous));
    _atom_set_code_previous(_atom, _previous);
    return _atom;
}

static void _code_next(page * _thread, atom ** _cursor) {
    /* 如果后续链表没有使用，就阻止游标移动，继续使用当前链表 */
    if (_atom_get_code_next(*_cursor) == 0) return;
    atom * _link = _code_inherit(
        _thread, _atom_allow_code_access(*_cursor) ?
        _TYPE_ACCESS : _TYPE_TAIL, *_cursor);
    _atom_set_code_current(_link, _atom_get_code_next(*_cursor));
    _atom_set_code_next(_link, 0);
    if (_atom_get_code_next(*_cursor) &&
            _code_check(_atom_get_code_next(*_cursor))) {
        _atom_set_code_previous(_atom_get_code_next(*_cursor), _link);
    }
    _atom_set_code_next(*_cursor, _link);
    *_cursor = _link;
}

/* 下移分支链表 */
static void _code_down(page * _thread, atom ** _cursor, size_t _type) {
    /* 这里保证 code_next 一定是空的 */
    _code_next(_thread, _cursor);
    atom * _link = _code_allocate(_thread, _type);
    _atom_set_code_first(_link, _link);
    _atom_set_code_previous(_link, *_cursor);
    _atom_set_code_current(_link, 0);
    _atom_set_code_next(_link, 0);

    if (_atom_get_code_current(*_cursor)) {
        _atom_set_code_next(*_cursor, _link);
    } else {
        _atom_set_code_current(*_cursor, _link);
    }
    *_cursor = _link;
}

static void _code_down_twice(
        page * _thread, atom ** _cursor, size_t _type, size_t _twice) {
    if (_twice) { /* 二次分支链表 */
        _code_next(_thread, _cursor);
        /* 后续会转为 _TYPE_SEQUENCE */
        atom * _quick = _code_allocate(_thread, _TYPE_QUICK);
        atom * _link = _code_allocate(_thread, _type);
        _atom_set_code_first(_quick, _quick);
        _atom_set_code_previous(_quick, *_cursor);
        _atom_set_code_current(_quick, _link);
        _atom_set_code_next(_quick, 0);
        _atom_set_code_first(_link, _link);
        _atom_set_code_previous(_link, _quick);
        _atom_set_code_current(_link, 0);
        _atom_set_code_next(_link, 0);

        if (_atom_get_code_current(*_cursor)) {
            _atom_set_code_next(*_cursor, _quick);
        } else {
            _atom_set_code_current(*_cursor, _quick);
        }
        *_cursor = _link;
    } else {
        _code_down(_thread, _cursor, _type);
    }
}

/* 一个 code 只会包含 code text base 三种原子，base 原子无需删除 */
static inline void _code_release_direct(page * _thread, atom * _code) {
    #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
        if (_atom_get_info(_code) == _TYPE_INFO_BRIEF_64) {
            _page_release_single(_thread, _code);
        } else
    #endif
    if (_atom_get_info(_code) == _TYPE_INFO_FULL) {
        #if !((defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64))
            if (_atom_get_type(_code) == _TYPE_BRIEF_32) {
                _page_release_double(_thread, _code);
            } else
        #endif
        if (_atom_get_type(_code) == _TYPE_TEXT) {
            _page_release_extra(_thread, _code);
            _page_release_double(_thread, _code);
        } else if (_code_check(_code)) {
            if (_atom_get_code_next(_code))
                _code_release_direct(_thread, _atom_get_code_next(_code));
            if (_atom_get_code_current(_code))
                _code_release_direct(_thread, _atom_get_code_current(_code));
        }
    }
    /* 这里仅做简易回收，删除时不考虑引用数量，正式回收需要按引用数量进行 */
}

static inline void _code_clear_line(page * _thread, atom ** _cursor) {
    while (_atom_get_type(*_cursor) != _TYPE_LINE) {
        *_cursor = _atom_get_code_first(*_cursor);
        if (_atom_allow_code_up(*_cursor)) {
            *_cursor = _atom_get_code_previous(*_cursor);
        }
    }
    if (_atom_get_code_current(*_cursor)) {
        _code_release_direct(_thread, _atom_get_code_current(*_cursor));
        _atom_set_code_current(*_cursor, 0);
    }
    if (_atom_get_code_next(*_cursor)) {
        _code_release_direct(_thread, _atom_get_code_next(*_cursor));
        _atom_set_code_next(*_cursor, 0);
    }
}

/* 自动越过多余结构 */
static void _code_auto(atom ** _cursor) {
    for (atom * _temp; ; ) {
        _temp = _atom_get_code_first(*_cursor);
        if (_atom_allow_code_quick(_temp)) { /* 单独分支的链表 */
            _temp = _atom_get_code_previous(_temp);
            if (_atom_get_type(_temp) == _TYPE_QUICK) {
                _atom_set_type(_temp, _TYPE_SEQUENCE);
                _temp = _atom_get_code_previous(_temp);
            }
            *_cursor = _temp;
        } else {
            break;
        }
    }
}

/* 上移返回主链表 */
static size_t _code_up(atom ** _cursor) {
    atom * _temp = _atom_get_code_first(*_cursor);
    if (_atom_get_code_previous(_temp) && _atom_allow_code_up(_temp)) {
        *_cursor = _atom_get_code_previous(_temp);
        _code_auto(_cursor);
        return 0;
    }
    return 1; /* 已经抵达顶层，无法上移 */
}

#define _code_read_first() {                                                 \
    _type = _keyword_analyze(                                                \
        _thread, _texts, _unicodes, _bytes, _file, _charset, _charset_size,  \
        _range, _range_size, _keywords, _analyze, _temp);                    \
    _cache = _type == _KEYWORD_TEXT ? _temp : _analyze;                      \
}

#define _code_read_one _cache_clear(_cache); _code_read_first

#define _code_concat(cache) _cache_push_multi(_thread, cache, 0,             \
    _atom_get_extra_used(_cache), _atom_get_extra_address(_cache))

static void _code_analyze(
        page * _thread, atom * _texts, atom * _unicodes,
        atom * _bytes, FILE * _file, const unsigned * _charset,
        unsigned _charset_size, unsigned * _range, size_t _range_size,
        atom * _keywords, atom * _analyze, atom * _temp,
        atom ** _cursor, atom * _assist, atom * _words,
        size_t * _level, size_t * _max, size_t * _piece) {
    size_t _type;
    atom * _cache;
    for (; ; ) {
        _code_read_first();
        _start:
        // printf("start assist_used:%zu level:%zu max:%zu piece:%zu\n",
        //     _atom_get_extra_used(_assist), *_level, *_max, *_piece);
        if (_atom_get_extra_used(_assist)) {
            for (; ; ) {
                if (_type == _KEYWORD_EOF) {
                    return;
                } else if (_type == _KEYWORD_ASSIST) {
                    /* 辅助中出现辅助标记表示取消 */
                    _cache_clear(_assist);
                    _code_clear_line(_thread, _cursor);
                    _code_read_one();
                    break;
                } else if (_type == _KEYWORD_SEPARATOR) {
                    /* 保留任何隔离符号 */
                    _code_concat(_assist);
                    _code_read_one();
                } else {
                    /* 结束并转到普通模式，并将转译字符转为真实字符 */
                    _code_down(_thread, _cursor, _TYPE_COMMENT);
                    _atom_set_code_next(*_cursor, _text_build(
                        _thread, _atom_get_extra_used(_assist) - 1, 1,
                        _atom_get_extra_address(_assist) + 1));
                    *_cursor = _atom_get_code_previous(*_cursor);
                    _cache_clear(_assist);
                    break;
                }
            }
        } else if (*_level) {
            size_t _count = (*_piece == 1 || *_piece == 2) ? -1 : 0;
            for (size_t _keep = _atom_get_extra_used(_words);;) {
                if (_type == _KEYWORD_EOF) {
                    return;
                } else if (_type == _KEYWORD_SEPARATOR) {
                    *_piece += 2;
                    // printf("read type %zu %zu\n", _type, *_piece);
                    if (_count != -1 && _count > *_max) *_max = _count;
                    _count = 0;
                    _keep = _atom_get_extra_used(_words);
                    _code_concat(_words);
                } else if (_type == _KEYWORD_WORD && _count != -1) {
                    *_piece += 2;
                    // printf("read type %zu %zu\n", _type, *_piece);
                    _count += 1;
                    _code_concat(_words);
                    if (_count == *_level) {
                        _code_next(_thread, _cursor);
                        // printf("piece:%zu level:%zu max:%zu keep:%zu\n",
                        //     *_piece, *_level, *_max, _keep);
                        if (_atom_get_code_current(*_cursor)) {
                            _atom_set_code_next(*_cursor, _text_build(
                                _thread, _keep, *_piece / 2 > *_level + 1 ?
                                *_max + 1 : *_piece & 1 ? 0 : 1,
                                _atom_get_extra_address(_words)));
                        } else {
                            _atom_set_code_current(*_cursor, _text_build(
                                _thread, _keep, *_piece / 2 > *_level + 1 ?
                                *_max + 1 : *_piece & 1 ? 0 : 1,
                                _atom_get_extra_address(_words)));
                        }
                        *_level = 0;
                        _cache_clear(_words);
                        _code_auto(_cursor);
                        _code_read_one();
                        break;
                    }
                } else {
                    if (_type == _KEYWORD_TEXT) {
                        // printf("read text %zu %s\n",
                        //     _cache->text_length, _cache->extra_address);
                        *_piece |= 1;
                    } else {
                        *_piece += 2;
                    }
                    // printf("read type %zu %zu\n", _type, *_piece);
                    if (_count != -1 && _count > *_max) *_max = _count;
                    _count = -1;
                    _code_concat(_words);
                }
                _code_read_one();
            }
        }

        for (; ; ) {
            switch (_type) {
                case _KEYWORD_TEXT:
                    _code_next(_thread, _cursor);
                    if (_atom_get_code_current(*_cursor)) {
                        _atom_set_code_next(*_cursor, _text_build(
                            _thread, _atom_get_extra_used(_cache), 0,
                            _atom_get_extra_address(_cache)));
                    } else {
                        _atom_set_code_current(*_cursor, _text_build(
                            _thread, _atom_get_extra_used(_cache), 0,
                            _atom_get_extra_address(_cache)));
                    }
                    _code_auto(_cursor);
                    break;
                case _KEYWORD_EXECUTE_L:
                    _code_down(_thread, _cursor, _TYPE_EXECUTE);
                    break;
                case _KEYWORD_SEQUENCE_L:
                    _code_down(_thread, _cursor, _TYPE_SEQUENCE);
                    break;
                case _KEYWORD_SYNCHRONIC_L:
                    _code_down(_thread, _cursor, _TYPE_SYNCHRONIC);
                    break;
                case _KEYWORD_STATIC_L:
                    _code_down(_thread, _cursor, _TYPE_STATIC);
                    break;
                case _KEYWORD_EXECUTE_R:
                case _KEYWORD_SEQUENCE_R:
                case _KEYWORD_SYNCHRONIC_R:
                case _KEYWORD_STATIC_R:
                    if (_atom_allow_code_access(*_cursor) || /* 提前结束错误 */
                            _code_up(_cursor)) { /* 右关键词比左边多 */
                        return;
                    }
                    break;
                case _KEYWORD_EXTEND:
                    _code_down_twice(_thread, _cursor, _TYPE_EXTEND,
                        _atom_allow_code_access(*_cursor));
                        break;
                case _KEYWORD_MARK:
                    _code_down_twice(_thread, _cursor, _TYPE_MARK,
                        _atom_allow_code_access(*_cursor));
                        break;
                case _KEYWORD_JUMP:
                    _code_down_twice(_thread, _cursor, _TYPE_JUMP,
                        _atom_allow_code_access(*_cursor));
                        break;
                case _KEYWORD_EXPLAIN:
                    if (_atom_get_type(*_cursor) == _TYPE_EXTEND) {
                        _atom_set_type(*_cursor, _TYPE_EXTEND_EXPLAIN);
                    } else {
                        _code_down_twice(_thread, _cursor, _TYPE_EXPLAIN,
                            _atom_allow_code_access(*_cursor));
                    }
                    break;
                case _KEYWORD_INTERPRET:
                    if (_atom_get_type(*_cursor) == _TYPE_EXTEND) {
                        _atom_set_type(*_cursor, _TYPE_EXTEND_INTERPRET);
                    } else {
                        _code_down_twice(_thread, _cursor, _TYPE_INTERPRET,
                            _atom_allow_code_access(*_cursor));
                    }
                    break;
                case _KEYWORD_COMMENT:
                    /* 注释遇到非首位的情况 */
                    if (_atom_allow_code_access(*_cursor)) {
                        /* 连续注释视为同一个 */
                        if (_atom_get_type(*_cursor) != _TYPE_COMMENT) {
                            return;
                        }
                    } else {
                        _code_down(_thread, _cursor, _TYPE_COMMENT);
                    }
                    break;
                case _KEYWORD_RETURN:
                    if (_atom_allow_code_access(*_cursor)) {
                        /* 连续返回视为返回空值 */
                        if (_atom_get_type(*_cursor) != _TYPE_RETURN) {
                            _code_auto(_cursor);
                        } else {
                            return; /* 返回遇到非首位的情况，打断 */
                        }
                    } else {
                        _code_down(_thread, _cursor, _TYPE_RETURN);
                    }
                    break;
                case _KEYWORD_BASE:
                    _code_next(_thread, _cursor);
                    if (_atom_get_code_current(*_cursor)) {
                        _atom_set_code_next(*_cursor,
                            (atom *) _thread->_page_main->_uniform_base);
                    } else {
                        _atom_set_code_current(*_cursor,
                            (atom *) _thread->_page_main->_uniform_base);
                    }
                    size_t _r = _atom_get_reference(
                        (atom *) _thread->_page_main->_uniform_base);
                    _atom_set_reference(
                        (atom *) _thread->_page_main->_uniform_base, _r + 1);
                    _code_auto(_cursor);
                    break;
                case _KEYWORD_CANCEL: /* 清理当前行 */
                    _code_clear_line(_thread, _cursor);
                    break;
                case _KEYWORD_ACCESS:
                    /* 如果当前的游标是单独分支链表的头部，不允许增加连接 */
                    if (_atom_allow_code_access(*_cursor)) {
                        return;
                    } else if (_atom_get_code_next(*_cursor)) {
                        atom * _next = _atom_get_code_next(*_cursor);
                        if (_atom_allow_code_quick(_next)) {
                            if (_atom_get_type(_next) == _TYPE_COMMENT &&
                                    !_atom_get_code_current(_next)) {
                                /* 辅助转为注释 */
                                _atom_set_code_current(
                                    _next, _atom_get_code_next(_next));
                                _atom_set_code_next(_next, 0);
                            }
                            while (_atom_get_code_next(_next)) {
                                atom * _temp = _atom_get_code_next(_next);
                                if (_atom_get_type(_temp) == _TYPE_ACCESS) {
                                    _next = _temp;
                                } else {
                                    break;
                                }
                            }
                            *_cursor = _next;
                        } else {
                            atom * _phrase =
                                _code_allocate(_thread, _TYPE_PHRASE);
                            _atom_set_code_first(_phrase, _phrase);
                            _atom_set_code_previous(_phrase, *_cursor);
                            _atom_set_code_next(*_cursor, _phrase);
                            _atom_set_code_current(_phrase, _next);
                            if (_code_check(_next)) {
                                _atom_set_code_first(_next, _phrase);
                                _atom_set_code_previous(_next, _phrase);
                            }
                            *_cursor = _phrase;
                       }
                    } else if (_atom_get_code_current(*_cursor)) {
                        atom * _current = _atom_get_code_current(*_cursor);
                        if (_atom_allow_code_quick(_current)) {
                            while (_atom_get_code_next(_current)) {
                                atom * _temp = _atom_get_code_next(_current);
                                if (_atom_get_type(_temp) == _TYPE_ACCESS) {
                                    _current = _temp;
                                } else {
                                    break;
                                }
                            }
                            *_cursor = _current;
                        } else {
                            atom * _phrase =
                                _code_allocate(_thread, _TYPE_PHRASE);
                            _atom_set_code_first(_phrase, _phrase);
                            _atom_set_code_previous(_phrase, *_cursor);
                            _atom_set_code_current(*_cursor, _phrase);
                            _atom_set_code_current(_phrase, _current);
                            if (_code_check(_current)) {
                                _atom_set_code_first(_current, _phrase);
                                _atom_set_code_previous(_current, _phrase);
                            }
                            *_cursor = _phrase;
                        }
                    } else {
                        return;
                    }
                    break;
                case _KEYWORD_WORD:
                    *_level = 1;
                    *_max = 0;
                    for (; ; ) {
                        _code_read_one(); /* 读取下一个 */
                        // printf("read type first %zu\n", _type);
                        /* 分割符号表示开始记录 */
                        if (_type == _KEYWORD_SEPARATOR) {
                            *_piece = 0;
                        } else if (_type == _KEYWORD_WORD) {
                            *_level += 1;
                            continue;
                        } else {
                            if (_type == _KEYWORD_TEXT) {
                                *_piece = 1;
                            } else {
                                *_piece = 2;
                            }
                            _code_concat(_words);
                        }
                        _code_read_one();
                        goto _start;
                    }
                case _KEYWORD_ASSIST:
                    if (_atom_allow_code_access(*_cursor)) {
                        return;
                    } else {
                        _cache_push_uint8(_thread, _assist, _KEYWORD_ASSIST);
                        _code_read_one();
                        goto _start;
                    }
                    break;
                case _KEYWORD_EOF:
                    return;
            }
            _code_read_one();
        }
    }
}

static void _code_to_string(
        page * _thread, atom * _keywords, atom * _string, atom * _code);

static void inline _code_to_string_default(
        page * _thread, atom * _keywords, atom * _string, size_t _type) {
    atom * _default = ((atom **) _atom_get_extra_address(
        _keywords))[_type - _KEYWORD_SEPARATOR];
    _cache_push_multi(_thread, _string, 0,
        _atom_get_extra_used(_default), _atom_get_extra_address(_default));
}

static inline void _code_to_string_words(
        page * _thread, atom * _keywords, atom * _string, size_t _time) {
    atom * _word = ((atom **) _atom_get_extra_address(
        _keywords))[_KEYWORD_WORD - _KEYWORD_SEPARATOR];
    for (int _i = 0; _i < _time; _i ++)
        _cache_push_multi(_thread, _string, 0,
            _atom_get_extra_used(_word), _atom_get_extra_address(_word));
}

static inline void _code_to_string_separator(
        page * _thread, atom * _keywords, atom * _string, atom * _current) {
    if (!(_atom_get_type(_current) == _TYPE_COMMENT &&
            _atom_get_code_next(_current) &&
            _atom_get_code_current(_current) == 0)) {
        _code_to_string_default(
            _thread, _keywords, _string, _KEYWORD_SEPARATOR);
    }
}

#define _CODE_TO_STRING(code)                                                \
    _code_to_string(_thread, _keywords, _string, code)

#define _CODE_TO_STRING_DEFAULT(type)                                        \
    _code_to_string_default(_thread, _keywords, _string, type)

#define _CODE_TO_STRING_WORDS(time)                                          \
    _code_to_string_words(_thread, _keywords, _string, time)

#define _CODE_TO_STRING_SEPARATOR(code)                                      \
    _code_to_string_separator(_thread, _keywords, _string, code)

static inline void _code_to_string_escape(
        page * _thread, atom * _keywords, atom * _string, atom *_text) {
    #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
        if (_atom_get_info(_text) == _TYPE_INFO_BRIEF_64) {
            _cache_push_escaped_2_unicode(
                _thread, _string, _atom_get_brief_address(_text));
        } else
    #endif
    if (_atom_get_info(_text) == _TYPE_INFO_FULL) {
        #if !((defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64))
            if (_atom_get_type(_text) == _TYPE_BRIEF_32) {
                _cache_push_escaped_2_unicode(
                    _thread, _string, _atom_get_brief_address(_text));
            } else
        #endif
        if (_atom_get_type(_text) == _TYPE_TEXT) {
            _cache_push_escaped_2_unicode(
                _thread, _string, _atom_get_extra_address(_text));
        }
    }
}

static void _code_to_string_inner(
        page * _thread, atom * _keywords, atom * _string, atom * _code) {
    // printf("code type %zu\n", _atom_get_type(_code));
    // if (_atom_get_code_current(_code))
    //     printf("\tcode current type %zu\n",
    //         _atom_get_type(_atom_get_code_current(_code)));
    // if (_atom_get_code_next(_code))
    //     printf("\tcode next type %zu\n",
    //         _atom_get_type(_atom_get_code_next(_code)));

    switch (_atom_get_type(_code)) {
        case _TYPE_LINE:
        case _TYPE_TAIL:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING(_atom_get_code_current(_code));
                if (_atom_get_type(
                        _atom_get_code_next(_code)) == _TYPE_LINE) {
                    _cache_push_uint8(_thread, _string, '\n');
                } else {
                    _CODE_TO_STRING_SEPARATOR(_atom_get_code_current(_code));
                }
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_PHRASE:
        case _TYPE_ACCESS:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            }
            break;
        case _TYPE_EXECUTE:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_L);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_SEPARATOR(_atom_get_code_current(_code));
                _CODE_TO_STRING(_atom_get_code_next(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_R);
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_L);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_R);
            } else {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_L);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_R);
            }
            break;
        case _TYPE_SEQUENCE:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SEQUENCE_L);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_SEPARATOR(_atom_get_code_current(_code));
                _CODE_TO_STRING(_atom_get_code_next(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SEQUENCE_R);
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SEQUENCE_L);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SEQUENCE_R);
            } else {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SEQUENCE_L);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SEQUENCE_R);
            }
            break;
        case _TYPE_SYNCHRONIC:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SYNCHRONIC_L);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_SEPARATOR(_atom_get_code_current(_code));
                _CODE_TO_STRING(_atom_get_code_next(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SYNCHRONIC_R);
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SYNCHRONIC_L);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SYNCHRONIC_R);
            } else {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SYNCHRONIC_L);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_SYNCHRONIC_R);
            }
            break;
        case _TYPE_STATIC:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_STATIC_L);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_SEPARATOR(_atom_get_code_current(_code));
                _CODE_TO_STRING(_atom_get_code_next(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_STATIC_R);
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_STATIC_L);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_STATIC_R);
            } else {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_STATIC_L);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_STATIC_R);
            }
            break;
        case _TYPE_COMMENT:
            if (_atom_get_code_next(_code)) {
                if (_atom_get_code_current(_code)) {
                    _CODE_TO_STRING_DEFAULT(_KEYWORD_COMMENT);
                    _CODE_TO_STRING(_atom_get_code_current(_code));
                    _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                    _CODE_TO_STRING(_atom_get_code_next(_code));
                } else {
                    _CODE_TO_STRING_DEFAULT(_KEYWORD_ASSIST);
                    atom * _text = _atom_get_code_next(_code);
                    _code_to_string_escape(
                        _thread, _keywords, _string, _text);
                }
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_COMMENT);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_EXPLAIN:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXPLAIN);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXPLAIN);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_INTERPRET:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_INTERPRET);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_INTERPRET);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_EXTEND:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXTEND);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXTEND);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_EXTEND_EXPLAIN:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXTEND);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXPLAIN);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXTEND);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXPLAIN);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_EXTEND_INTERPRET:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXTEND);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_INTERPRET);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_EXTEND);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_INTERPRET);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_MARK:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_MARK);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_MARK);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_JUMP:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_JUMP);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_JUMP);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            }
            break;
        case _TYPE_RETURN:
            if (_atom_get_code_next(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_RETURN);
                _CODE_TO_STRING(_atom_get_code_current(_code));
                _CODE_TO_STRING_DEFAULT(_KEYWORD_ACCESS);
                _CODE_TO_STRING(_atom_get_code_next(_code));
            } else if (_atom_get_code_current(_code)) {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_RETURN);
                _CODE_TO_STRING(_atom_get_code_current(_code));
            } else {
                _CODE_TO_STRING_DEFAULT(_KEYWORD_RETURN);
                _CODE_TO_STRING_DEFAULT(_KEYWORD_RETURN);
            }
            break;
    }
}

static inline void _code_to_string_brief(
        page * _thread, atom * _keywords, atom * _string, atom * _code) {
    char * _address = _atom_get_brief_address(_code);
    size_t _length = _text_brief_count(_address);
    size_t _protect = _atom_get_brief_protect(_code);
    if (_protect) {
        if (_length) {
            _CODE_TO_STRING_WORDS(_protect);
            _CODE_TO_STRING_DEFAULT(_KEYWORD_SEPARATOR);
            _cache_push_multi(_thread, _string, 0, _length, _address);
            _CODE_TO_STRING_DEFAULT(_KEYWORD_SEPARATOR);
            _CODE_TO_STRING_WORDS(_protect);
        } else {
            _CODE_TO_STRING_WORDS(_protect);
            _CODE_TO_STRING_DEFAULT(_KEYWORD_SEPARATOR);
            _CODE_TO_STRING_WORDS(_protect);
        }
    } else {
        _cache_push_multi(_thread, _string, 0, _length, _address);
    }
}

static inline void _code_to_string_text(
        page * _thread, atom * _keywords, atom * _string, atom * _code) {
    char * _address = _atom_get_extra_address(_code);
    size_t _length = _atom_get_extra_used(_code);
    size_t _protect = _atom_get_text_protect(_code);
    if (_protect) {
        if (_length) {
            _CODE_TO_STRING_WORDS(_protect);
            _CODE_TO_STRING_DEFAULT(_KEYWORD_SEPARATOR);
            _cache_push_multi(_thread, _string, 0, _length, _address);
            _CODE_TO_STRING_DEFAULT(_KEYWORD_SEPARATOR);
            _CODE_TO_STRING_WORDS(_protect);
        } else {
            _CODE_TO_STRING_WORDS(_protect);
            _CODE_TO_STRING_DEFAULT(_KEYWORD_SEPARATOR);
            _CODE_TO_STRING_WORDS(_protect);
        }
    } else {
        _cache_push_multi(_thread, _string, 0, _length, _address);
    }
}

/*
    #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
        if (_atom_get_info(_code) == _TYPE_INFO_BRIEF_64) {

        } else
    #endif
    if (_atom_get_info(_code) == _TYPE_INFO_HALF) {
        if (_atom_get_type(_code) == _TYPE_BASE) {

        }
    } else if (_atom_get_info(_code) == _TYPE_INFO_FULL) {
        #if !((defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64))
            if (_atom_get_type(_code) == _TYPE_BRIEF_32) {

            } else
        #endif
        if (_atom_get_type(_code) == _TYPE_TEXT) {

        } else if (_code_check(_code)) {

        }
    }
*/
static void _code_to_string(
        page * _thread, atom * _keywords, atom * _string, atom * _code) {
    #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
        if (_atom_get_info(_code) == _TYPE_INFO_BRIEF_64) {
            _code_to_string_brief(_thread, _keywords, _string, _code);
        } else
    #endif
    if (_atom_get_info(_code) == _TYPE_INFO_HALF) {
        if (_atom_get_type(_code) == _TYPE_BASE) {
            _CODE_TO_STRING_DEFAULT(_KEYWORD_BASE);
        }
    } else if (_atom_get_info(_code) == _TYPE_INFO_FULL) {
        #if !((defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64))
            if (_atom_get_type(_code) == _TYPE_BRIEF_32) {
                _code_to_string_brief(_thread, _keywords, _string, _code);
            } else
        #endif
        if (_atom_get_type(_code) == _TYPE_TEXT) {
            _code_to_string_text(_thread, _keywords, _string, _code);
        } else if (_code_check(_code)) {
            _code_to_string_inner(_thread, _keywords, _string, _code);
        }
    }
}

#endif
