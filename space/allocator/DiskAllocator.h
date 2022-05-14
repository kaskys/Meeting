//
// Created by abc on 19-8-29.
//

#ifndef UNTITLED8_DISKALLOCATOR_H
#define UNTITLED8_DISKALLOCATOR_H

#include <cstdio>
#include <memory>

class DiskAllocator{
public:
    DiskAllocator() = default;
    ~DiskAllocator() = default;

    char* allocator(uint32_t);
    void deallocator(char*, uint32_t);

    template <typename... Args> void construct(char*, Args&&...);
    void destroy(char*);
};

extern DiskAllocator disk_alloc;

#endif //UNTITLED8_DISKALLOCATOR_H
