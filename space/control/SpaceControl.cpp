//
// Created by abc on 19-8-31.
//
#include "../manager/MemoryReader.h"

void* SpaceControl::malloc(uint32_t msize, const MemorySpace&) {
    CommonHead *common_head = nullptr;
    SpaceHead space_head = convertBufSize({SPACE_TYPE_MEMORY, 0, 0, msize});

    try {
        common_head = memory_alloc.allocator<CommonHead>(static_cast<uint32_t>(reductionBufSize(space_head)));
        makeSpaceHead(common_head->buf, space_head);
    }catch (std::bad_alloc &e){
        std::cout << "allocator common buf bad_alloc(" << msize << ")" << std::endl;

    }
    return common_head ? reinterpret_cast<void*>(common_head->buf) : nullptr;
}

InitLocator SpaceControl::malloc(uint32_t msize) throw(std::bad_alloc){
    uint32_t len = std::max<uint32_t>(msize, static_cast<uint32_t>(space_manager.minMemoryLen())),
             asize = static_cast<uint32_t>(memory_attr.align_size);
    MemoryCorrelate *correlate = nullptr;
//    SpaceHead space_head = convertBufSize({SPACE_TYPE_MEMORY, 0, SPACE_CONTROL_MALLOC_CONTROL, msize + (uint32_t)sizeof(MemoryBufferUseInfo)});
    msize = space_size_round_up(len, asize);
    try{
        correlate = space_manager.obtainBuffer(msize);
    }catch (std::runtime_error &e){
        std::cout << e.what() << std::endl;
        throw std::bad_alloc();
    }

    return InitLocator(correlate);
}

void* SpaceControl::malloc(uint32_t msize, const DiskSpace&) {
    return disk_alloc.allocator(msize);
}

void SpaceControl::free(void *fp) noexcept {
    SpaceHead space_head = extractSpaceHead(reinterpret_cast<char*>(fp));

    switch (space_head.type){
        case SPACE_TYPE_MEMORY:
            free(fp, space_head, MemorySpace());
            break;
        case SPACE_TYPE_DISK:
            free(fp, space_head, DiskSpace());
            break;
        default:
            break;
    }
}

void SpaceControl::free(void *fp,const SpaceHead &head, const MemorySpace&) noexcept {
    if(head.control){
//        space_manager.returnBuffer((char*)fp, head);
    }else{
        memory_alloc.deallocator(((reinterpret_cast<char*>(fp)) - sizeof(CommonHead)), static_cast<uint32_t>(reductionBufSize(head)));
    }
}

void SpaceControl::free(void *fp,const SpaceHead &head, const DiskSpace&) noexcept {
    disk_alloc.deallocator(reinterpret_cast<char*>(fp), static_cast<uint32_t>(reductionBufSize(head)));
}
//--------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------InitLocator------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

MemoryReader InitLocator::locatorCorrelate(uint32_t bsize) {
    return MemoryReader(MemoryLocator(correlate->correlateMemoryCorrelate(bsize), bsize, correlate));
}

char* InitLocator::locatorBuffer(uint32_t offset){
    return (correlate->getCorrelateBuffer() + offset);
}

void InitLocator::releaseBuffer() {
    correlate->releaseCorrelateBuffer();
}

//--------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------MemoryLocator----------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

MemoryLocator::MemoryLocator() : BasicLocator(), memory_locator_correlate(nullptr) {}

MemoryLocator::MemoryLocator(uint32_t start, uint32_t len, MemoryCorrelate *correlate) throw(std::runtime_error)
                                                      : BasicLocator(start, len, 0), memory_locator_correlate(nullptr) {
    memory_locator_correlate = new (std::nothrow) DynamicLocatorCorrelate(this, correlate);
}

MemoryLocator::MemoryLocator(uint32_t buffer_len, char *locator_buffer, const std::function<void(char*, int)> &func) throw(std::runtime_error)
                                                   : BasicLocator(0, buffer_len, 0), memory_locator_correlate(nullptr) {
    memory_locator_correlate = new (std::nothrow) FixedLocatorCorrelate(this, locator_buffer, func);
}

MemoryLocator::~MemoryLocator() {
    delete memory_locator_correlate;
}

/**
 * 先前移动读取点
 * @param front_size
 * @return
 */
uint32_t MemoryLocator::frontLocator(uint32_t front_size) {
    uint32_t old_locator = read_len;

    if((read_len += front_size) > buf_len){
        read_len = buf_len;
    }

    return old_locator;
}

/**
 * 向后移动读取点
 * @param back_size
 * @return
 */
uint32_t MemoryLocator::backLocator(uint32_t back_size) {
    uint32_t old_locator = read_len;

    if(read_len < back_size) {
        read_len = 0;
    }else{
        read_len -= back_size;
    }

    return old_locator;
}