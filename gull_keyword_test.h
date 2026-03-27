#ifndef _GULL_KEYWORD_TEST_H
#define _GULL_KEYWORD_TEST_H 1

#include <stdio.h>
#include "gull_keyword.h"

void gull_keyword_test() {
    page * _process = _page_allocate_process(_PAGE_SIZE_4K, 12);

    char * _test[] = {(char *) _KEYWORD_ACCESS, "hello test", "hello"};

    _process->_uniform_bulitins = _page_allocate_thread(_process);
    _process->_uniform_keywords = _keyword_initial(
        _process->_uniform_bulitins, 0, 0, 0, 0, _test, 3);

    atom * _keyword = _process->_uniform_keywords;
    for (;;) {
        printf("%zu %s\n", _atom_get_atom_type(_keyword),
            _atom_get_extra_address(_keyword));
        _keyword = _atom_get_keyword_next(_keyword);
        if (_keyword == 0) break;
    }
}

#endif
