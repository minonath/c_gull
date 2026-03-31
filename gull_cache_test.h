#ifndef _GULL_CACHE_TEST_H
#define _GULL_CACHE_TEST_H 1

#include <stdio.h>
#include "gull_cache.h"

void gull_cache_test() {
    page * _process = _page_allocate_process(_PAGE_SIZE_4K, 12);
    atom * _cache = _cache_with(_process, 1, 10, "hello meow");

    _cache_push(_process, _cache, unsigned char, '~');
    _cache_push(_process, _cache, unsigned, 123456);

    printf("%u\n", _cache_pop_uint32(_cache));
    for (int _i = 0; _i < 11; _i ++) {
        printf("%c", _cache_pop_uint8(_cache));
    }
    printf("\n");

    _cache_push_multi(_process, _cache, 0, 11, "ASDFGHJKL;'");
    for (int _i = 0; _i < 11; _i ++) {
        printf("%c", _cache_pop_uint8(_cache));
    }
    printf("\n");

    _cache_push_multi2(_process, _cache, 1, 11,
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K');
    for (int _i = 0; _i < 11; _i ++) {
        printf("%c", _cache_pop_uint8(_cache));
    }
    printf("\n");

    char _text[] = "hello world \\x4F dlrow olleh";
    _cache_push_escaped_2_unicode(_process, _cache, _text);
    for (int _i = _atom_get_extra_used(_cache); _i > 0; _i --) {
        printf("%c", _cache_pop_uint8(_cache));
    }
    printf("%s\n", _atom_get_extra_address(_cache));

    for (int _i = 0; _i < 1000; _i ++) {
        _cache_push_uint8(_process, _cache, (_i % 10) + '0');
    }
    _cache_push_uint8(_process, _cache, 0);
    printf("%s\n", _atom_get_extra_address(_cache));
}

#endif
