#ifndef _GULL_LIST_H
#define _GULL_LIST_H 1

#include "gull_cache.h"
#include "gull_reference.h"

#define _LIST_DEFAULT 20 * sizeof(size_t)

static inline atom * _list_allocate(page * _thread, size_t _size) {
    atom * _cache = _page_allocate_bind(_thread, _TYPE_LIST, _size);
    _atom_set_extra_used(_cache, 0);
    _atom_set_s4(_cache, 0);
    return _cache;
}

static inline atom * _list_append(
        page * _thread, atom * _list, atom * _value) {
    // printf(">>> %p\n", _value);
    _cache_push(_thread, _list, atom *, _value);
    _reference_increase(_thread, _list, _value);
    return _value;
}

// static inline atom * _list_pop(page * _thread, atom * _cache) {
//     atom * _r; _cache_pop(_cache, atom *, _r);
//     _atom_reference_decrease(_r);
//     return _r;
// }
//
// static inline atom * _list_insert(
//         page * _thread, atom * _list, atom * _value, size_t _index) {
//     size_t _list_push_old = _atom_get_extra_used(_list);
//     size_t _list_push_new = _list_push_old + sizeof(atom *);
//     _cache_expand(thread, _list, _list_push_new);
//     atom ** _address = (atom **) _atom_get_extra_address(_list);
//     size_t _cur = _list_push_old / sizeof(atom *);
//     if (_cur > _index) {
//         do {
//             size_t _sub = _cur - 1;
//             _address[_cur] = _address[_sub];
//             _cur = _sub;
//         } while (_cur > _index);
//         _address[_index] = _value;
//     } else {
//         _address[_cur] = _value;
//     }
//     _atom_set_extra_used(_list, _lsit_push_new);
// }

static void _list_remove(page * _thread, atom * _list, size_t _index) {
    size_t _size = _atom_get_extra_used(_list) / sizeof(atom *) - 1;
    if (_index > _size) return;

    atom ** _address = _atom_get_extra_address(_list);
    atom * _cache = _address[_index];
    for (size_t _i = _index; _i < _size;) {
        size_t _j = _i + 1;
        _address[_i] = _address[_j];
        _i = _j;
    }
    _atom_set_extra_used(_list, _size * sizeof(size_t));
    _reference_decrease(_thread, _list, _cache);
}

static size_t _list_traversal(
        page * _thread, atom* _scan, atom * _container, atom * _target) {
    size_t _result = 0;
    size_t _size = _atom_get_extra_used(_container) / sizeof(atom *);

    if (_size == 0) return 0;

    atom ** _list = _atom_get_extra_address_block(_thread, _container);
    for (size_t _i = 0; _i < _size; _i ++) {
        _result |= _reference_traversal(_thread, _scan, _list[_i], _target);
    }

    _atom_set_extra_address_unblock(_thread, _container);
    if (_result) return _atom_set_container_loop(_container, 1);
    return 0;
}

static void _list_record(page * _thread, atom* _loop, atom * _container) {
    _cache_push(_thread, _atom_get_a6(_loop), atom *, _container);
    size_t _size = _atom_get_extra_used(_container) / sizeof(atom *);
    if (_size == 0) return;
    atom ** _list = _atom_get_extra_address(_container);
    for (size_t _i = 0; _i < _size; _i ++) {
        _tree_set_insert_container(_thread, _loop, _list[_i], 1);
    }
}

static void _list_release(page * _thread, atom * _atom, size_t _special) {
    if (_special == 0) {
        size_t _size = _atom_get_extra_used(_atom) / sizeof(atom *);
        atom ** _list = _atom_get_extra_address_block(_thread, _atom);
        for (size_t _i = 0; _i < _size; _i ++) {
            _reference_decrease(_thread, _atom, _list[_i]);
        }
        _atom_set_extra_address_unblock(_thread, _atom);
    }

    // _atom_set_reference(_atom, 0);
    // _atom_set_extra_used(_atom, 0);
    _cache_release_direct(_thread, _atom);
}

#endif
