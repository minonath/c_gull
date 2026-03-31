#ifndef _GULL_CODE_TEST_H
#define _GULL_CODE_TEST_H 1

#include "gull_code.h"
#include "gull_code_structure.h"

#define _CODE_TEST_LOOP(test) {                                              \
    _cache_push_multi(_process, _bytes, 1, strlen(test), test);              \
    _code_analyze(                                                           \
        _process, _texts, _unicodes, _bytes, 0, 0, 0, 0, 0,                  \
        _process->_uniform_keywords, _analyze, _temp, &_cursor,              \
        _assist, _words, &_level, &_max, &_piece);                           \
    _cache_clear(_string);                                                   \
    _code_to_string(                                                         \
        _process, _process->_uniform_keywords, _string, _keep);              \
    size_t _used = _atom_get_extra_used(_string);                            \
    char * _address = _atom_get_extra_address(_string);                      \
    _address[_used] = 0;                                                     \
    printf("<< %zu >> %s\n\n", _used, _address);                             \
    _cache_clear(_string);                                                   \
    _code_to_structure(                                                      \
        _process, _process->_uniform_keywords, _string, _keep, 0);           \
    _used = _atom_get_extra_used(_string);                                   \
    _address = _atom_get_extra_address(_string);                             \
    _address[_used] = 0;                                                     \
    printf("%s\n\n", _address);                                              \
}


void gull_code_test() {
    page * _process = _page_allocate_process(_PAGE_SIZE_4K, 12);

    char * _test[] = {
        (char *) _KEYWORD_ACCESS, ".", "hello test",
        (char *) _KEYWORD_SEPARATOR, "_", " "};

    atom * _base = (atom *) &_process->_uniform_base;
    _atom_set_info(_base, _TYPE_INFO_HALF);
    _atom_set_reference(_base, 0);
    _atom_set_type(_base, _TYPE_BASE);
    _process->_uniform_bulitins = _page_allocate_thread(_process);
    _process->_uniform_keywords = _keyword_initial(
        _process->_uniform_bulitins, 0, 0, 0, 0, _test, 6);

    atom * _cursor = _code_line(_process);
    atom * _keep = _cursor;
    atom * _assist = _cache_allocate(_process, sizeof(size_t));
    atom * _words = _cache_allocate(_process, sizeof(size_t));
    atom * _string = _cache_allocate(_process, sizeof(size_t));
    size_t _level = 0, _max = 0, _piece = 0;

    atom * _texts = _cache_allocate(_process, sizeof(size_t));
    atom * _unicodes = _cache_allocate(_process, sizeof(size_t));
    atom * _bytes = _cache_allocate(_process, sizeof(size_t));
    atom * _analyze = _cache_allocate(_process, sizeof(size_t));
    atom * _temp = _cache_allocate(_process, sizeof(size_t));

    char _test1[] =
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "meow hello test woof // "
        "####(#www.meep.com)hello test\\n\nABC\t"
        "Gull  (say <)@(hellohello\\worldworld).once"
        "/\n\n       (`// hello `` ``` h `` `` test```` ` ``(``` 你好"
    ;
    _CODE_TEST_LOOP(_test1);

        // _cache_push_multi(_process, _bytes, 1, strlen(_test), _test);
        // _code_analyze(
        //     _process, _texts, _unicodes, _bytes, 0, 0, 0, 0, 0,
        //     _process->_uniform_keywords, _analyze, _temp, &_cursor,
        //     _assist, _words, &_level, &_max, &_piece);

    char _test2[] = " ```))) meow ";
    _CODE_TEST_LOOP(_test2);

    // char _test3[] = "woof```hello``````good ``` `hello test`";
    // _CODE_TEST_LOOP(_test3);
    //
    // char _test4[] = "hello ` //```test meow````````` one ``` ```.```` ```";
    // _CODE_TEST_LOOP(_test4);
    //
    char _test5[] =
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
        "1234567890abcdefghijklmnopqrstuvwxyz () (-(wqw)-)"
    ;
    _CODE_TEST_LOOP(_test5);
    //
    // printf("level:%zu max:%zu piece:%zu\n", _level, _max, _piece);
}

#endif
