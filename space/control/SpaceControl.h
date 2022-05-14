//
// Created by abc on 19-8-30.
//

#ifndef UNTITLED8_MEMORYCONTROL_H
#define UNTITLED8_MEMORYCONTROL_H

#include "../allocator/MemoryAllocator.h"
#include "../allocator/DiskAllocator.h"
#include "../manager/MemoryManager.h"

class MemoryReader;
class SpaceControl;
class MemoryCorrelate;
class MeetingAddressNote;

struct MemorySpace{};
struct DiskSpace{};

class BasicLocator {
public:
    BasicLocator() = default;
    explicit BasicLocator(uint32_t start, uint32_t len, uint32_t read) : buf_start(start), buf_len(len), read_len(read) {}
    virtual ~BasicLocator() = default;

    BasicLocator(const BasicLocator&) = default;
    BasicLocator(BasicLocator&&) = default;
    BasicLocator& operator=(const BasicLocator&) = default;
    BasicLocator& operator=(BasicLocator&&) = default;

    virtual uint32_t getLocatorLen() const { return buf_len; }
protected:
    //关联内存的起始点
    uint32_t buf_start;
    //关联内存的长度
    uint32_t buf_len;
    //已经读取的长度
    uint32_t read_len;
};

/**
 * 内存定位器
 *  管理定位关联器中关联的内存的位置关系
 */
class MemoryLocator : public BasicLocator{
    friend class RIterator;
public:
    MemoryLocator();
    explicit MemoryLocator(uint32_t, uint32_t,  MemoryCorrelate*)  throw(std::runtime_error);
    explicit MemoryLocator(uint32_t, char*, const std::function<void(char*, int)> &func) throw(std::runtime_error);
    ~MemoryLocator() override;

    MemoryLocator(const MemoryLocator&) = default;
    MemoryLocator(MemoryLocator&&) = default;

    MemoryLocator& operator=(const MemoryLocator&) = default;
    MemoryLocator& operator=(MemoryLocator&&) = default;
protected:
    /**
     * 定位内存
     * @tparam T
     * @param is_front
     * @return
     */
    template <typename M> M* locatorMemory(bool is_front) throw(std::range_error){
        typedef M locator_type;
        if((buf_len - read_len) < sizeof(locator_type)){
            std::string s("MemoryLocator::locator ");
            s.append(typeid(M).name());
            s.append(" length has exceeded the range.");
            throw std::range_error(s.c_str());
        }
        return reinterpret_cast<M*>((memory_locator_correlate->getCorrelateBuffer() + (buf_start + (is_front ? frontLocator(sizeof(locator_type)) : read_len))));
    }

    uint32_t frontLocator(uint32_t);
    uint32_t backLocator(uint32_t);

    /**
     * 剩下的内存长度
     * @return
     */
    uint32_t locatorLength() const { return (buf_len - read_len); }
    /**
     * 释放当前内存的读取请求
     */
    void releaseLocatorReader() { memory_locator_correlate->releaseCorrelateBuffer(); }
    /**
     * 释放关联内存
     */
    void blastLocator() { memory_locator_correlate->blastCorrelate(); }
private:
    LocatorCorrelate *memory_locator_correlate;
};

/**
 * 初始关联器
 */
class InitLocator final{
public:
    InitLocator() = default;
    explicit InitLocator(MemoryCorrelate *mc) throw(std::runtime_error) : correlate(mc){
        if(!correlate) throw std::runtime_error("InitLocator::MemoryCorrelate cannot be null!");
    }
    ~InitLocator() = default;

    void setOnReleaseMemory(std::function<void(int)> *func_) { correlate->callBackOnReleaseMemory(func_); }
    char* locatorBuffer(uint32_t);
    void  releaseBuffer();
    MemoryReader locatorCorrelate(uint32_t);
private:
    MemoryCorrelate *correlate;
};

class SpaceControl final{
    friend class MemoryManager;
public:
    SpaceControl() = default;
    ~SpaceControl() = default;

    void* malloc(uint32_t, const MemorySpace&);
    void* malloc(uint32_t, int, const MemorySpace&);
    InitLocator malloc(uint32_t) throw(std::bad_alloc);

    void* malloc(uint32_t, const DiskSpace&);

    void ergodicRecorder(){ space_manager.ergodicRecorder(); }

    void free(void*) noexcept;
    void free(void*,const SpaceHead&, const MemorySpace&) noexcept;
    void free(void*,const SpaceHead&, const DiskSpace&) noexcept;
private:
    MemoryManager space_manager;
};

#endif //UNTITLED8_MEMORYCONTROL_H
