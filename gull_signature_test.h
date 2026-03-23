#ifndef _GULL_SIGNATURE_TEST_H
#define _GULL_SIGNATURE_TEST_H 1

#include <stdio.h>
#include "gull_signature.h"

void gull_signature_test() {
    struct timespec _request = {0, 1}, _remain = {0};
    nanosleep(&_request, &_remain);

    signature _signature = {0};
    char _string[_SIGNATURE_SIZE];

    _signature_string(&_signature, _string);
    printf("second %ld\n", _signature._signature_second);
    printf("nanotime %ld\n", _signature._signature_nano);
    printf("count %zu\n", _signature._signature_count);
    printf("%s\n\n", _string);

    _signature_string(_singature_initial(&_signature), _string);
    printf("second %ld\n", _signature._signature_second);
    printf("nanotime %ld\n", _signature._signature_nano);
    printf("count %zu\n", _signature._signature_count);
    printf("%s\n\n", _string);

    _signature_string(_singature_generate(&_signature), _string);
    printf("second %ld\n", _signature._signature_second);
    printf("nanotime %ld\n", _signature._signature_nano);
    printf("count %zu\n", _signature._signature_count);
    printf("%s\n\n", _string);

    _signature._signature_count = -1;

    _signature_string(_singature_generate(&_signature), _string);
    printf("second %ld\n", _signature._signature_second);
    printf("nanotime %ld\n", _signature._signature_nano);
    printf("count %zu\n", _signature._signature_count);
    printf("%s\n\n", _string);
}



#endif
