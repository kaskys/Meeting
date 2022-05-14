//
// Created by abc on 19-8-29.
//
#include "MemoryAllocator.h"

std::allocator<char> MemoryAllocator::alloc_ = std::allocator<char>();
MemoryAllocator memory_alloc = MemoryAllocator();

//char* MemoryAllocator::allocator(uint32_t as) {
//    if(as > 0){
//        return alloc_.allocate(as);
//    }else{
//        throw std::bad_alloc();
//    }
//}
//
//void MemoryAllocator::deallocator(char *dp, uint32_t ds) noexcept{
//    alloc_.deallocate(dp, ds);
//}

