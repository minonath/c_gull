#ifndef _GULL_TEXT_TEST_H
#define _GULL_TEXT_TEST_H 1

#include "gull_text.h"

static atom * _text_output(page * _thread, atom * _atom) {
    return 0;
}

void gull_text_test() {
    page * _process = _page_allocate_process(_PAGE_SIZE_4K, 12);
    atom * _t1 = _text_build(_process, 15, 0, "hello meow 1234567890");
    printf("%s\n", _atom_get_brief_address(_t1));
    atom * _t2 = _text_build(
        _process, 20, 0, "hruwfworfbwregihwurbqlweuhrqweb--fsdfg2i");
    printf("%s\n", _atom_get_extra_address(_t2));
    atom * _t3 = _text_clone(_process, _t2);
    printf("%s\n", _atom_get_extra_address(_t3));

    atom * _texts = _cache_allocate(_process, sizeof(size_t));
    atom * _unicodes = _cache_allocate(_process, sizeof(size_t));
    atom * _bytes = _cache_allocate(_process, sizeof(size_t));

    char _test[] = "Test Escaped: \\x09\t\\t \\x55 好\\u597D";

    _cache_push_multi(_process, _bytes, 1, strlen(_test), _test);

    for (;;) {
        int _a = _text_read_char(
            _process, _texts, _unicodes, _bytes, 0, 0, 0, 0, 0);
        if (_a == -1) break;
        if (_a > 127) {
            printf("(%d)", _a);
        } else {
            printf("%c", (char) _a);
        }
    }
    printf("\n");

    FILE * _file = fopen("gull_text_test.h", "r");

    for (;;) {
        int _a = _text_read_char(
            _process, _texts, _unicodes, 0, _file, 0, 0, 0, 0);
        if (_a == -1) break;
        if (_a > 127) {
            printf("(%d)", _a);
        } else {
            printf("%c", (char) _a);
        }
    }
    printf("\n");
    fclose(_file);
}
#endif
