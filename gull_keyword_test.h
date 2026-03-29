#ifndef _GULL_KEYWORD_TEST_H
#define _GULL_KEYWORD_TEST_H 1

#include <stdio.h>
#include "gull_keyword.h"

void _extra_string_print(atom * _extra) {
    char * _address = _atom_get_extra_address(_extra);
    _address[_atom_get_extra_used(_extra)] = 0;
    printf("\"%s\"\n", _address);
}

void gull_keyword_test() {
    page * _process = _page_allocate_process(_PAGE_SIZE_4K, 12);

    char * _test[] = {(char *) _KEYWORD_ACCESS, "hello test", "."};

    _process->_uniform_bulitins = _page_allocate_thread(_process);
    _process->_uniform_keywords = _keyword_initial(
        _process->_uniform_bulitins, 0, 0, 0, 0, _test, 3);

    atom * _keyword = _process->_uniform_keywords;
    for (;;) {
        printf("%zu %s\n", _atom_get_type(_keyword),
            _atom_get_extra_address(_keyword));
        _keyword = _atom_get_keyword_next(_keyword);
        if (_keyword == 0) break;
    }
    _keyword = _process->_uniform_keywords;
    atom ** _address = _atom_get_extra_address(_keyword);
    for (size_t _i = 0; _i < _KEYWORD_LIMIT - _KEYWORD_SEPARATOR; _i ++) {
        printf("\t%zu: %zu %s\n", _i, _atom_get_type(_address[_i]),
            _atom_get_extra_address(_address[_i]));
    }

    atom * _texts = _cache_allocate(_process, sizeof(size_t));
    atom * _unicodes = _cache_allocate(_process, sizeof(size_t));
    atom * _bytes = _cache_allocate(_process, sizeof(size_t));
    atom * _analyze = _cache_allocate(_process, sizeof(size_t));
    atom * _temp = _cache_allocate(_process, sizeof(size_t));

    char _meow[] = "(wow hello test world).(@(&:meow))";
    _cache_push_multi(_process, _bytes, 1, strlen(_meow), _meow);

    size_t _type = _keyword_analyze(
        _process, _texts, _unicodes, _bytes, 0, 0, 0,
        0, 0, _process->_uniform_keywords, _analyze, _temp);
    atom * _cache = _type == _KEYWORD_TEXT ? _temp : _analyze;

    for (;;) {
        printf("%zu ", _type);
        _extra_string_print(_cache);
        if (_type == _KEYWORD_EOF) {
            break;
        }
        _cache_clear(_cache);
        _type = _keyword_analyze(
            _process, _texts, _unicodes, _bytes,  0, 0, 0,
            0, 0, _process->_uniform_keywords, _analyze, _temp);
        _cache = _type == _KEYWORD_TEXT ? _temp : _analyze;
    }
}

#endif
