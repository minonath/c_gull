#ifndef _GULL_LIST_TEST_H
#define _GULL_LIST_TEST_H 1

#include "gull_list.h"

#include <stdlib.h>

int _index_of(atom ** _list, atom * _one, int _limit) {
    for (int _i = 0; _i < _limit; _i ++) {
        if (_list[_i] == _one) return _i;
    }
    return -1;
}

void gull_list_test() {
    page * _process = _page_allocate_process(_PAGE_SIZE_64K, 12);

    _process->_container_traversals[0] = _list_traversal;
    _process->_container_counts[0] = _list_count;
    _process->_container_destroys[0] = _list_destroy;
    printf("%ld\n", _process->_page_used);

    atom * _test[30] = {0};
    for (int _i = 0; _i < 30; _i ++) {
        _test[_i] = _list_allocate(_process, 0);
    }
    _atom_set_reference(_test[0], 1);

    /*
    0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 29 →→ 0
                    ↓
                    8
                    ↓
                    9   27 → 28 →→ 26
    2               ↓    ↑
    ↑              10   26    19 →→ 6
    ↑               ↓    ↑    ↑
    22 ← 21 ← 20 ← 11 → 25    18 →→ 24 →→ 21
                    ↓         ↑
                    12        17
                    ↓         ↑
         21 ←← 23 ← 13 → 14 → 15 → 16 →→ 9
    */

    _list_append(_process, _test[0], _test[1]);
    _list_append(_process, _test[1], _test[2]);
    _list_append(_process, _test[2], _test[3]);
    _list_append(_process, _test[3], _test[4]);
    _list_append(_process, _test[4], _test[5]);
    _list_append(_process, _test[5], _test[6]);
    _list_append(_process, _test[6], _test[7]);

    _list_append(_process, _test[4], _test[8]);
    _list_append(_process, _test[8], _test[9]);
    _list_append(_process, _test[9], _test[10]);
    _list_append(_process, _test[10], _test[11]);
    _list_append(_process, _test[11], _test[12]);
    _list_append(_process, _test[12], _test[13]);
    _list_append(_process, _test[13], _test[14]);
    _list_append(_process, _test[14], _test[15]);
    _list_append(_process, _test[15], _test[16]);
    _list_append(_process, _test[16], _test[9]);

    _list_append(_process, _test[15], _test[17]);
    _list_append(_process, _test[17], _test[18]);
    _list_append(_process, _test[18], _test[19]);
    _list_append(_process, _test[19], _test[6]);

    _list_append(_process, _test[11], _test[20]);
    _list_append(_process, _test[20], _test[21]);
    _list_append(_process, _test[21], _test[22]);
    _list_append(_process, _test[22], _test[2]);

    _list_append(_process, _test[13], _test[23]);
    _list_append(_process, _test[23], _test[21]);

    _list_append(_process, _test[18], _test[24]);
    _list_append(_process, _test[24], _test[21]);

    _list_append(_process, _test[11], _test[25]);
    _list_append(_process, _test[25], _test[26]);
    _list_append(_process, _test[26], _test[27]);
    _list_append(_process, _test[27], _test[28]);
    _list_append(_process, _test[28], _test[26]);

    _list_append(_process, _test[7], _test[29]);
    _list_append(_process, _test[29], _test[0]);


    for (int _i = 0; _i < 30; _i ++) {
        printf("(%ld %ld %ld %ld %ld) ", _atom_get_reference(_test[_i]),
            _atom_get_container_scan(_test[_i]),
            _atom_get_container_loop(_test[_i]),
            _atom_get_container_cache(_test[_i]),
            _atom_get_container_count(_test[_i]));
    }
    printf("%ld\n", _process->_page_used);

    _reference_decrease(_process, _test[0]);

    for (int _i = 0; _i < 30; _i ++) {
        printf("(%ld %ld %ld %ld %ld) ", _atom_get_reference(_test[_i]),
            _atom_get_container_scan(_test[_i]),
            _atom_get_container_loop(_test[_i]),
            _atom_get_container_cache(_test[_i]),
            _atom_get_container_count(_test[_i]));
    }

    #define MEW 3
    #define TEST (8000 * MEW)
    atom * _cache[TEST] = {0};
    atom * _root = _list_allocate(_process, 0);

    printf("\n+++++++++++++++++%ld\n", _process->_page_used);
    for (int _i = 0; _i < TEST; _i ++) {
        _cache[_i] = _list_allocate(_process, 0);
        // printf("start %ld %p ", _i, _cache[_i]);
        _list_append(_process, _root, _cache[_i]);
    }

    struct timespec _aaaa, _bbbb;
    clock_gettime(CLOCK_REALTIME, &_aaaa);

    srand(time(0));
    for (int _i = 0; _i < TEST; _i += MEW) {
        printf("%d\n", _i);
        for(int _j = 0; _j <= (rand() % (3 * MEW)); _j ++) {
            _list_append(_process, _cache[_i], _cache[rand() % TEST]);
        }
        /*
        for (int _i = 0; _i < TEST; _i ++) {
            printf("(index:%d %ld s:%ld l:%ld c:%ld %ld) ", _i,
                _atom_get_reference(_cache[_i]),
                _atom_get_container_scan(_cache[_i]),
                _atom_get_container_loop(_cache[_i]),
                _atom_get_container_cache(_cache[_i]),
                _atom_get_container_count(_cache[_i]));
        }
        printf("\n");

        for (int _i = 0; _i < TEST; _i ++) {
            printf("< %d : ", _i);
            size_t _size = _atom_get_extra_used(_cache[_i]) / sizeof(size_t);
            atom ** _list = _atom_get_extra_address(_cache[_i]);
            for (size_t _a = 0; _a < _size; _a ++) {
                printf("%d ", _index_of(_cache, _list[_a], TEST));
            }
            printf(">  ");
        }
        printf("\n");
        */
    }
    clock_gettime(CLOCK_REALTIME, &_bbbb);
    printf("time spend %ld %ld %ld %ld\n",
        _aaaa.tv_sec, _aaaa.tv_nsec, _bbbb.tv_sec, _bbbb.tv_nsec);
    // return;

    printf("######### %ld\n", _process->_page_used);
    size_t _keep = _process->_page_used;
    for (int _i = TEST - 1; _i >= 0; _i --) {
        printf("%d\n", _i);
        _list_remove(_process, _root, _i);
        /*
        for (int _i = 0; _i < TEST; _i ++) {
            printf("(index:%d %ld s:%ld l:%ld c:%ld %ld) ", _i,
                _atom_get_reference(_cache[_i]),
                _atom_get_container_scan(_cache[_i]),
                _atom_get_container_loop(_cache[_i]),
                _atom_get_container_cache(_cache[_i]),
                _atom_get_container_count(_cache[_i]));
        }
        printf("\n");
        for (int _i = 0; _i < TEST; _i ++) {
            if (_atom_get_reference(_cache[_i])) {
                printf("< %d : ", _i);
                size_t _size =
                    _atom_get_extra_used(_cache[_i]) / sizeof(size_t);
                atom ** _list = _atom_get_extra_address(_cache[_i]);
                for (size_t _a = 0; _a < _size; _a ++) {
                    int _p = _index_of(_cache, _list[_a], TEST);
                    if (_p == -1) {
                        printf("error %p %ld %ld", _list, _a, _list[_a]);
                        break;
                    } else {
                        printf("%d ", _p);
                    }
                }
                printf(">  ");
            }
        }

        printf("\n"); */
        // if ( _keep != _process->_page_used) break;
    }
    clock_gettime(CLOCK_REALTIME, &_bbbb);
    printf("time spend %ld %ld %ld %ld\n",
        _aaaa.tv_sec, _aaaa.tv_nsec, _bbbb.tv_sec, _bbbb.tv_nsec);
    printf("%ld\n", _process->_page_used);
}

#endif
