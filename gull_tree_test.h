#ifndef _GULL_TREE_TSET_H
#define _GULL_TREE_TEST_H 1

#include "gull_tree.h"
#include <stdlib.h>

static void _gull_tree_print_iter(atom * _node, size_t _level) {
    if (_node) {
        _gull_tree_print_iter(_atom_get_node_left(_node), _level + 1);

        for (size_t _i = 0; _i < _level; _i ++) {
            printf(" ");
        }
        if (_atom_get_node_color(_node)) {
            printf("-");
        } else {
            printf("O");
        }
        printf(" %zu\n", _atom_get_node_key(_node));

        _gull_tree_print_iter(_atom_get_node_right(_node), _level + 1);
    }
}

void _gull_tree_print(page * _thread, atom * _tree) {
    printf("---------- %zu\n", _thread->_page_used);
    _gull_tree_print_iter(_atom_get_tree_root(_tree), 0);
}

void gull_tree_test() {
    page * _process = _page_allocate_process(_PAGE_SIZE_64K, 12);
    atom * _tree = _tree_allocate(_process, _TYPE_TREE);
    printf("---------- %zu\n", _process->_page_used);

    #define _TT 36
    size_t _list[_TT] = {0};
    for (size_t _i = 0; _i < _TT; _i ++) {
        _list[_i] = _i;
    }

    srand(time(0));
    for (size_t _i = 0; _i < _TT; _i ++) {
        size_t _k = _list[_i];
        size_t _s = rand() % _TT;
        _list[_i] = _list[_s];
        _list[_s] = _k;
    }

    for (size_t _i = 0; _i < _TT; _i ++) {
        _tree_insert(_process, _tree, _list[_i], 1);
    }
    _gull_tree_print(_process, _tree);

    for (size_t _i = 0; _i < _TT; _i ++) {
        size_t _k = _list[_i];
        size_t _s = rand() % _TT;
        _list[_i] = _list[_s];
        _list[_s] = _k;
    }

    for (size_t _i = 0; _i < _TT; _i ++) {
        atom * _node = _tree_insert(_process, _tree, _list[_i], 0);
        _tree_remove(_process, _tree, _node);
        _gull_tree_print(_process, _tree);
    }
}

#endif
