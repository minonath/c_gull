#ifndef _GULL_PAGE_TEST_H
#define _GULL_PAGE_TEST_H 1

#include <stdio.h>
#include <stdlib.h>
#include "gull_page.h"

static void _page_print(page * _page) {
    printf("address : %p\n", _page);
    printf("\tpage  page_info & page_reference: %u & %zu\n",
        _page->_page_info, _page->_page_reference);
    printf("\tpage  page_type : %zu\n", _page->_page_type);
    printf("\tpage  page_previous : %p\n", _page->_page_previous);
    printf("\tpage  page_next : %p\n", _page->_page_next);
    printf("\tpage  page_main : %p\n", _page->_page_main);
    printf("\tpage  page_used : %zu\n", _page->_page_used);
    printf("\tpage  page_remain : %zu\n", _page->_page_remain);
    printf("\tpage  page_size : %zu\n", _page->_page_size);

    printf("\tthread page_single : %p\n", _page->_page_single);
    printf("\tthread page_single_size : %zu\n", _page->_page_single_size);
    printf("\tthread page_double : %p\n", _page->_page_double);
    printf("\tthread page_double_size : %zu\n", _page->_page_double_size);

    printf("\tthread page_atom : %p\n", _page->_page_atom);
    printf("\tthread page_atom_size : %zu\n", _page->_page_atom_size);
    printf("\tthread page_extra : %p\n", _page->_page_extra);
    printf("\tthread page_extra_size : %zu\n", _page->_page_extra_size);
    printf("\tthread page_extra_merge : %p\n", _page->_page_extra_merge);
    printf("\tthread page_recycle : %p\n", _page->_page_recycle);
    printf("\tthread page_recycle_size : %zu\n", _page->_page_recycle_size);
    printf("\tthread page_recycle_limit : %zu\n", _page->_page_recycle_limit);
}

static void _shuffle(atom ** _list, int _size) {
    srand(time(0));
    for (int _i = 0; _i < _size; _i ++) {
        int _r = rand() % _size;
        atom * _keep = _list[_i];
        _list[_i] = _list[_r];
        _list[_r] = _keep;
    }
}

void gull_page_test() {
    printf("%lu\n", _PAGE_EXTEND_HEAD_SIZE);
    printf("%lu -> ", offsetof(page, _signature_generator));
    printf("%lu\n", _PAGE_THREAD_HEAD_SIZE);
    printf("%lu -> ", sizeof(page));
    printf("%lu\n", _PAGE_PROCESS_HEAD_SIZE);

    page * _process = _page_allocate_process(_PAGE_SIZE_4K, 12);
    printf("page: %p used: %zu\n", _process, _process->_page_used);

    srand(time(0));

    #define _C 1000
    atom * _atom_double[_C] = {0};
    atom * _atom_single[_C] = {0};
    atom * _atom_bind[_C] = {0};
    for (int _i = 0; _i < _C; _i ++) {
        _atom_double[_i] = _page_allocate_double(_process);
        _atom_set_atom_info(_atom_double[_i], _TYPE_INFO_FULL);
        _atom_set_atom_type(_atom_double[_i], _TYPE_MAP);
        _atom_single[_i] = _page_allocate_single(_process);
        _atom_set_atom_info(_atom_single[_i], _TYPE_INFO_HALF);
        _atom_set_atom_type(_atom_single[_i], _TYPE_LONG);
        _atom_bind[_i] = _page_allocate_bind(
            _process, _TYPE_TEXT, rand() % 210);
    }
    _shuffle(_atom_double, _C);
    _shuffle(_atom_single, _C);
    _shuffle(_atom_bind, _C);

    for (int _i = _C / 3; _i < _C; _i ++) {
        _page_release_double(_process, _atom_double[_i]);
        _page_release_single(_process, _atom_single[_i]);
        _page_release_extra(_process, _atom_bind[_i]);
        _page_release_double(_process, _atom_bind[_i]);
    }

    for (int _i = _C / 3; _i < _C; _i ++) {
        _atom_double[_i] = _page_allocate_double(_process);
        _atom_set_atom_info(_atom_double[_i], _TYPE_INFO_FULL);
        _atom_set_atom_type(_atom_double[_i], _TYPE_MAP);
        _atom_single[_i] = _page_allocate_single(_process);
        _atom_set_atom_info(_atom_single[_i], _TYPE_INFO_HALF);
        _atom_set_atom_type(_atom_single[_i], _TYPE_LONG);
        _atom_bind[_i] = _page_allocate_bind(
            _process, _TYPE_TEXT, rand() % 210);
    }

    printf("page: %p used: %zu\n", _process, _process->_page_used);
    for (int _i = 0; _i < _C; _i ++) {
        if (!_atom_is_legal(_process, _atom_double[_i]) ||
                !_atom_is_legal(_process, _atom_single[_i])) {
            printf("访问错误\n");
            break;
        }
    }

    _page_print(_process);

    for (int _i = 0; _i < _C; _i ++) {
        _page_release_double(_process, _atom_double[_i]);
        _page_release_single(_process, _atom_single[_i]);
        _page_release_extra(_process, _atom_bind[_i]);
        _page_release_double(_process, _atom_bind[_i]);
    }

    _page_print(_process);

}

#endif
