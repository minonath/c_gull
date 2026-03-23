#ifndef _GULL_ATOM_TEST_H
#define _GULL_ATOM_TEST_H 1

#include <stdio.h>
#include <string.h>
#include "gull_atom.h"

#define _ATOM_MEMBER_TEST(pr, mb, vl) \
    printf(#mb); printf("\t"); printf(#pr, _atom_get_##mb(&_test)); \
    printf(" -> "); _atom_set_##mb(&_test, vl); \
    printf(#pr, _atom_get_##mb(&_test)); printf("\n");

void gull_atom_test() {
    atom _test = {0};

    _ATOM_MEMBER_TEST(%zu, atom_info, 5);
    _ATOM_MEMBER_TEST(%zu, atom_reference, 12345);
    _ATOM_MEMBER_TEST(%zu, atom_type, 0x11111); // 69905

    _ATOM_MEMBER_TEST(%p, recycle_previous, (atom *) 0x22222);
    _ATOM_MEMBER_TEST(%p, recycle_next, (atom *) 0x33333);

    _ATOM_MEMBER_TEST(%p, code_first, (atom *) 0x44444);
    _ATOM_MEMBER_TEST(%p, code_previous, (atom *) 0x55555);
    _ATOM_MEMBER_TEST(%p, code_current, (atom *) 0x66666);
    _ATOM_MEMBER_TEST(%p, code_next, (atom *) 0x77777);

    _ATOM_MEMBER_TEST(%zu, container_scanned, 0);
    _ATOM_MEMBER_TEST(%zu, container_cycle, 0x88888); // 559240
    _ATOM_MEMBER_TEST(%zu, container_scanned, 1);

    _ATOM_MEMBER_TEST(%p, tree_base, (atom *) 0x99999);
    _ATOM_MEMBER_TEST(%p, tree_root, (atom *) 0xaaaaa);
    _ATOM_MEMBER_TEST(%p, tree_pair, (atom *) 0xbbbbb);

    _ATOM_MEMBER_TEST(%p, base_value, (atom *) 0xccccc);
    _ATOM_MEMBER_TEST(%p, base_next, (atom *) 0xddddd);

    // 注意这里 atom 的最后 3/4 位被清除，这反而是正常的，结果应为 0xeeee8
    _ATOM_MEMBER_TEST(%p, node_parent, (atom *) 0xeeeee);
    _ATOM_MEMBER_TEST(%p, node_child, (atom *) 0xfffff);
    _ATOM_MEMBER_TEST(%zu, node_key, 0x1010101); // 16843009
    _ATOM_MEMBER_TEST(%zu, node_color, 1);
    _ATOM_MEMBER_TEST(%p, node_value, (atom *) 0x2020202); // 结果应为 0x2020200
    _ATOM_MEMBER_TEST(%zu, node_color, 0);
    _ATOM_MEMBER_TEST(%p, node_left, (atom *) 0x3030303);
    _ATOM_MEMBER_TEST(%p, node_right, (atom *) 0x4040404);

    _ATOM_MEMBER_TEST(%p, pair_previous, (atom *) 0x5050505);
    _ATOM_MEMBER_TEST(%p, pair_next, (atom *) 0x6060606);
    _ATOM_MEMBER_TEST(%p, pair_key, (atom *) 0x7070707);
    _ATOM_MEMBER_TEST(%p, pair_value, (atom *) 0x8080808);

    _ATOM_MEMBER_TEST(%zu, extra_used, 111);
    _ATOM_MEMBER_TEST(%p, extra_address, (atom *) 0x9090909);
    _ATOM_MEMBER_TEST(%zu, extra_size, 222); // 0xde

    _ATOM_MEMBER_TEST(%zu, text_protect, 0x10101010); // 269488144
    _ATOM_MEMBER_TEST(%p, keyword_next, (atom *) 0x11111111);

    _ATOM_MEMBER_TEST(%p, state_code, (atom *) 0x12121212);
    _ATOM_MEMBER_TEST(%p, state_cursor, (atom *) 0x13131313);
    _ATOM_MEMBER_TEST(%p, state_scope, (atom *) 0x14141414);
    _ATOM_MEMBER_TEST(%zu, state_end, 0);
    _ATOM_MEMBER_TEST(%zu, state_branch, 1);
    _ATOM_MEMBER_TEST(%zu, state_index, 2);
    _ATOM_MEMBER_TEST(%p, state_outer, (atom *) 0x15151515);
    _ATOM_MEMBER_TEST(%p, state_inner, (atom *) 0x16161616);


    _ATOM_MEMBER_TEST(%i, int8, -17);
    _ATOM_MEMBER_TEST(%u, uint8, 18);
    _ATOM_MEMBER_TEST(%i, int16, -19);
    _ATOM_MEMBER_TEST(%u, uint16, 20);
    _ATOM_MEMBER_TEST(%i, int32, -21);
    _ATOM_MEMBER_TEST(%u, uint32, 22);
    _ATOM_MEMBER_TEST(%lli, int64, -23);
    _ATOM_MEMBER_TEST(%llu, uint64, 24);

    _ATOM_MEMBER_TEST(%f, float, -25.26);
    _ATOM_MEMBER_TEST(%lf, double, 27.28);

    _ATOM_MEMBER_TEST(%zu, big_first, 29);
    _ATOM_MEMBER_TEST(%zu, big_second, 30);

    _atom_set_big_type_offset(&_test, 12, 34567);
    printf("big_type & offset\t%zu & %zu\n",
        _atom_get_big_type(&_test), _atom_get_big_offset(&_test));

    _ATOM_MEMBER_TEST(%llu, clock_second, 31313131);
    _ATOM_MEMBER_TEST(%llu, clock_nano, 32323232);

    char * _brief = _atom_get_brief_address(&_test);
    printf("brief old\t%s\n", _brief);
    _ATOM_MEMBER_TEST(%u, brief_protect, -1); // 255
    strcpy(_brief, "hello meow");
    printf("brief new\t%s\n", _brief);
}

#undef _ATOM_MEMBER_TEST

#endif
