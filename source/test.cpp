#include <stdio.h>

// Test constructors
void __attribute__((constructor)) construct(void) {
    fprintf(stderr, "In constructor.\n");
}

extern "C" void message() {
    fprintf(stderr, "Hello world!\n");
    // Test exception handling
    try {
        throw 42;
    } catch (int e) {
        fprintf(stderr, "In exception handler.\n");
    }
}

// Test destructors
void __attribute__((destructor)) destruct(void) {
    fprintf(stderr, "In destructor.\n");
}