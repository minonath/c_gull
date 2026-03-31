#ifndef _GULL_CODE_STRUCTURE_H
#define _GULL_CODE_STRUCTURE_H 1

static void _code_to_structure(page * _thread, atom * _keywords,
    atom * _string, atom * _code, size_t _count);

#define _CODE_TO_STRUCTURE(code)                                             \
    _code_to_structure(_thread, _keywords, _string, code, _count + 1)

static void inline _code_push_hex(
        page * _thread, atom * _string, void * _address) {
    if (_address) {
        char _cache[32] = {0};
        uint8_t _index = 0;
        size_t _hex = (size_t) _address / 6 / sizeof(size_t);
        while (_hex > 0) {
            size_t _next = _hex / 16;
            size_t _one = _hex - _next * 16;
            if (_one < 10) {
                _cache[_index ++] = _one + '0';
            } else if (_one < 16) {
                _cache[_index ++] = _one - 10 + 'a';
            }
            _hex = _next;
        }
        _cache_push_multi(_thread, _string, 1, _index, _cache);
    } else {
        _cache_push_multi(_thread, _string, 0, 4, "null");
    }
}

static char * _code_to_structure_type(size_t _type) {
    switch (_type) {
        case _TYPE_LINE: return "LINE";
        case _TYPE_TAIL: return "TAIL";
        case _TYPE_PHRASE: return "PHRASE";
        case _TYPE_ACCESS: return "ACCESS";
        case _TYPE_EXECUTE: return "EXECUTE";
        case _TYPE_SEQUENCE: return "SEQUENCE";
        case _TYPE_SYNCHRONIC: return "SYNCHRONIC";
        case _TYPE_STATIC: return "STATIC";
        case _TYPE_COMMENT: return "COMMENT";
        case _TYPE_EXPLAIN: return "EXPLAIN";
        case _TYPE_INTERPRET: return "INTERPRET";
        case _TYPE_EXTEND: return "EXTEND";
        case _TYPE_EXTEND_EXPLAIN: return "EXTEND_EXPLAIN";
        case _TYPE_EXTEND_INTERPRET: return "EXTEND_INTERPRET";
        case _TYPE_MARK: return "MARK";
        case _TYPE_JUMP: return "JUMP";
        case _TYPE_RETURN: return "RETURN";
    }
    return "MISC";
}

static void _code_to_structure_inner(page * _thread, atom * _keywords,
        atom * _string, atom * _code, size_t _count) {
    for (int _i = 0; _i < _count; _i ++)
        _cache_push_multi(_thread, _string, 0, 3, "   ");
    _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_L);

    char * _inner = _code_to_structure_type(_atom_get_type(_code));

    _cache_push_multi(_thread, _string, 0, strlen(_inner), _inner);
    _cache_push_multi(_thread, _string, 0, 6, " this:");
    _code_push_hex(_thread, _string, _code);
    _cache_push_multi(_thread, _string, 0, 7, " first:");
    _code_push_hex(_thread, _string, _atom_get_code_first(_code));
    _cache_push_multi(_thread, _string, 0, 10, " previous:");
    _code_push_hex(_thread, _string, _atom_get_code_previous(_code));

    if (_atom_get_code_current(_code)) {
        _cache_push_uint8(_thread, _string, '\n');
        _CODE_TO_STRUCTURE(_atom_get_code_current(_code));
    } else {
        _cache_push_multi(_thread, _string, 0, 5, " NULL");
    }
    if (_atom_get_code_next(_code)) {
        _cache_push_uint8(_thread, _string, '\n');
        _CODE_TO_STRUCTURE(_atom_get_code_next(_code));
    } else {
        _cache_push_multi(_thread, _string, 0, 5, " NULL");
    }
    _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_R);
}

static void _code_to_structure_brief(page * _thread, atom * _keywords,
        atom * _string, atom * _code, size_t _count) {

    for (int _i = 0; _i < _count; _i ++)
        _cache_push_multi(_thread, _string, 0, 3, "   ");
    _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_L);
    _cache_push_multi(_thread, _string, 0, 6, "BRIEF ");

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
    _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_R);
}

static void _code_to_structure_text(page * _thread, atom * _keywords,
        atom * _string, atom * _code, size_t _count) {
    for (int _i = 0; _i < _count; _i ++)
        _cache_push_multi(_thread, _string, 0, 3, "   ");
    _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_L);
    _cache_push_multi(_thread, _string, 0, 5, "TEXT ");

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
    _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_R);
}

static void _code_to_structure(page * _thread, atom * _keywords,
        atom * _string, atom * _code, size_t _count) {
    #if (defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64)
        if (_atom_get_info(_code) == _TYPE_INFO_BRIEF_64) {
            _code_to_structure_brief(
                _thread, _keywords, _string, _code, _count);
        } else
    #endif
    if (_atom_get_info(_code) == _TYPE_INFO_HALF) {
        if (_atom_get_type(_code) == _TYPE_BASE) {
            for (int _i = 0; _i < _count; _i ++)
                _cache_push_multi(_thread, _string, 0, 3, "   ");
            _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_L);
            _cache_push_multi(_thread, _string, 0, 5, "BASE ");
            _CODE_TO_STRING_DEFAULT(_KEYWORD_BASE);
            _CODE_TO_STRING_DEFAULT(_KEYWORD_EXECUTE_R);
        }
    } else if (_atom_get_info(_code) == _TYPE_INFO_FULL) {
        #if !((defined(__LP64__) && __LP64__) || (defined(_LP64) && _LP64))
            if (_atom_get_type(_code) == _TYPE_BRIEF_32) {
                _code_to_structure_brief(
                    _thread, _keywords, _string, _code, _count);
            } else
        #endif
        if (_atom_get_type(_code) == _TYPE_TEXT) {
            _code_to_structure_text(
                _thread, _keywords, _string, _code, _count);
        } else if (_code_check(_code)) {
            _code_to_structure_inner(
                _thread, _keywords, _string, _code, _count);
        }
    }
}

#endif
