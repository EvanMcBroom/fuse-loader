// Copyright (c) 2023 Evan McBroom
// 
// This file is part of fuse-loader.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define FUSE_USE_VERSION 31
#include "../include/fuse-loader.h"
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>

int globalAllowedPid;
char* globalFileContent;
long globalFileSize;

// Clear the file's content when the filesystem is destroyed
static void fs_destroy(void *privateData) {
    (void)privateData;
    memset(globalFileContent, '\0', globalFileSize);
}

static int fs_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi) {
    (void)fi;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        // Allow all uses to list the root of the mount
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2; // 2 hardlinks are specified for '/' and '.'
    } else {
        // Restrict file access to the current user and group
        stbuf->st_mode = S_IFREG | 0400;
        stbuf->st_nlink = 1;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_size = globalFileSize; // All mount paths are the same globally set file
        stbuf->st_blocks = 0;
        stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL);
    }
    return 0;
}

static void* fs_init(struct fuse_conn_info* conn, struct fuse_config* cfg) {
    (void)conn;
    cfg->kernel_cache = 1;
    return NULL;
}

// Further restrict file access to the current process and its parent
static int fs_open(const char* path, struct fuse_file_info* fi) {
    (void)path;
    (void)fi;
    // Only allow the pid of the original process to access the file
    struct fuse_context* context = fuse_get_context();
    if (context->pid != globalAllowedPid)
        return -ENOENT;
    return 0;
}

// Satisfy all read requests using the same globally set file
static int fs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    (void)path;
    (void)fi;
    if (offset < globalFileSize) {
        if (offset + size > (unsigned long)globalFileSize) {
            size = globalFileSize - offset;
        }
        memcpy(buf, globalFileContent + offset, size);
    } else {
        size = 0;
    }
    return size;
}

// Make the fuse mount appear empty
static int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    (void)offset;
    (void)fi;
    (void)flags;
    if (strcmp(path, "/") != 0)
        return -ENOENT;
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    return 0;
}

static const struct fuse_operations operations = {
    .destroy = fs_destroy,
    .getattr = fs_getattr,
    .init = fs_init,
    .open = fs_open,
    .read = fs_read,
    .readdir = fs_readdir,
};

fl_fuse* fl_create_fuse(char* mountPath, char* fileContent, long fileSize) {
    int pid = fork();
    if (pid == 0) {
        // Libfuse may create a child process to manage the fuse mount
        // We create a process group to be able to easily kill the process tree at a later point
        if (setsid() != -1) {
            globalAllowedPid = getppid();
            struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
            fuse_opt_add_arg(&args, mountPath); // Used by fuse as the name of the application
            fuse_opt_add_arg(&args, mountPath); // Used by fuse as the mount path
            globalFileContent = fileContent;
            globalFileSize = fileSize;
            fuse_main(args.argc, args.argv, &operations, NULL);
            fuse_opt_free_args(&args);
        }
        return NULL;
    } else {
        fl_fuse* fuse = malloc(sizeof(fl_fuse));
        fuse->path = strdup(mountPath);
        fuse->pid = pid;
        return fuse;
    }
}

void fl_destroy_fuse(fl_fuse* fuse) {
    // Kill the entire child process tree
    killpg(fuse->pid, SIGKILL);
    // The child process running the fuse mount may not have terminated yet
    // If that happens remove will error as EBUSY and we will need to briefly wait before trying again
    while (umount2(fuse->path, MNT_DETACH) == -1 && errno == EBUSY) {
        usleep(100);
    }
    free(fuse->path);
    free(fuse);
}

void* fl_dlopen(char* fileContent, long fileSize) {
    void* library = NULL;
    char mountPath[] = "/mnt/XXXXXX";
    if (mkdtemp(mountPath) == mountPath) {
        library = fl_dlopen3(fileContent, fileSize, mountPath);
        remove(mountPath);
    }
    return library;
}

void* fl_dlopen3(char* fileContent, long fileSize, char* mountPath) {
    return fl_dlopen4(fileContent, fileSize, mountPath, "lib.so");
}

void* fl_dlopen4(char* fileContent, long fileSize, char* mountPath, char* fileName) {
    void* library = NULL;
    // Create the fuse mount which will provide access to the file
    fl_fuse* fuse = fl_create_fuse(mountPath, fileContent, fileSize);
    if (fuse) {
        // Load the library from the fuse mount
        // The example fuse was designed such that all file names are treated the same
        // Any path you use will provide access to the same in-memory file content
        size_t fusePathSize = strlen(mountPath) + strlen(fileName) + 2;
        char* fusePath = malloc(fusePathSize);
        sprintf(fusePath, "%s/%s", mountPath, fileName);
        fusePath[fusePathSize] = '\0';
        // The init routine for the fuse mount may not have executed yet
        // If that happens dlopen will fail and we will need to briefly wait before trying again
        while (!(library = dlopen(fusePath, RTLD_LAZY))) {
            usleep(100);
        }
        // The fuse mount may now be removed
        // The mount path may also be removed but that is the responsibility of the caller
        free(fusePath);
        fl_destroy_fuse(fuse);
    }
    return library;
}