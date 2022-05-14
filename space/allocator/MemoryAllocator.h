//
// Created by abc on 19-8-26.
//

#ifndef UNTITLED8_MEMORYALLOCATOR_H
#define UNTITLED8_MEMORYALLOCATOR_H

#include <cstdio>
#include <iostream>
#include <memory>

class MemoryAllocator{
public:
    MemoryAllocator() = default;
    ~MemoryAllocator() = default;

    template <typename T> T* allocator(uint32_t ts = 1) {
        return reinterpret_cast<T*>(alloc_.allocate(sizeof(T) * ts));
    }
    template <typename T> void deallocator(T *tp, uint32_t ds = 1) noexcept {
        alloc_.deallocate((char*)tp, sizeof(T) * ds);
    }

    template <typename T, typename... Args> void construct(T *cp, Args&&... args){ ::new((void *)cp) T(std::forward<Args>(args)...); }
    template <typename T> void destroy(T *dp) { dp->~T(); }
private:
    static std::allocator<char> alloc_;
};

extern MemoryAllocator memory_alloc;

#endif //UNTITLED8_MEMORYALLOCATOR_H
