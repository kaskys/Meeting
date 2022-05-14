//
// Created by abc on 19-9-6.
//
#include "MemoryReader.h"

RIterator::RIterator(MemoryReader *r) throw(std::runtime_error) : reader(r)  {
    if(!reader){
        throw std::runtime_error("ReaderIterator::MemoryReader cannot be null~");
    }
}

RIterator::~RIterator() {
    reader->releaseReader();
}

/**
 * 剩下的内存长度
 * @return
 */
uint32_t RIterator::IteratorSize() {
    return reader->locatorLength();
}

/**
 * 读取内存
 * @return
 */
char* RIterator::IteratorBuffer() {
    return reader->locatorMemory<char>(false);
}

//--------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------MemoryReader---------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * 已经读取了size个字节（即读取点向前移动size个字节）
 * @param size
 */
void MemoryReader::alreadMemory(uint32_t size) {
    checkRead();
    frontLocator(size);
}

/**
 * 复位size个字节（即读取点向后移动size个字节）
 * @param size
 */
void MemoryReader::rereadMemory(uint32_t size) {
    checkRead();
    backLocator(size);
}

/**
 * 爆破该阅读器(回收该阅读器关联的内存)
 * @param reader
 */
void MemoryReader::detonateReader(MemoryReader *reader) {
    reader->blastLocator();
}


