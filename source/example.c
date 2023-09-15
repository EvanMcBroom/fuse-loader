#define _GNU_SOURCE
#include "../include/fuse-loader.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* read_file_content(char* path, long* fileSize) {
    // Get the size of the file
    FILE* file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    *fileSize = ftell(file);
    // Read the file content
    char* content = malloc(*fileSize + 1);
    fseek(file, 0, SEEK_SET);
    fread(content, *fileSize, 1, file);
    fclose(file);
    return content;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        // Use the library the user specified to demonstrate the loading approach
        // Other applications may provide this content directly from memory
        long fileSize;
        char* fileContent = read_file_content(argv[1], &fileSize);
        // Load the library using a fuse mount
        void* library = fl_dlopen(fileContent, fileSize);
        free(fileContent);
        if (library) {
            // Resolve a function from the dynamically loaded library and call it
            // Casting is done for the assignment statement because ISO C does not
            // allow a ‘void *’ to initialize a function pointer
            void (*message)();
            *((void**)&message) = dlsym(library, "message");
            (*message)();
            // Close the library once its no longer needed
            dlclose(library);
        } else {
			fprintf(stderr, "Could not load the specified library.");
        }
    } else {
        printf("%s <so path>", argv[0]);
    }
}