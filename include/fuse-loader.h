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

#pragma once

typedef struct fl_fuse_t {
    char* path;
    int pid;
} fl_fuse;

fl_fuse* fl_create_fuse(char* mountPath, char* fileContent, long fileSize);

void fl_destroy_fuse(fl_fuse* fuse);

void* fl_dlopen(char* fileContent, long fileSize);

void* fl_dlopen3(char* fileContent, long fileSize, char* mountPath);

void* fl_dlopen4(char* fileContent, long fileSize, char* mountPath, char* fileName);