//
// Created by abc on 19-9-6.
//

#ifndef UNTITLED8_MEMORYREADER_H
#define UNTITLED8_MEMORYREADER_H

#include <memory>
#include "../control/SpaceControl.h"

class MemoryReader;

class RIterator{
public:
    explicit RIterator(MemoryReader*) throw(std::runtime_error);
    virtual ~RIterator();

    RIterator(const RIterator&) = default;
    RIterator(RIterator&&) = default;
    RIterator& operator=(const RIterator&) = default;
    RIterator& operator=(RIterator&&) = default;
protected:
    uint32_t IteratorSize();
    char* IteratorBuffer();
private:
    MemoryReader *reader;       //内存读取器
};

/**
 * 读取迭代器
 *  该迭代器获取读取点的内存并转换为T类型的指针,提供给用户操作T类型的指针
 * @tparam T
 */
template <typename T> class ReaderIterator : public RIterator{
    typedef T IteratorType;
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<IteratorType*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    IteratorType& operator*() { return *iterator_buffer; }
    IteratorType* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len-= sizeof(IteratorType); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    ReaderIterator& operator>>(IteratorType &type) {
        new (&type) IteratorType(*iterator_buffer); return *this;
    }

    ReaderIterator& operator<<(const IteratorType &type) {
        new (iterator_buffer) IteratorType(type); return *this;
    }
private:
    uint32_t iterator_len;             //内存长度
    IteratorType *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<char> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()), iterator_buffer(IteratorBuffer()) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    char& operator*() { return *iterator_buffer; }
    char* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    char *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<unsigned char> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<unsigned char*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    unsigned char& operator*() { return *iterator_buffer; }
    unsigned char* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    unsigned char *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<signed char> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<signed char*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    signed char& operator*() { return *iterator_buffer; }
    signed char* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    signed char *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<wchar_t> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<wchar_t*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    wchar_t& operator*() { return *iterator_buffer; }
    wchar_t* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    wchar_t *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<char16_t> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<char16_t*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    char16_t& operator*() { return *iterator_buffer; }
    char16_t* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    char16_t *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<char32_t> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<char32_t*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    char32_t& operator*() { return *iterator_buffer; }
    char32_t* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    char32_t *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<short> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<short*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    short& operator*() { return *iterator_buffer; }
    short* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    short *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<unsigned short> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<unsigned short*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    unsigned short& operator*() { return *iterator_buffer; }
    unsigned short* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    unsigned short *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<int> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<int*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    int& operator*() { return *iterator_buffer; }
    int* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    int *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<unsigned int> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<unsigned int*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    unsigned int& operator*() { return *iterator_buffer; }
    unsigned int* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    unsigned int *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<long> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<long*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    long& operator*() { return *iterator_buffer; }
    long* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    long *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<unsigned long> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<unsigned long*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    unsigned long& operator*() { return *iterator_buffer; }
    unsigned long* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    unsigned long *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<long long> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<long long*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    long long& operator*() { return *iterator_buffer; }
    long long* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    long long *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<unsigned long long> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<unsigned long long*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    unsigned long long& operator*() { return *iterator_buffer; }
    unsigned long long* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    unsigned long long *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<float> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<float*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    float& operator*() { return *iterator_buffer; }
    float* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    float *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<double> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<double*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    double& operator*() { return *iterator_buffer; }
    double* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    double *iterator_buffer;     //对应的内存
};

template <> class ReaderIterator<long double> : public RIterator{
public:
    explicit ReaderIterator(MemoryReader *reader) : RIterator(reader), iterator_len(IteratorSize()),
                                           iterator_buffer(reinterpret_cast<long double*>(IteratorBuffer())) {}
    explicit ReaderIterator(MemoryReader *reader, uint32_t) : RIterator(reader), iterator_len(0), iterator_buffer(nullptr) {}
    ~ReaderIterator() override = default;

    long double& operator*() { return *iterator_buffer; }
    long double* operator->() { return iterator_buffer; }

    ReaderIterator& operator++() {
        ++iterator_buffer; iterator_len -= sizeof(char); return *this;
    }
    ReaderIterator operator++(int) {
        ReaderIterator temp = *this;
        operator++();
        return temp;
    }

    template <typename OperatorType> ReaderIterator& operator>>(OperatorType &type) throw(std::runtime_error) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (&type) OperatorType(*reinterpret_cast<OperatorType*>(iterator_buffer)); return *this;
    }

    template <typename OperatorType> ReaderIterator& operator<<(const OperatorType &type) {
        if(sizeof(OperatorType) > iterator_len) { throw std::runtime_error("ReaderIterator::operator>>() Out Of Memory"); }
        new (iterator_buffer) OperatorType(type); return *this;
    }
private:
    uint32_t iterator_len;     //内存长度
    long double *iterator_buffer;     //对应的内存
};

/**
 * 内存阅读器
 */
class MemoryReader final : public MemoryLocator {
    friend class RIterator;
public:
    MemoryReader() : MemoryLocator(), bomb(nullptr, MemoryReader::detonateReader) {}
    explicit MemoryReader(const MemoryLocator &locator) : MemoryLocator(locator), bomb(this, MemoryReader::detonateReader) {}
    explicit MemoryReader(uint32_t buffer_len, char *reader_buffer, const std::function<void(char*, int)> &rfunc)
                               : MemoryLocator(buffer_len, reader_buffer, rfunc), bomb(this, MemoryReader::detonateReader) {}
    ~MemoryReader() override = default;

    MemoryReader(const MemoryReader &mr) noexcept : MemoryLocator(mr) { bomb = mr.bomb; }
    MemoryReader(MemoryReader &&mr) noexcept : MemoryLocator(mr) { bomb = std::move(mr.bomb); }

    MemoryReader& operator=(const MemoryReader &mr) noexcept {
        MemoryLocator::operator=(mr); bomb = mr.bomb;
        return *this;
    }
    MemoryReader& operator=(MemoryReader&& mr) noexcept {
        MemoryLocator::operator=(std::move(mr)); bomb = std::move(mr.bomb);
        return *this;
    }

    void alreadMemory(uint32_t);
    void rereadMemory(uint32_t);
    /**
      * 读取内存
      * @tparam T
      * @param skip 是否读取后跳过该段
      * @return
      */
    template <typename Type> Type readMemory(bool skip) throw(std::range_error){
        checkRead();
        Type t = *locatorMemory<Type>(skip);
        releaseLocatorReader();
        return t;
    }
    /**
     * 读取内存指针(禁止)
     *  因为该指针指向的是原内存的地址
     *  当该内存发生转移后新内存地址发生变化
     *  该指针指向原内存是无效地址
     * @tparam T
     * @param skip  是否读取后跳过该读取长度
     * @return
     */
//    template <typename Type> Type* readMemory<Type*>(bool skip) throw(std::range_error) {
//        checkRead();
//        T *t = locatorMemory<T>(skip);
//        return nullptr;
//    }
    /**
      * 读取内存，返回迭代器
      * @tparam T
      * @return
      */
    template <typename Type> ReaderIterator<Type> readMemory(){
        checkRead();
        return ReaderIterator<Type>(this);
    }
    template <typename Type> ReaderIterator<Type> endMemory(){
        checkRead();
        return ReaderIterator<Type>(this, 0);
    }
    void detonate() { detonateReader(this); }
private:
    static void detonateReader(MemoryReader*);

    /**
     * 检查阅读器是否有效
     */
    inline void checkRead() const{
        if(bomb.use_count() <= 0){ throw std::runtime_error("MemoryReader::MemoryLocator is invalid."); }
    }

    void releaseReader() { checkRead(); releaseLocatorReader(); }

    //爆破点(即拥有该阅读器的使用数量,当<=0时,回收该阅读器关联的内存)
    std::shared_ptr<MemoryReader> bomb;
};

#endif //UNTITLED8_MEMORYREADER_H
