//
// Created by abc on 19-8-29.
//
#include "DiskAllocator.h"

DiskAllocator disk_alloc = DiskAllocator();

char* DiskAllocator::allocator(uint32_t) {
    return nullptr;
}

void DiskAllocator::deallocator(char *, uint32_t ) {
    //暂不实现
}

template <typename... Args> void DiskAllocator::construct(char *, Args&&...) {
    //暂不实现
}

void DiskAllocator::destroy(char *) {
    //暂不实现
}

