//
// Created by abc on 19-9-2.
//
#include "../control/SpaceControl.h"

//static template <typename T, typename... Args> MemoryCorrelate<T>* CreateCorrelateMemory(Args&&... args){
//    MemoryCorrelate<T> *buf_correlate = memory_alloc.allocator<MemoryCorrelate<T>>();
//
//    if(buf_correlate){
//        memory_alloc.construct(buf_correlate, args);
//    }
//
//    return buf_correlate;
//}
//
//static template <typename T> void DestroyCorrelateMemory(MemoryCorrelate<T> *correlate){
//    if(correlate){
//        if(std::is_same<T, MemoryManager>::value){
//            memory_alloc.destroy(correlate);
//        }
//        memory_alloc.deallocator<MemoryCorrelate<T>>(correlate);
//    }
//
//}
//
//template <typename T> MemoryCorrelate<T>::MemoryCorrelate(CommonHead *head, MemoryManager *m)
//        : memory_done(false){
//    manager_info.buf_head = head;
//    manager_info.manager = m;
//    sameCorrelateTemplate<MemoryManager>();
//}
//
//template <typename T> MemoryCorrelate::MemoryCorrelate(MemoryCorrelate *parent) : memory_done(false), parent(parent) {
//    sameCorrelateTemplate<MemoryCorrelate>();
//}
//
//template <typename T> MemoryCorrelate<T>::~MemoryCorrelate() {
//    if(std::is_same<T, MemoryManager>::value){
//        while(!manager_info.child.empty()){
//            DestroyCorrelateMemory<MemoryCorrelate>(manager_info.child.pop());
//        }
//    }
//}

//template <typename T,typename U, typename... Args> MemoryCorrelate<U>&
//                                            MemoryCorrelate<T>::swap(const MemoryCorrelate<T> &correlate, Args&&... args) {
//    MemoryCorrelate<T> res(correlate.buf_head, correlate.correlate_lock, correlate.child);
//    setTemplate<U>(res, std::forward<Args>(args)...);
//    return res;
//}

//template <typename T, typename U> void MemoryCorrelate<T>::setTemplate(correlate_parent &correlate, MemoryManager *manager){
//    correlate.manager = manager;
//}
//
//template <typename T, typename U> void MemoryCorrelate<T>::setTemplate(correlate_child &correlate, correlate_parent *mc,
//                                                                       std::list<correlate_child*>::iterator &&iterator){
//    correlate.parent = std::make_pair(mc, iterator);
//}
//
//template <typename T, typename U> void MemoryCorrelate<T>::setTemplate(correlate_parent &correlate, const correlate_parent &set_){
//    correlate.manager = set_.manager;
//}
//
//template <typename T, typename U> void MemoryCorrelate<T>::setTemplate(correlate_child &correlate, const correlate_child &set_){
//    correlate.parent = std::move(set_.parent);
//}

//template <typename T, typename U> void MemoryCorrelate<T>::sameCorrelateTemplate() const {
//    if(!std::is_same<T, U >::value){
//        throw std::bad_cast();
//    }
//}
//
//template <typename T> CommonHead* MemoryCorrelate<T>::getCorrelate() {
//    if(std::is_same<T, MemoryManager>::value) {
//        manager_info.correlate_lock.lock_shared();
//        return manager_info.buf_head;
//    }else{
//        return parent->manager_info.buf_head;
//    }
//}
//
//template <typename T> void MemoryCorrelate<T>::setCorrelate(CommonHead *head) {
//    sameCorrelateTemplate<MemoryManager>();
//    std::unique_lock<std::shared_timed_mutex> ulock(manager_info.correlate_lock);
//    manager_info.buf_head = head;
//}
//
//template <typename T> void MemoryCorrelate<T>::releaseCorrelate() {
//    std::is_same<T, MemoryManager>::value ? manager_info.correlate_lock.unlock_shared()
//                                          : parent->manager_info.correlate_lock.unlock_shared();
//}
//
//template <typename T> void MemoryCorrelate<T>::blastCorrelate() {
//    if(std::is_same<T, MemoryManager>::value){
//        memory_done.store(true, std::memory_order_release);
//        if(manager_info.child.empty()){
//            manager_info.manager->obtain_buffer();
//        }
//    }else{
//        parent->unmountMemoryCorrelate(this);
//        memory_done.store(true, std::memory_order_release);
//    }
//}
//
//template <typename T, typename U> void MemoryCorrelate<T>::mountMemoryCorrelate(CorrelateChild *mount) {
//    sameCorrelateTemplate<MemoryManager>();
//    if(mount){
//        manager_info.child.push(mount);
//        manager_info.child_size.fetch_add(1);
//        mount->adsorbMemoryCorrelate(this);
//    }
//}
//
//template <typename T, typename U> void MemoryCorrelate::unmountMemoryCorrelate(CorrelateChild *unmount) {
//    sameCorrelateTemplate<MemoryManager>();
//    if(unmount && !unmount->memory_done.load(std::memory_order_acquire)) {
//        manager_info.child_size.fetch_sub(1);
//        unmount->unadsorbMemoryCorrelate();
//    }
//}
//
//template <typename T, typename U> void MemoryCorrelate<T>::adsorbMemoryCorrelate(CorrelateParent *adsorb) {
//    sameCorrelateTemplate<MemoryCorrelate>();
//    parent = adsorb;
//}
//
//template <typename T, typename U> void MemoryCorrelate<T>::unadsorbMemoryCorrelate() {
//    sameCorrelateTemplate<MemoryCorrelate>();
//    parent = nullptr;
//}

/**
 * 构造MemoryBuffer
 * @param froce
 * @param msize
 * @param permit_info
 * @param init_
 */
void MemoryProducer::makeMemoryBuffer(bool force, int msize, PermitInfo *permit_info, std::function<void(int)> init_) {
    int (*func_)(MemoryProducer*, std::atomic_int&) = nullptr;
    //设置申请函数
    if(force){
        //申请MemoryBuffer
        func_ = reinterpret_cast<int(*)(MemoryProducer*, std::atomic_int&)>(&MemoryProducer::createForce);
    }else{
        //强制申请MemoryBuffer
        func_ = reinterpret_cast<int(*)(MemoryProducer*, std::atomic_int&)>(&MemoryProducer::createApply);
    }

    //调用申请函数
    if((*func_)(this, permit_info->status)){
        //初始化MemoryBuffer
        onInitMemoryBuffer(init_, msize);
        //完成申请MemoryBuffer
        doneMemoryBuffer(permit_info);
    }
}

/**
 * 完成申请MemoryBuffer
 * @param permit_info
 */
void MemoryProducer::doneMemoryBuffer(PermitInfo *permit_info) {
    int status_ = MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST;

    //更改状态
    while(!permit_info->status.compare_exchange_weak(status_, MEMORY_BUFFER_THREAD_PERMIT_STATUS_DONE, std::memory_order_release, std::memory_order_consume)){
        if(status_ != MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST){
            break;
        }
    }
}

/**
 * 强制申请MemoryBuffer
 * @param permit_status
 * @return
 */
int MemoryProducer::createForce(std::atomic_int &permit_status){
    int status_ = permit_status.load(std::memory_order_consume);

    switch (status_){
        case MEMORY_BUFFER_THREAD_PERMIT_STATUS_DONE:
            //转换状态
            waitCreate(permit_status);
        case MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT:
            //申请MemoryBuffer
            return createApply(permit_status);
        case MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST:
        default:
            return 0;
    }
}

/**
 * 申请MemoryBuffer
 * @param permit_status
 * @return
 */
int MemoryProducer::createApply(std::atomic_int &permit_status) {
    int status_ = MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT;

    //更改状态
    while(!permit_status.compare_exchange_weak(status_, MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST, std::memory_order_release, std::memory_order_consume)){
        if(status_ != MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT){
            break;
        }
    }

    return (status_ == MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT) ? 1 : 0;
}

/**
 * 转换等待状态
 * @param permit_status
 */
void MemoryProducer::waitCreate(std::atomic_int &permit_status) {
    int status_ = MEMORY_BUFFER_THREAD_PERMIT_STATUS_DONE;

    while(!permit_status.compare_exchange_weak(status_, MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT, std::memory_order_release, std::memory_order_consume)){
        if(status_ != MEMORY_BUFFER_THREAD_PERMIT_STATUS_DONE){
            break;
        }
    }

}

/**
 * 回调构造MemoryBuffer
 * @param buffer
 * @param permit_info
 */
void MemoryProducer::onMakeMemoryBuffer(MemoryBuffer*, PermitInfo *permit_info) {
    permit_info->permit.fetch_add(MEMORY_BUFFER_THREAD_PERMIT_CHANGE_VALUE, std::memory_order_release);
}

/**
 * 回调消耗MemoryBuffer
 * @param buffer
 * @param permit_info
 */
void MemoryProducer::onConsumeMemoryBuffer(MemoryBuffer*, PermitInfo *permit_info) {
    int old_permit_value = permit_info->permit.fetch_sub(MEMORY_BUFFER_THREAD_PERMIT_CHANGE_VALUE, std::memory_order_release);

    if(old_permit_value == MEMORY_BUFFER_THREAD_PERMIT_DONE_VALUE){
        waitCreate(permit_info->status);
    }
}

/**
 * 构造内存
 * @param size  MemoryBuffer的数量
 * @param len   内存总长度
 * @param buffers
 * @return
 */
int MemoryProducer::initMemoryBuffer(int size, int len, InitMemoryBuffer *buffers) throw(std::bad_alloc){
    int cinit_size = 0;
    SpaceHead space_head = convertBufSize({SPACE_TYPE_MEMORY, 0, 1, static_cast<uint32_t>(len)});
    len = reductionBufSize(space_head);

    for(int i = 0; i < size; i++, cinit_size++){
        try {
            buffers[i].len = len - sizeof(CommonHead);
            //申请len长度的内存
            buffers[i].common = reinterpret_cast<CommonHead*>(createMemoryBuffer(len));
            memset(buffers[i].common, 0, static_cast<size_t>(len));
        }catch (std::bad_alloc &e){
            //没有申请到内存,抛出异常
            if(!cinit_size){
                throw;
            }
            //申请到内存，但不是需要的数量，结束循环
            break;
        }

        if(buffers[i].common) {
            //初始化SpaceHead
            makeSpaceHead(buffers[i].common->buf, space_head);
        }
    }

    //返回申请到内存的数量
    return cinit_size;
}

void MemoryProducer::uinitMemoryBuffer(const InitMemoryBuffer &buffers) {
    if(buffers.common && (buffers.len > 0)){
        destroyMemoryBuffer(reinterpret_cast<char*>(buffers.common), buffers.len + sizeof(CommonHead));
    }
}

void MemoryProducer::uinitMemoryBuffer(CommonHead *common) {
    if(common){
        destroyMemoryBuffer(reinterpret_cast<char*>(common), reductionBufSize(extractSpaceHead(common->buf)));
    }
}

/**
 * 申请MemoryBuffer的内存
 * @param size
 * @return
 */
char* MemoryProducer::createMemoryBuffer(int size) {
    return memory_alloc.allocator<char>(static_cast<uint32_t>(size));
}

/**
 * 销毁MemoryBuffer的内存
 * @param buf
 * @param size
 */
void MemoryProducer::destroyMemoryBuffer(char *buf, int size) {
    memory_alloc.deallocator(buf, static_cast<uint32_t>(size));
}

//--------------------------------------------------------------------------------------------------------------------//
//----------------------------------------MemoryAdjudicator-----------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * 把MemoryBufferHead从sign链表中取出
 * (1) 令c.next ==> prev
 * (2) prev 比较交换 c, next  ==> 成功返回
 * (3)失败说明 b 也执行相同操作, 重新加载prev(prev.next.load),即指向 a
 * (4)重新执行第(2)步操作(直到 b 的取出操作完成, a.next 才指向c,第(2)才会成功返回)
 *
 *          |=h=|
 *            |
 *            |
 *            —
 *       -- |=a=|---------------
 *       |    |                |
 *       |    |                |
 *   (3) |    |                |
 *       |    —                |
 *   -----  |=b=|   <----      |(4)
 *   |   |    |         |      |
 *   |   |    |         |      |
 *   |   |    |         | (1)  |
 *   |   |    —         |      |
 *   |   -> |=c=|   -----      |
 *   |        |                |
 *   |(2)     |                |
 *   |        |                |
 *   |        —                |
 *   --->   |=d=|  <------------
 * @param prev
 * @param dsc
 */

/**
 * MemoryBuffer的内存使用状态(空闲内存占总空间比)
 * Fixed不可使用
 */
MemoryBufferSign MemoryAdjudicator::memoryBufferUseStatus(MemoryBuffer *buffer) {
    if(buffer->sign != MEMORY_BUFFER_SIGN_FIXED) {
        return memoryBufferUseStatus0(buffer->use_amount.load(std::memory_order_consume), buffer->buffer_len, buffer->sign);
    }else{
        return buffer->sign;
    }
}

/**
 * 回收use_info并转换为对应的idle_info
 * @param use_info
 * @param pi_info
 * @return
 */
MemoryBufferIdleInfo* MemoryAdjudicator::recoverMemoryBufferUseInfo(MemoryBufferUseInfo *use_info, MemoryBufferIdleInfo *pi_info, bool is_last) {
    MemoryBuffer *buffer = nullptr;
    MemoryBufferUseInfo *pu_info = nullptr;
    MemoryBufferIdleInfo *idle_info = nullptr;
    MemoryCorrelate *use_correlate = nullptr;

    //判断use_info的回收位和use_info是否被转移到另一个MemoryBuffer里
    if((use_info->delay_len ==  MEMORY_BUFFER_SUBSTITUTE_INFO_DELAY_LEN)){
        //use_info->delay_len == MEMORY_BUFFER_SUBSTITUTE_INFO_DELAY_LEN 说明该use_info被装转移到另一个MemoryBuffer里
        use_info->delay_len = 0;
        buffer = use_info->parent;
        pu_info = use_info->prev;
    }else if(use_info->info_status.load(std::memory_order_consume) == MEMORY_BUFFER_USE_INFO_STATUS_DEATH){
        buffer = use_info->parent;
        pu_info = use_info->prev;
        use_correlate = use_info->correlate;
    }

    if(buffer){
        //卸载use_info
        unlinkMemoryBufferUseInfo(use_info);
        //构造并挂载idle_info,返回该idle_info
        idle_info = linkMemoryBufferIdleInfo(initMemoryBufferIdleInfo(buffer, pu_info, use_info->start,
                                                                      is_last ? buffer->buffer_len - use_info->start
                                                                              : use_info->len + use_info->delay_len),
                                                                      pi_info, buffer);
        //释放FIXED的关联的MemoryCorrelate内存
        if(use_correlate){
            onReleaseMemoryCorrelate(use_correlate);
        }
    }

    return idle_info;
}

/**
 * 搜索可用(空闲)的内存空间
 * @param buffer                    被搜索的MemoryBuffer
 * @param use_callback_func         use_info回调函数
 * @param idle_callback_func        idle_info回调函数
 * @param len                       搜索长度
 * @param auto_shrink_buffer        收缩idle_info的空间(将多个合并成一个)
 * @return
 */
CommonHead* MemoryAdjudicator::searchAvailableBuffer(MemoryBuffer *buffer, std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> *lookup_func,
                                                     std::function<void(MemoryBufferUseInfo*)> *use_callback_func,
                                                     std::function<void(MemoryBufferIdleInfo*)> *idle_callback_func, int len, int search_flag) {
    //MemoryBuffer剩下的可用内存(大约)
    int remain_len = buffer->buffer_len - buffer->use_amount.load(std::memory_order_consume),
        idle_len = 0, search_pos = 0;
    MemoryBufferUseInfo *lpu_info = nullptr;
    MemoryBufferIdleInfo *pidle_info = nullptr;

    /*
     * 探查函数,探测对应的idle_info的可用内存是否满足于需求
     */
    auto probe_func = [&](MemoryBufferIdleInfo *idle_info) -> void {
        //减少剩下的可用内存
        remain_len -= (idle_len = getMemoryBufferIdleLen(reinterpret_cast<char*>(idle_info)));

        if(idle_len >= len){
            //满足需要,标记pidle_info
            pidle_info = idle_info;
            //标记搜索内存点
            search_pos = pidle_info->start;
            //减少空间长度
            pidle_info->start += len;
        }

        //因为idle_info需要合并pi_info时,当执行idle_info时,减少的长度是(pi_info.len + idle_info.len),
        //所以执行pi_info时减去的pi_info.len,结束后需要加上pi_info.len
        if(get_flag(search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)){
            remain_len += idle_len;
        }
    };

    auto break_func = [&]() -> bool {
        //返回是否结束遍历MemoryBuffer的use_info和idle_info
        return (((remain_len < len) && !get_flag(search_flag, MEMORY_BUFFER_FLAG_NOSIZE_UNBREAK)) ||
                (pidle_info && !get_flag(search_flag, MEMORY_BUFFER_FLAG_FIND_UNBREAK)));
    };

    std::function<MemoryBufferIdleInfo*(SearchInfo*)> use_func = [&](SearchInfo *sinfo) -> MemoryBufferIdleInfo* {
        MemoryBufferIdleInfo *recover_use_info = nullptr;

        if(!(recover_use_info = recoverMemoryBufferUseInfo(sinfo->use_info, sinfo->pi_info, sinfo->is_last))){
            //没有回收use_info
            if(use_callback_func){
                (*use_callback_func)(sinfo->use_info);
            }
        }

        return recover_use_info;
    };

    std::function<int(SearchInfo*)> idle_func = [&](SearchInfo *sinfo) -> bool {
        lpu_info = sinfo->pu_info;

        //判断是否合并
        if(isMergeMemoryBufferIdleInfo(sinfo->idle_info, sinfo->pi_info)){
            mergeMemoryBufferIdleInfo(sinfo->memory_buffer, sinfo->idle_info, sinfo->pi_info, MEMORY_BUFFER_MERGE_IDLE_INFO_UNLINK);
        }

        //判断该idle_info是最后一个及后面还有内存未回收
        if(sinfo->is_last && (buffer->buffer_len > sinfo->idle_info->end)){
            //向后扩展该idle_info的可用空间
            sinfo->idle_info = expandMemoryBufferIdleInfo(sinfo->idle_info, sinfo->memory_buffer, sinfo->memory_buffer->buffer_len);
        }

        if(idle_callback_func){
            (*idle_callback_func)(sinfo->idle_info);
        }

        //探测idle_info
        probe_func(sinfo->idle_info);

        return break_func();
    };

    static std::function<void(MemoryAdjudicator*, SearchInfo*)> shrink_func = [](MemoryAdjudicator *adjudicator, SearchInfo *sinfo) -> void{
        adjudicator->shrinkMemoryBufferIdleInfo0(sinfo->pi_info, sinfo->upi_info, sinfo->memory_buffer);
    };

    //搜索信息类
    SearchInfo search_info(buffer, &use_func, &idle_func, get_flag(search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER) ? &shrink_func : nullptr);
    searchMemoryBuffer(&search_info);

    //已经探测到了可用空间
    if(pidle_info){
        MemoryBufferUseInfo *use_info = nullptr;
        if(lookup_func){
            use_info = (*lookup_func)(buffer, search_pos, len, 0);
            //设置位于idle_info前面的use_info
            pidle_info->ipu_info = use_info;
            shrinkMemoryBufferIdleInfo(pidle_info, lpu_info, buffer);
        }else{
            //FIXED
            use_info = initMemoryBufferUseInfo(getMemoryBufferUseInfo(buffer->buffer + search_pos, len), nullptr, buffer, search_pos, len,
                                               shrinkMemoryBufferIdleInfo(pidle_info, use_info, buffer));
        }

        onConstructMemoryBuffer(reinterpret_cast<CommonHead*>((buffer->buffer + search_pos)), search_flag);

        linkMemoryBufferUseInfo(use_info, lpu_info);
        buffer->use_amount.fetch_add(len, std::memory_order_release);
    }

    return (pidle_info ? reinterpret_cast<CommonHead*>(buffer->buffer + search_pos) : nullptr);
}

/**
 * 修复负载内存
 * fixed不可用
 * @param load_buffer
 * @param use_func
 * @param idle_func
 */
void MemoryAdjudicator::repairLoadMemoryBuffer(MemoryBuffer *load_buffer, std::function<void(MemoryBufferUseInfo*)> *use_func, std::function<void(MemoryBufferIdleInfo*)> *idle_func) {
    int memory_use_len = load_buffer->use_amount.load(std::memory_order_consume);

    std::function<MemoryBufferIdleInfo*(SearchInfo*)> search_use_func = [&](SearchInfo *sinfo) -> MemoryBufferIdleInfo*{
        MemoryBufferIdleInfo *recover_use_info = nullptr;

        //判断use_info的回收位
        if(!(recover_use_info = recoverMemoryBufferUseInfo(sinfo->use_info, sinfo->pi_info, sinfo->is_last))){
            if(use_func != nullptr){
                //回调use_info给调用者
                (*use_func)(sinfo->use_info);
            }
        }

        return recover_use_info;
    };

    std::function<int(SearchInfo*)> search_idle_func = [&](SearchInfo *sinfo) -> int{
        if(isMergeMemoryBufferIdleInfo(sinfo->idle_info, sinfo->pi_info)){
            mergeMemoryBufferIdleInfo(sinfo->memory_buffer, sinfo->idle_info, sinfo->pi_info, MEMORY_BUFFER_MERGE_IDLE_INFO_UNLINK);
        }

        //判断该idle_info是最后一个及后面还有内存未回收
        if(sinfo->is_last && (load_buffer->buffer_len > sinfo->idle_info->end)){
            //向后扩展该idle_info的可用空间
            sinfo->idle_info = expandMemoryBufferIdleInfo(sinfo->idle_info, sinfo->memory_buffer, sinfo->memory_buffer->buffer_len);
        }

        if(idle_func != nullptr){
            //回调idle_info给调用者
            (*idle_func)(sinfo->idle_info);
        }

        return 0;
    };

    SearchInfo search_info(load_buffer, &search_use_func, &search_idle_func, nullptr);
    searchMemoryBuffer(&search_info);

    //获取Buffer的新状态
    load_buffer->sign = memoryBufferUseStatus0(memory_use_len, load_buffer->buffer_len ,load_buffer->sign);
}

/**
 * 初始化使用内存信息
 * @param buffer_info
 * @param buffer
 * @return
 */
MemoryBufferUseInfo* MemoryAdjudicator::initMemoryBufferUseInfo(MemoryBufferUseInfo *use_info, MemoryCorrelate *correlate ,MemoryBuffer *buffer, int start, int len, int delay_len) {
    memory_alloc.construct(use_info, start, len, delay_len, buffer, correlate);
    return use_info;
}

/**
 * 初始化空闲内存信息
 * @param buffer
 * @param start
 * @param len
 * @return
 */
MemoryBufferIdleInfo* MemoryAdjudicator::initMemoryBufferIdleInfo(MemoryBuffer *buffer, MemoryBufferUseInfo *ipu_info, int start, int len) {
    MemoryBufferIdleInfo *idle_info = nullptr;

    if(len >= sizeof(MemoryBufferIdleInfo)){
        memory_alloc.construct((idle_info = getMemoryBufferIdleInfo(buffer->buffer + start, len)), start, start + len, ipu_info);
    }

    return idle_info;
}

/**
 * 初始化替补内存信息
 * @param use_info
 * @return
 */
MemoryBufferSubstituteInfo* MemoryAdjudicator::initMemoryBufferSubstituteInfo(MemoryBufferUseInfo *use_info) {
    MemoryBufferSubstituteInfo *substitute_info = nullptr;

    if(use_info){
        memory_alloc.construct((substitute_info = getMemoryBufferSubstituteInfo(use_info->parent->buffer + use_info->start, use_info->len)),
                               use_info->start, use_info->len, use_info->parent, use_info->prev, use_info->next);
    }

    return substitute_info;
}

/**
 * 处理负载内存
 * @param load_buffers
 * @param response_func
 * @param request_func
 * @param idle_size
 * @param buffer_size
 * @param load_buffer_len
 * @param alloc_buffer_len
 */
void MemoryAdjudicator::treatmentLoadMemoryBuffer(MemoryBufferHolder &load_holder, int load_use_len, int load_use_size, int load_buffer_len, int load_buffer_size, int alloc_buffer_len) {
    if((((load_buffer_len - load_use_len) / alloc_buffer_len)) >= 1) {
//    提交线程任务([=]() -> void {
        forceLoadMemoryBuffer(load_holder, load_use_size, load_buffer_size, alloc_buffer_len);
//    });
    }
    //响应合并后的MemoryBuffer(负载系数不够,重新挂载)
    onResponseMemoryBuffer(load_holder);
}

/**
 * 强制修复负载内存
 * @param load_buffers
 * @param buffer_size
 */
void MemoryAdjudicator::forceLoadMemoryBuffer(MemoryBufferHolder &load_holder, int load_use_size, int load_buffer_size, int) {
    int buffer_pos = 0, use_pos = 0, load_use_len = 0, insert_use_pos = -1;
    MemoryBufferMergeInfo merge_info = MemoryBufferMergeInfo();
    MergeMemory merge_array[load_buffer_size];
    MergeLocationTable location_table[load_buffer_size];
    MergeMemoryUseNote merge_use_array[load_use_size];

    {
        memset(merge_array, 0, sizeof(MergeMemory) * load_buffer_size);
        memset(location_table, 0, sizeof(MergeLocationTable) * load_buffer_size);
        memset(merge_use_array, 0, sizeof(MergeMemoryUseNote) * load_use_size);
    }

    std::function<void(MemoryBufferUseInfo*)> use_func = [&](MemoryBufferUseInfo *use_info) -> void {
        load_use_len += use_info->len;

        if(load_use_size > use_pos){
            //将use_info排序的插入merge_use_array中
            insert_use_pos = InsertSortUse(location_table + buffer_pos, reinterpret_cast<char*>(use_info), merge_use_array, use_info->len, use_pos++);
            onMemoryBufferRecorderDebris(MEMORY_ADJUDICATOR_BUFFER_INFO_INCREASE_VALUE);
        }
    };

    std::function<void(MemoryBufferIdleInfo*)> idle_func = [&](MemoryBufferIdleInfo *idle_info) -> void{
        if(insert_use_pos >= 0){
            //设置use_info后面的idle_info(idle_info对应的use_info只能是一个或没有,只能获取前面的use_info)
            //|-...-|-use_info-|-idle_info-|-...-|
            //                    uni_info
            merge_use_array[insert_use_pos].uni_info = reinterpret_cast<char*>(idle_info);
        }
    };

    load_holder.onHolderNote([&](MemoryBufferIterator iterator) ->  bool {
        if((--load_buffer_size) <= 0){ return false; }

        //修复MemoryBuffer及遍历use_info
        repairLoadMemoryBuffer(*iterator, &use_func, &idle_func);

        if(load_use_len > 0) {
            //插入排序memory_buffer及初始化merge_array
            InsertSortMemory(merge_array, location_table, reinterpret_cast<char*>(*iterator), buffer_pos++,
                             (*iterator)->buffer_len - load_use_len, (*iterator)->use_info_size);
        }

        load_use_len = 0;
        insert_use_pos = -1;
        return false;
    });

    MergeMemoryFunc(merge_array, merge_use_array, reinterpret_cast<char*>(&merge_info), getMemoryBufferIdleLen,
                                nextMemoryBufferIdle, insertMemoryBufferUseToIdle, clearMergeInfo, buffer_pos, use_pos);
}

/**
 * 链接MemoryBufferUseInfo
 * 从MemoryBufferHead上获取到新的内存,需要创建MemoryBufferUseInfo并链接到MemoryBufferHead
 * @param use_info
 * @param pu_info
 * @param buffer 因为use_info可能实际类型为MemoryBufferSubstituteInfo,没有parent,所以需要该参数
 */
void MemoryAdjudicator::linkMemoryBufferUseInfo(MemoryBufferUseInfo *use_info, MemoryBufferUseInfo *pu_info) {
    if(pu_info){
        use_info->next = pu_info->next;
        pu_info->next = use_info;
    }else{
        use_info->next = use_info->parent->use_buf_info;
        use_info->parent->use_buf_info = use_info;
    }
    use_info->prev = pu_info;

    if(use_info->next){
        use_info->next->prev = use_info;
    }


    use_info->parent->use_info_size += MEMORY_ADJUDICATOR_BUFFER_INFO_INCREASE_VALUE;
}

/**
 * 断开MemoryBufferUseInfo
 * 从MemoryBufferHead上断开MemoryBufferUseInfo,因为该MemoryBufferUseInfo已经被释放
 * @param use_info
 * @param pu_info
 * @param buffer 同linkMemoryBufferUseInfo
 */
void MemoryAdjudicator::unlinkMemoryBufferUseInfo(MemoryBufferUseInfo *use_info) {
    if(use_info->prev){
        use_info->prev->next = use_info->next;
    }else{
        use_info->parent->use_buf_info = use_info->next;
    }

    if(use_info->next){
        use_info->next->prev = use_info->prev;
    }

    use_info->parent->use_info_size -= MEMORY_ADJUDICATOR_BUFFER_INFO_INCREASE_VALUE;
}

/**
 * 将use_info指向的内存空间生成substitute_info
 * 将substitute_info替换掉use_info
 * @param use_info
 * @param pu_info
 */
MemoryBufferUseInfo* MemoryAdjudicator::substituteMemoryBufferUseInfo(MemoryBufferUseInfo *use_info, MemoryBufferIdleInfo *uni_info) {
    MemoryBufferSubstituteInfo *substitute_info = initMemoryBufferSubstituteInfo(use_info);

    if(substitute_info->prev){
        substitute_info->prev->next = reinterpret_cast<MemoryBufferUseInfo*>(substitute_info);
    }else{
        substitute_info->parent->use_buf_info = reinterpret_cast<MemoryBufferUseInfo*>(substitute_info);
    }

    if(substitute_info->next){
        substitute_info->next->prev = reinterpret_cast<MemoryBufferUseInfo*>(substitute_info);
    }

    if(uni_info){
        uni_info->ipu_info = reinterpret_cast<MemoryBufferUseInfo*>(substitute_info);
    }

    return reinterpret_cast<MemoryBufferUseInfo*>(substitute_info);
}

/**
 * 获取Buffer的新状态
 * @param muse_len
 * @param buffer_len
 * @param sign
 * @return
 */
MemoryBufferSign MemoryAdjudicator::memoryBufferUseStatus0(int muse_len, int buffer_len, MemoryBufferSign sign) {
    float load_factor =  (static_cast<float>(muse_len)) / buffer_len;

    switch (sign){
        case MEMORY_BUFFER_SIGN_IDLE:
        case MEMORY_BUFFER_SIGN_USE:
        case MEMORY_BUFFER_SIGN_LOAD:
            if(load_factor >= MEMORY_BUFFER_LOAD_FACTOR){
                sign = MEMORY_BUFFER_SIGN_LOAD;
            }else{
                sign = MEMORY_BUFFER_SIGN_USE;
            }
            break;
        default:
            break;
    }

    return sign;
}


/**
 * 按顺序遍历MemoryBuffer的use_info和idle_info
 * 遍历MemoryBuffer时能存在相邻的idle_info,但合并MemoryBuffer时不能存在相邻的idle_info
 * @param search_info
 */
void MemoryAdjudicator::searchMemoryBuffer(SearchInfo *search_info) {
    //use_info和idle_info的数量
    int msize = search_info->memory_buffer->use_info_size + search_info->memory_buffer->idle_info_size;

    search_info->use_info = search_info->memory_buffer->use_buf_info;
    search_info->idle_info = search_info->memory_buffer->idle_buf_info;
    search_info->is_last = false;

    for(;;){
        if((--msize) <= 0){
            search_info->is_last = true;
        }

find_new_idle:
        /*
         * 执行use_info或idle_info
         * 判断没有idle_info 或 (有use_info 且 use_info在idle_info的前面)
         */
        if(!search_info->idle_info || (search_info->use_info && (search_info->use_info->start < search_info->idle_info->start))){
            //执行use_info的回调函数
            MemoryBufferIdleInfo *ni_info = (*search_info->use_func)(search_info);

            //判断是否回收了use_info
            if(ni_info){
                search_info->use_info = search_info->pu_info ? search_info->pu_info->next : search_info->memory_buffer->use_buf_info;

                //回收了use_info并转换对应的idle_info
                //设置idle_info、use_info
                search_info->idle_info = ni_info;

                //重新执行
                goto find_new_idle;
            }else{
                //判断是否回收了use_info
                //设置pu_info、use_info
                search_info->pu_info = search_info->use_info;
                search_info->use_info = search_info->pu_info->next;
            }
        }else{
            //判断是否需要移动合并pi_info
            if(search_info->pi_info && search_info->shrink_func){
                (*search_info->shrink_func)(this, search_info);

                search_info->pi_info = search_info->idle_info->prev;
            }

            //执行idle_info的回调函数
            if((*search_info->idle_func)(search_info)){
                break;
            }

            //设置upi_info为pi_info前面的use_info(shrink_func函数需要使用)
            search_info->upi_info = search_info->pu_info;
            search_info->idle_info = (search_info->pi_info = search_info->idle_info)->next;

        }

        if(!msize){
            //执行完了use_info和idle_info,结束循环（需要回调pi_info）
            break;
        }
    }
}

/**
 * 链接MemoryBufferIdleInfo(不需要合并处理,留给调用者合并)
 * @param idle_info
 * @param pi_info
 * @param buffer
 * @return
 */
MemoryBufferIdleInfo* MemoryAdjudicator::linkMemoryBufferIdleInfo(MemoryBufferIdleInfo *idle_info, MemoryBufferIdleInfo *pi_info, MemoryBuffer *buffer) {
    buffer->idle_info_size += MEMORY_ADJUDICATOR_BUFFER_INFO_INCREASE_VALUE;
    if(pi_info){
        /*
         * (1)|___(pi_info)____|_____(idle_info)______|___________|.....|_________|
         * 或
         * (2)|___....___|___(pi_info)____|_____(idle_info)______|___________|.....|_________|
         * 或
         * (3)|___|....|___(pi_info)____|______....____|____(idle_info)____|_____|...|____|
         */
        idle_info->prev = pi_info;
        idle_info->next = pi_info->next;

        if(pi_info->next){
            pi_info->next->prev = idle_info;
        }
        pi_info->next = idle_info;
//        if(isMergeMemoryBufferIdleInfo(idle_info, pi_info)) {
//            //(1)和(2)需要合并
//            mergeMemoryBufferIdleInfo(buffer, idle_info, pi_info, MEMORY_BUFFER_MERGE_IDLE_INFO_UNLINK);
//        }
    }else{
        /*
         * |___(pu_info)___|____(idle_info)_____|______|....|______|
         * 或
         * |_____(idle_info)____|__________|....|____|
         */
        if(buffer->idle_buf_info){
            buffer->idle_buf_info->prev = idle_info;
        }
        idle_info->next = buffer->idle_buf_info;

        buffer->idle_buf_info = idle_info;
    }

    return idle_info;
}

/**
 * 移除MemoryBufferIdleInfo
 * @param idle_info  移除目标
 * @param buffer
 * @return
 */
MemoryBufferIdleInfo* MemoryAdjudicator::unlinkMemoryBufferIdleInfo(MemoryBufferIdleInfo *idle_info, MemoryBuffer *buffer){
    buffer->idle_info_size -= MEMORY_ADJUDICATOR_BUFFER_INFO_INCREASE_VALUE;

    if(idle_info->prev){
        idle_info->prev->next = idle_info->next;
    }else{
        buffer->idle_buf_info = idle_info->next;
    }

    if(idle_info->next){
        idle_info->next->prev = idle_info->prev;
    }

    return idle_info->prev;
}

/**
 * 将use_info移动到buf的位置上
 * @param buffer    移动目标
 * @param use_info  移动源 (非MemoryBufferSubstituteInfo)
 * @param opu_info  同一个buffer上不需要使用该参数
 * @param pu_info   移动目标的use_info位置前一个use_info
 * @param idle_info 复制品,非buffer上对应的idle_info
 */
MemoryBufferUseInfo* MemoryAdjudicator::memmoveMemoryBufferUseInfo(MemoryBuffer *buffer, MemoryBufferUseInfo *use_info, MemoryBufferIdleInfo *uni_info, MemoryBufferUseInfo *npu_info,
                                                                   MemoryBufferIdleInfo *idle_info, std::function<void(int)> move_func){
    char *move_buf = nullptr, *src_buf = nullptr;

    std::function<char*()> correlate_func = [&]() -> char* {
        memmove(move_buf, src_buf, static_cast<size_t>(use_info->len));
        return move_buf;
    };

    /*
     * 判断use_info,idle_info,移动的位置,需要移动的长度
     */
    if(use_info && idle_info && (move_buf = buffer->buffer + idle_info->start) && (src_buf = use_info->parent->buffer + use_info->start)){

        //不在同一个MemoryBuffer上转移use_info
        if(use_info->parent != buffer){
            int info_status = 0, use_len = use_info->len;
            do{
                info_status = use_info->info_status.load(std::memory_order_consume);
                if(info_status == MEMORY_BUFFER_USE_INFO_STATUS_DEATH){
                    use_len = 0;
                    break;
                }
            }while(!use_info->info_status.compare_exchange_weak(info_status, MEMORY_BUFFER_USE_INFO_STATUS_MOVE, std::memory_order_release, std::memory_order_relaxed));

            substituteMemoryBufferUseInfo(use_info, uni_info);
            use_info->parent->use_amount.fetch_sub(use_len, std::memory_order_release);

            (use_info->parent = buffer)->use_amount.fetch_add(use_len, std::memory_order_release);
            linkMemoryBufferUseInfo(use_info, npu_info);


            use_info->info_status.store(info_status, std::memory_order_release);
        }

        use_info->start = idle_info->start;

        //当use_info是MemoryBufferSubstituteInfo时,一定是在同一个MemoryBuffer上移动,所以移动后的idle_info.ipu_info是指向use_info
        if(use_info->delay_len != MEMORY_BUFFER_SUBSTITUTE_INFO_DELAY_LEN) {
            use_info->correlate->setCorrelateBuffer(correlate_func);
        }else{
            use_info = substituteMemoryBufferUseInfo(use_info, idle_info);
        }

        if(move_func != nullptr){
            move_func(use_info->len);
        }


        idle_info->ipu_info = use_info;
    }

    return use_info;
}

/**
 * 将idle_info移动到buf的位置上
 * @param buffer
 * @param idle_info
 * @param pi_info
 * @param buf
 * @return
 */
MemoryBufferIdleInfo* MemoryAdjudicator::memmoveMemoryBufferIdleInfo(MemoryBuffer*, MemoryBufferIdleInfo*, MemoryBufferIdleInfo*, char*){
    //暂时不需要实现
    return nullptr;
}

/**
 * 缩小idle_info的长度
 * @param idle_info         目标idle_info
 * @param pu_info           idle_info的前一个use_info
 * @param len               idle_info缩小的长度
 * @param shrink_func       没有执行移动的回调函数
 * @return
 */
int MemoryAdjudicator::shrinkMemoryBufferIdleInfo(MemoryBufferIdleInfo *idle_info, MemoryBufferUseInfo *pu_info, MemoryBuffer *buffer) {
    int idle_len = idle_info->end - idle_info->start;

    //判断idle_info的长度(end - start)不能满足自身类型长度(sizeof(MemoryBufferIdleInfo)),则需要取消该MemoryBufferIdle
    if(idle_len <= sizeof(MemoryBufferIdleInfo)){
        shrinkMemoryBufferIdleInfo0(idle_info, pu_info, buffer);
    }else{
        idle_len = 0;
    }

    return idle_len;
}

/**
 * 移动idle_info后面的use_info,直到idle_info->next或没有use_info位置
 *  idle_info后面的每个use_info向前移动idle_info的长度
 *  完成后合并idle_info
 *
 * FIXED不能缩,因为USE的use_info.correlate关联了FIXED的内存,如果FIXED移动了内存,则use_info.correlate的内存会错位
 * @param idle_info
 * @param use_info
 * @param pu_info
 * @param buffer
 * @return
 */
void MemoryAdjudicator::shrinkMemoryBufferIdleInfo0(MemoryBufferIdleInfo *idle_info, MemoryBufferUseInfo *pu_info, MemoryBuffer *buffer) {
    MemoryBufferUseInfo *npu_info = pu_info ? pu_info->next : buffer->use_buf_info;
    MemoryBufferIdleInfo *ni_idle = idle_info->next;
    MemoryBufferIdleInfo shrink_idle = *idle_info;


//    4种情况要处理
//    |_idle_|    |_ni_|    -->     (1)
//    0           |_ni_|    -->     (2)
//    |_idle_|    0         -->     (3)
//    0           0         -->     (4)
    if(buffer->sign != MEMORY_BUFFER_SIGN_FIXED){

        if(shrink_idle.end > shrink_idle.start){
            //情况(1) 、 (3) 和 不是FIXED（FIXED不需要移动,只需移除idle_info即可）
            /*
             * 判断use_info, ni_idle,需要往后移动idle_info到尾部或ni_idle前面
             */
            for (;;) {
                if (!npu_info || (ni_idle && (shrink_idle.end >= ni_idle->start))) {
                    break;
                }

                pu_info = memmoveMemoryBufferUseInfo(buffer, npu_info, nullptr, nullptr, &shrink_idle,
                                                     [&](int move_len) -> void {
                                                        shrink_idle.start += move_len;
                                                        shrink_idle.end += move_len;
                                                     });
                npu_info = pu_info->next;
            }
        }

        //移动成功,判断是否需要合并idle_info
        if(isMergeMemoryBufferIdleInfo(ni_idle, &shrink_idle)) {
            mergeMemoryBufferIdleInfo(buffer, ni_idle, &shrink_idle, MEMORY_BUFFER_MERGE_IDLE_INFO_NOTUNLINK);
        }
    }

    //从idle链表移除idle_info
    unlinkMemoryBufferIdleInfo(&shrink_idle, buffer);
}

/**
 * 扩大idle_info的长度或重新回收buffer尾部的空间
 * @param idle_info
 * @param buffer
 * @param target_pos
 * @return
 */
MemoryBufferIdleInfo* MemoryAdjudicator::expandMemoryBufferIdleInfo(MemoryBufferIdleInfo *idle_info, MemoryBuffer *buffer, int target_pos) {
    if((target_pos >= 0) || (target_pos <= buffer->buffer_len)){
        if(target_pos < idle_info->start){
            idle_info->start = target_pos;
        }

        if(target_pos > idle_info->end){
            MemoryBufferIdleInfo *pidle_info = idle_info->prev;
            unlinkMemoryBufferIdleInfo(idle_info, buffer);

            idle_info = initMemoryBufferIdleInfo(buffer, idle_info->ipu_info, idle_info->start, target_pos - idle_info->start);
            linkMemoryBufferIdleInfo(idle_info, pidle_info, buffer);
        }
    }

    return idle_info;
}


//--------------------------------------------------------------------------------------------------------------------//
//----------------------------------------MemoryRecorder--------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

RecordBasic *const MemoryRecorder::records [MEMORY_BUFFER_SIGN_SIZE] =
                  { reinterpret_cast<RecordBasic*const>(&record_external),
                    reinterpret_cast<RecordBasic*const>(&record_idle),
                    reinterpret_cast<RecordBasic*const>(&record_use),
                    reinterpret_cast<RecordBasic*const>(&record_load), reinterpret_cast<RecordBasic*const>(&record_debris),
                    reinterpret_cast<RecordBasic*const>(&record_fixed) };   /* NOLINT */

MemoryRecorder::MemoryRecorder() {
    initMemoryBufferArrayOperatorConcurrent(true);
}

void MemoryRecorder::initMemoryBufferArrayOperatorConcurrent(bool) {
//    不需要实现（UnLockLIst的原因）
//    buffer_array[MEMORY_BUFFER_SIGN_USE].set_operator_concurrent(operator_concurrent);
}

/**
 * 提取负载内存
 * @return
 */
MemoryBufferHolder MemoryRecorder::extractLoadMemoryBuffer() {
    MemoryBufferHolder load_holder = buffer_array[MEMORY_BUFFER_SIGN_LOAD].thread_holder();
    records[MEMORY_BUFFER_SIGN_LOAD]->setRecord(load_holder.onHolderSize(), this, RecordNumber());
    return load_holder;
}


/**
 * 挂载MemoryBufferHead
 *  新增或重新挂载MemoryBuffer
 *  改变sign的MemoryBufferHead
 * unmount的MemoryBufferHead
 * @param memory_buffer
 * @param push              是否插入数组
 */
void MemoryRecorder::mountMemoryBuffer(MemoryBuffer *memory_buffer, bool push) {
    if(push){
        records[memory_buffer->sign]->setRecord(1, this, record_number);
        buffer_array[memory_buffer->sign].push_front(memory_buffer);
    }

    memory_buffer->mount.store(MEMORY_BUFFER_STATUS_MOUNT, std::memory_order_release);
}

/**
 * 卸载MemoryBufferHead
 *  获取MemoryBufferHead的内存
 *  改变sign的MemoryBufferHead
 *  需要整理的MemoryBufferHead
 * @param memory_buffer
 * @param try_size
 * @return
 */
bool MemoryRecorder::unmountMemoryBuffer(MemoryBuffer *memory_buffer, int try_size) {
    bool buffer_status, unmount_res;

    //判断尝试卸载MemoryBuffer的次数
    if(try_size >= std::thread::hardware_concurrency()){
        //直到卸载或被其他线程卸载为止
        unmount_res = true;
        //std::memory_order_acquire可见性,获取挂载的线程修改的数据
        buffer_status = memory_buffer->mount.load(std::memory_order_acquire);
        do{
            if(buffer_status == MEMORY_BUFFER_STATUS_UNMOUNT){
                unmount_res = false;
                break;
            }
        }while(!memory_buffer->mount.compare_exchange_weak(buffer_status, MEMORY_BUFFER_STATUS_UNMOUNT, std::memory_order_release, std::memory_order_acquire));
    }else{
        //直到卸载或用完次数为止
        unmount_res = false;
        for (int i = 0; i < try_size; i++) {
            buffer_status = memory_buffer->mount.load(std::memory_order_acquire);
            if ((buffer_status == MEMORY_BUFFER_STATUS_MOUNT) && memory_buffer->mount.compare_exchange_weak(buffer_status, MEMORY_BUFFER_STATUS_UNMOUNT,
                                                                                                            std::memory_order_release, std::memory_order_relaxed)) {
                unmount_res = true;
                break;
            }
        }
    }

    return unmount_res;
}

/**
 * 查找head_array上可用内存长度>=len的MemoryBufferHead
 * @param sign
 * @param len
 * @param call_func
 * @return
 */
CommonHead* MemoryRecorder::lookupAvailableBuffer(MemoryBufferSign sign, int lookup_len, int lookup_flag, int alloc_buffer_len,
                                                  std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> *lookup_func) {
    LookupInfo lookup_info = LookupInfo(lookup_flag, lookup_len, alloc_buffer_len, 0, sign, lookup_func);

    if((lookup_len > (MEMORY_BUFFER_BIG_MEMORY_FACTOR * alloc_buffer_len)) && (sign != MEMORY_BUFFER_SIGN_FIXED)){
        //设置为搜索大内存
        lookup_info.search_flag = set_flag(lookup_info.search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER);
        lookup_info.unmount_value = INT32_MAX;
    }

    if(sign != MEMORY_BUFFER_SIGN_FIXED){
        MemoryBufferHolder load_holder;
        return lookupAvailableBuffer0(&lookup_info, &load_holder);
    }else{
        return lookupAvailableBuffer0(&lookup_info, nullptr);
    }

}

/**
 * 实现函数
 * @param lookup_info
 * @param load_buffers MEMORY_BUFFER_SIGN_FIXED不需要整理内存,也不需要移动内存,所以load_buffers == nullptr
 * @return
 */
CommonHead* MemoryRecorder::lookupAvailableBuffer0(LookupInfo *lookup_info, MemoryBufferHolder *load_holder) {
    int unmount_res = 0;
    //可用内存
    CommonHead *lookup_common = nullptr;
    MemoryBufferIterator lookup_iterator;
    MemoryBufferIterator beginp, endp;

    /*
     * 负载函数
     */
    auto factor_func = [&]() -> bool {
        bool push_load = false;

        //如果MemoryBuffer为MEMORY_BUFFER_SIGN_FIXED 或 MemoryBuffer转换为MEMORY_BUFFER_SIGN_LOAD状态
        if(((*lookup_iterator)->sign == MEMORY_BUFFER_SIGN_FIXED) ||
           (push_load = (((*lookup_iterator)->sign = memoryBufferUseStatus(*lookup_iterator)) == MEMORY_BUFFER_SIGN_LOAD))){
            //调用消耗MemoryBuffer函数
            onMemoryBufferExhaust(*lookup_iterator);
        }

        if(load_holder && push_load){
            //移除lookup_buffer;
            records[lookup_info->sign]->setRecord(-1, this, record_number);
            try {
                buffer_array[lookup_info->sign].erase(beginp);
            }catch (MemoryBufferEraseFinal &e){
                //删除成功，插入到负载MemoryBuffer,等待处理
                (*load_holder) << lookup_iterator;
                //load_buffers->push_front(lookup_buffer);
            }
        }

        return push_load;
    };

    //判断MemoryBuffer的空闲内存长度是否满载搜索长度
    auto check_len = [&](MemoryBuffer *check_buf) -> bool {
        int remain_len = check_buf->buffer_len - check_buf->use_amount.load(std::memory_order_consume);
        return (remain_len >= lookup_info->lookup_len);
    };

    //搜索挂载的所有MEMORY_BUFFER_SIGN_USE内存
    for(int satisfy_buffer = 0; !buffer_array[lookup_info->sign].empty(); satisfy_buffer = 0){
restart:
        beginp = buffer_array[lookup_info->sign].begin(), endp = buffer_array[lookup_info->sign].end();

        for(; beginp != endp; ++beginp){
            records[lookup_info->sign]->setRecord(1, this, record_call_size);
            lookup_iterator = beginp;

            //增加卸载次数
            if(!get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)){
                (lookup_info->unmount_value *= 2)++;
            }

            //判断该MemoryBuffer的空闲内存长度是否满载搜索长度
            if(check_len(*lookup_iterator)){
                //增加满足数量
                satisfy_buffer++;
                //卸载MemoryBuffer状态
                unmount_res = unmountMemoryBuffer(*lookup_iterator, lookup_info->unmount_value);
                //判断是否卸载成功
                if(unmount_res){
                    records[lookup_info->sign]->setRecord(1, this, record_occupy_size);

                    //判断满载搜索长度及是否搜索到可用内存(再次检查,因为可能在第一次检查和卸载之间被其他线程申请内存而导致内存长度不够)
                    if(check_len(*lookup_iterator) && (lookup_common = onLookupMemoryBuffer(*lookup_iterator, lookup_info->lookup_func,
                                                                                         nullptr, nullptr, lookup_info->lookup_len, lookup_info->search_flag))){
                        //获取到可用内存,重新挂载状态,结束循环
                        mountMemoryBuffer(*lookup_iterator, false);
                        records[lookup_info->sign]->setRecord(1, this, record_success_size);
                        break;
                    }else{
                        //搜索不到,减少满载数量
                        satisfy_buffer--;
                        //判断非大内存及该MemoryBuffer转换为MEMORY_BUFFER_SIGN_LOAD状态
                        if(!get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER) && factor_func()){
                            //重新搜索
                            goto restart;
                        }
                        //重新挂载状态
                        mountMemoryBuffer(*lookup_iterator, false);
                    }
                }else{
                    records[lookup_info->sign]->setRecord(1, this, record_unoccupy_size);
                }
            }else{
                //不满载搜索长度及非大内存
                if(!get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)) {
                    //调用消耗MemoryBuffer函数
                    onMemoryBufferExhaust(*lookup_iterator);
                }
            }
        }

        //搜索到可用内存,退出函数
        if(lookup_common){
            break;
        }

        /*
         * USE的内存里的空闲内存总长度不满载len或搜索不到长度为len的空闲内存
         */
        if(satisfy_buffer <= 0){
            //从LOAD链表中搜索到可用内存(返回)或小内存,结束循环(返回申请内存)
            if((lookup_common = lookupAvailableBufferOnLoad(lookup_info, load_holder)) || !get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)){
                break;
            }

            if(get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)) {
                //大内存,强制申请内存,从新开始
                onForceCreateMemoryBuffer();
            }else{
                break;
            }
        }
    }

    /**
     * 需要修理内存且LOAD内存链表有MemoryBuffer
     */
    if(load_holder && (load_holder->onHolderSize() > 0)){
        treatmentLoadMemoryBuffer(*load_holder, lookup_info->load_use_len, lookup_info->load_use_size, lookup_info->load_buffer_len,
                                  lookup_info->load_buffer_size, lookup_info->alloc_buffer_len);
    }
    return lookup_common;
}

/**
 * 搜索LOAD链表
 * @param lookup_info
 * @param load_buffers
 * @return
 */
CommonHead* MemoryRecorder::lookupAvailableBufferOnLoad(LookupInfo *lookup_info, MemoryBufferHolder *load_buffers) {
    int lookup_use_len = 0, lookup_use_size = 0;
    //可用内存
    CommonHead *lookup_common = nullptr;

    //需要搜索LOAD内存且需要修理内存
    if(lookup_info->search_load_buffer && load_buffers){
        //加载LOAD内存链表
//        MemoryBufferHolder load_sign_buffers = onRequestLoadMemoryBuffer() + *load_buffers;
        (*load_buffers) += onRequestLoadMemoryBuffer();
//      if (!load_sign_buffers.empty()) {
//          //将加载的LOAD内存链表合并到新增的LOAD内存链表里
//          load_buffers->merge(load_sign_buffers);
//      }

        load_buffers->onHolderNote([&](MemoryBufferIterator iterator) -> bool {
            bool on_lookup = false;
            if(!lookup_common && (lookup_common = onLookupMemoryBuffer(*iterator, lookup_info->lookup_func, &lookup_use_len, &lookup_use_size,
                                                                       lookup_info->lookup_len, set_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_NOSIZE_UNBREAK)))){
                //设置搜索到可用内存的MemoryBuffer
                mountMemoryBuffer(*iterator, true);
                on_lookup = true;
            }else{
                //设置use_info记录
                if(!lookup_common){
                    //没有可用内存,说明已经调用了onLookupMemoryBuffer函数(即lookup_use_len、lookup_use_size为有效数据),但没有搜索到可用内存
                    lookup_info->load_use_len += lookup_use_len;
                    lookup_info->load_use_size += lookup_use_size;
                }else{
                    //已经搜索到可用内存,没有调用onLookupMemoryBuffer函数(即lookup_use_len、lookup_use_size为无效数据)
                    lookup_info->load_use_len += (*iterator)->use_amount.load(std::memory_order_consume);
                    lookup_info->load_use_size += (*iterator)->use_info_size;
                }

                //设置MemoryBuffer记录
                lookup_info->load_buffer_len += (*iterator)->buffer_len;
                lookup_info->load_buffer_size++;
            }

            lookup_use_len = 0;
            lookup_use_size = 0;
            return on_lookup;
        });

        //搜索到满足长度为len的MemoryBuffer迭代器
//        MemoryBufferIterator lookup_iterator = load_buffers->end();
//        for(auto beginp = load_buffers->begin(), endp = load_buffers->end(); beginp != endp; ++beginp, lookup_use_len = 0, lookup_use_size = 0){
//
//        }

        //搜索到内存
//        if(lookup_iterator != load_buffers->end()){
//            //将搜索到满足长度为len的MemoryBuffer从LOAD内存链表里移除
//            load_buffers->erase(lookup_iterator);
//            //从新挂载
//            mountMemoryBuffer(*lookup_iterator, true);
//        }

        lookup_info->search_load_buffer = false;
    }


    return lookup_common;
}

/**
 * 遍历MemoryBuffer的use_info和idle_info（假设该MemoryBuffer已经被卸载）
 * @param buffer
 */
void MemoryRecorder::ergodicMemoryBufferInfo(MemoryBuffer *buffer) {
    int msize = buffer->use_info_size + buffer->idle_info_size;
    int use_size = 0, idle_size = 0, value_ = 0, use_value = 0;
    MemoryBufferUseInfo *use_info = buffer->use_buf_info, *pu_info = nullptr;
    MemoryBufferIdleInfo *idle_info = buffer->idle_buf_info;

    std::cout << "id(" << std::this_thread::get_id() << "):ergodicInfo:(" << buffer << "," << buffer->mount.load(std::memory_order_acquire) << ")->";
    for(;;){
        --msize;
        if(!idle_info || (use_info && (use_info->start < idle_info->start))){
            std::cout << std::dec << "use_info(" << (CommonHead*)(buffer->buffer + use_info->start) << ","
                      << use_info->start << "," << use_info->start + use_info->len << ") : ";

            if((buffer->sign != MEMORY_BUFFER_SIGN_FIXED) && (value_ != use_info->start)){
                std::cout << "?" << std::endl;
            }
            value_ = (getMemoryBufferUseLen(reinterpret_cast<char*>(use_info)) + use_info->start);
            use_value += (use_info->len);

            use_size++;
            pu_info = use_info;
            use_info = use_info->next;
        }else{
            std::cout << std::dec << "idle_info(" << reinterpret_cast<CommonHead*>(buffer->buffer + idle_info->start) << "," << idle_info->start << "," << idle_info->end << ") : ";

            if((buffer->sign != MEMORY_BUFFER_SIGN_FIXED) && ((value_ != idle_info->start))){
                std::cout << "?" << std::endl;
            }
            if((buffer->sign != MEMORY_BUFFER_SIGN_FIXED) && pu_info && (idle_info->ipu_info != pu_info)){
                std::cout << "?" << std::endl;
            }else{
                pu_info = nullptr;
            }

            value_ = idle_info->end;

            idle_size++;
            idle_info = idle_info->next;
        }
        if(!msize){
            int buse_value = buffer->use_amount.load(std::memory_order_consume);
            std::cout << std::endl;
            std::cout << "use_value->" << use_value << "," << buse_value << std::endl;
            if((buse_value < 0) || (buse_value > use_value) || (use_size != buffer->use_info_size) || (idle_size != buffer->idle_info_size)){
                std::cout << "?" << std::endl;
            }
            break;
        }
    }
}

/**
 * 遍历调用记录
 */
void MemoryRecorder::ergodicMemoryBufferRecorder() {
    for(int i = MEMORY_BUFFER_SIGN_EXTERNAL; i < MEMORY_BUFFER_SIGN_SIZE; i++){
        switch (i){
            case MEMORY_BUFFER_SIGN_EXTERNAL:   std::cout << "EXTERNAL : "; break;
            case MEMORY_BUFFER_SIGN_IDLE:       std::cout << "IDLE : ";     break;
            case MEMORY_BUFFER_SIGN_USE:        std::cout << "USE : ";      break;
            case MEMORY_BUFFER_SIGN_LOAD:       std::cout << "LOAD : ";     break;
            case MEMORY_BUFFER_SIGN_DEBRIS:     std::cout << "DEBRIS : ";   break;
            case MEMORY_BUFFER_SIGN_FIXED:      std::cout << "FIXED : ";    break;
            default:                            std::cout << "DEFAULT :";   break;
        }
        std::cout << std::endl;

        std::cout << "size(" << records[i]->getRecord(this, record_size) << ")" << std::endl;
        std::cout << "number(" << records[i]->getRecord(this, record_number) << ")" << std::endl;
        std::cout << "call_size(" << records[i]->getRecord(this, record_call_size) << ")" << std::endl;
        std::cout << "success_size(" << records[i]->getRecord(this, record_success_size) << ")" << std::endl;
        std::cout << "occupy_size(" << records[i]->getRecord(this, record_occupy_size) << ")" << std::endl;
        std::cout << "unoccupy_size(" << records[i]->getRecord(this, record_unoccupy_size) << ")" << std::endl;

        if(!buffer_array[i].empty()){
            for(auto begin : buffer_array[i]){
                ergodicMemoryBufferInfo(begin);
            }
        }

    }
}

/**
 * MemoryBuffer已经准备就绪(即从Idle转换为Use)
 */
void MemoryRecorder::onMemoryBufferReady() {
    MemoryBufferSign sign = MEMORY_BUFFER_SIGN_IDLE;
    //移除MemoryBuffer
    MemoryBufferList::clear(&buffer_array[sign],
                            [&](decltype(buffer_array[MEMORY_BUFFER_SIGN_IDLE].begin()) iterator) -> void {
                                records[sign]->setRecord(-1, this, record_number);
                                //更新MemoryBuffer的状态
                                (*iterator)->sign = memoryBufferUseStatus(*iterator);
                                //插入并挂载MemoryBuffer
                                mountMemoryBuffer(*iterator, true);
                            });
}

/**
 * MemoryBuffer已经使用完毕（即从Load或Use转换为IDLE）
 * @param buffers
 */
void MemoryRecorder::onMemoryBufferUnReady(MemoryBufferSign sign) {
    onMemoryBufferUnReady0(&buffer_array[sign]);
}

void MemoryRecorder::onMemoryBufferUnReady0(MemoryBufferList *buffers) {
    //移除MemoryBuffer(MemoryManager正在处于销毁阶段)
    MemoryBufferList::clear(buffers,
                            [&](decltype(buffers->begin()) iterator) -> void {
                                records[(*iterator)->sign]->setRecord(-1, this, record_number);
                                //设置为IDLE的状态
                                (*iterator)->sign = MEMORY_BUFFER_SIGN_IDLE;
                                //插入并挂载MemoryBuffer
                                mountMemoryBuffer(*iterator, true);
                            });
}

/**
 * 弹出状态为sign的MemoryBuffer
 * @param sign
 * @param pop_func
 */
void MemoryRecorder::onPopMemoryBuffer(MemoryBufferSign sign, std::function<void(MemoryBuffer *)> pop_func) {
    MemoryBufferList::clear(&buffer_array[sign],
                            [&](decltype(buffer_array[sign].begin()) iterator) -> void {
                                records[sign]->setRecord(-1, this, record_number);
                                if(pop_func != nullptr) pop_func(*iterator);
                            });
}

//--------------------------------------------------------------------------------------------------------------------//
//----------------------------------------MemoryManager---------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//


MemoryManager::MemoryManager(int fixed_init_size, uint32_t fixed_buffer_len, uint32_t idle_buffer_len) throw(std::bad_alloc)
        : memory_fixed_buffer_len(fixed_buffer_len), memory_idle_buffer_len(idle_buffer_len){
    makeMemoryBuffer(MEMORY_BUFFER_THREAD_PERMIT_CREATE_APPLY, fixed_init_size, &fixed_info,
                     std::bind((init_func)&MemoryManager::initFixedBuffer, this, std::placeholders::_1));
    makeMemoryBuffer(MEMORY_BUFFER_THREAD_PERMIT_CREATE_APPLY, MEMORY_BUFFER_SIGN_IDLE_INIT_SIZE, &idle_info,
                     std::bind((init_func)&MemoryManager::initIdleBuffer, this, std::placeholders::_1));
}

MemoryManager::~MemoryManager() {
    uinitIdleBuffer();
    uinitFixedBuffer();
}

/**
 * 获取内存
 * @param head
 * @return
 */
MemoryCorrelate* MemoryManager::obtainBuffer(uint32_t obtain_len) {
    char *obtain_buf = nullptr;
    MemoryBufferUseInfo *use_info = nullptr;
    MemoryCorrelate *correlate = nullptr;

    //先从MEMORY_BUFFER_SIGN_FIXED内存里获取MemoryCorrelate和MemoryBufferUseInfo的内存
    if((correlate = (reinterpret_cast<MemoryCorrelate*>(makeInsideMemoryBuffer(memory_buffer_correlate_use, MEMORY_BUFFER_FLAG_MAKE_SPACE_HEAD | MEMORY_BUFFER_FLAG_INIT_CORRELATE)->buf)))){
        //获取use_info
        use_info = getMemoryBufferUseInCorrelate(correlate);

        typedef MemoryBufferUseInfo* (*init_use_func)(MemoryAdjudicator*, MemoryBufferUseInfo*, MemoryCorrelate*, MemoryBuffer*, int, int, int);
        std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> lookup_func = std::bind(reinterpret_cast<init_use_func>(&MemoryAdjudicator::initMemoryBufferUseInfo),
                                                                                                  dynamic_cast<MemoryAdjudicator*>(this), use_info, correlate, std::placeholders::_1,
                                                                                                  std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

        for(;;){
            //搜索内存
            if((obtain_buf = reinterpret_cast<char*>(lookupAvailableBuffer(MEMORY_BUFFER_SIGN_USE, obtain_len, MEMORY_BUFFER_FLAG_NONE, allocIdleBufferLen(), &lookup_func)))){
                //设置关联内存
                correlate->setCorrelateBuffer0(obtain_buf);
                break;
            }else{
                //内存不足,申请内存
                makeMemoryBuffer(MEMORY_BUFFER_THREAD_PERMIT_CREATE_APPLY, MEMORY_BUFFER_SIGN_DEFAULT_INIT_SIZE, &idle_info,
                                 std::bind((init_func)&MemoryManager::initIdleBuffer, this, std::placeholders::_1));
            }
        }
    }

    return correlate;
}


/**
 * 释放内存
 * @param rp common->buf
 * @param rh 内存头部信息
 */
void MemoryManager::returnBuffer(MemoryCorrelate *correlate){ //char *rp, const SpaceHead &rh) {
//    {
//        int buffer_len = 0;
        //获取使用权,及关联器关联的内存,防止该内存被修改
//        char *rb = correlate->getCorrelateBuffer();
//        SpaceHead rh;
//        if (rb) {
//            releaseMemoryBuffer(rb, getMemoryBufferUseInfo(backCommonHead(rb), (buffer_len = reductionBufSize((rh = extractSpaceHead(rb))))), rh, buffer_len);
//        }
    MemoryBufferUseInfo *release_use = nullptr;
    if((release_use = getMemoryBufferUseInCorrelate(correlate))){
        releaseMemoryBuffer(release_use);
    }
//        //释放使用权
//        correlate->releaseCorrelateBuffer();
//    }
//    releaseMemoryCorrelate(correlate);
}

/**
 * 释放MemoryBufferUseInfo
 * @param rp
 * @param use_info      MemoryBuffer使用信息
 * @param space_head    内存头部信息
 * @param buffer_len    内存长度
 */
void MemoryManager::releaseMemoryBuffer(MemoryBufferUseInfo *use_info){//(char *rp, MemoryBufferUseInfo *use_info, const SpaceHead &space_head, int buffer_len) {
    int use_len = getMemoryBufferUseLen(reinterpret_cast<char*>(use_info)),
        info_status = MEMORY_BUFFER_USE_INFO_STATUS_LIVE;
    MemoryBuffer *parent = use_info->parent;

//    {
//        SpaceHead head = space_head;
//        //设置释放点
//        head.control = 0;
//        //清空内存头部信息
//        clearSpaceHead(rp);
//        //设置内存头部信息
//        makeSpaceHead(rp, head);
//    }


    while(!use_info->info_status.compare_exchange_weak(info_status, MEMORY_BUFFER_USE_INFO_STATUS_DEATH, std::memory_order_release, std::memory_order_acquire)){
        parent = use_info->parent;
        info_status = MEMORY_BUFFER_USE_INFO_STATUS_LIVE;
    }

    //减少MemoryBuffer的使用内存长度
    parent->use_amount.fetch_sub(use_len, std::memory_order_release);
}

/**
 * 释放MemoryCorrelate（释放提供该内存的FIXED的使用内存）,FIXED不需要获取关联器
 * @param correlate
 */
void MemoryManager::onReleaseMemoryCorrelate(MemoryCorrelate *correlate) {
    int buffer_len = 0;
    //获取内存头部信息
    SpaceHead correlate_head = extractSpaceHead(reinterpret_cast<char*>(correlate));
    MemoryBuffer *parent = nullptr;
    MemoryBufferUseInfo *use_info = nullptr;

    if(correlate){
        //获取MemoryBufferUseInfo
        use_info = getMemoryBufferUseInfo(backCommonHead(reinterpret_cast<char*>(correlate)), (buffer_len = reductionBufSize(correlate_head)));
        parent = use_info->parent;

        //设置释放点
//        correlate_head.control = 0;
//        //清空内存头部信息
//        clearSpaceHead((char*)correlate);
        //设置内存头部信息
//        makeSpaceHead((char*)correlate, correlate_head);
        use_info->info_status.store(MEMORY_BUFFER_USE_INFO_STATUS_DEATH, std::memory_order_release);

//      减少提供MemoryCorrelate的Fixed的使用内存长度
        parent->use_amount.fetch_sub(buffer_len, std::memory_order_release);
    }

}


/**
 * 构造并初始化FixedBuffer
 * @param fixed_size FixedBuffer的长度（构造内存的长度）
 *
 * |__SpaceHead__|__MemoryBuffer__|__MemoryBufferNote(UnLockList)__|___buffer___| ==> fixed_size
 *
 */
void MemoryManager::initFixedBuffer(int fixed_size)  throw(std::bad_alloc){
    int fixed_hlen = sizeof(MemoryBuffer), fixed_nlen = sizeof(MemoryBufferNote);
    MemoryBuffer *memory_buffer = nullptr;

    InitMemoryBuffer buffers[fixed_size];
    //构造fixed_size长度的内存
    try {
        if ((fixed_size = initMemoryBuffer(fixed_size, allocFixedBufferLen(), buffers)) > 0) {
            for (int i = 0; i < fixed_size; i++) {
                //初始化FixedBuffer
                memory_buffer = initMemoryBufferHead(reinterpret_cast<MemoryBuffer*>(buffers[i].common->buf),
                                                     buffers[i].common->buf + fixed_hlen + fixed_nlen,
                                                     buffers[i].len - fixed_hlen - fixed_nlen,
                                                     MEMORY_BUFFER_SIGN_FIXED);
                //插入并挂载FixedBuffer
                mountMemoryBuffer(memory_buffer, true);

                //回调
                onMakeMemoryBuffer(memory_buffer, &fixed_info);
            }
        }
    }catch (std::bad_alloc &e){
        std::cout << "initFixedBuffer->(" << e.what() << ")" << std::endl;
        throw;
    }
}

/**
 * 构造并初始化IdleBuffer
 * @param idle_size IdleBuffer的长度(构造内存的长度)
 *
 * |__SpaceHead__|____buffer____| ==> idle_size
 */
void MemoryManager::initIdleBuffer(int idle_size) throw(std::bad_alloc) {
    int make_pos = 0;
    MemoryBuffer *memory_buffer = nullptr;

    if((idle_size <= 0) || (getRecordInfo(MEMORY_BUFFER_SIGN_IDLE, RecordNumber()) <= 0)){
        idle_size = MEMORY_BUFFER_SIGN_IDLE_INIT_SIZE;
    }

    InitMemoryBuffer buffers[idle_size];
    try {
        //构造idle_size长度的内存
        if ((idle_size = initMemoryBuffer(idle_size, allocIdleBufferLen(), buffers)) > 0) {
            try {
                for (int i = 0; i < idle_size; i++, make_pos++) {
                    //初始化IdleBuffer(从FixedBuffer中提取并初始化memory_buffer，并关联该内存)
                    memory_buffer = makeMemoryBufferHead(buffers[i].common->buf, buffers[i].len,
                                                         MEMORY_BUFFER_SIGN_IDLE);
                    //插入并挂载IdleBuffer
                    mountMemoryBuffer(memory_buffer, true);

                    //回调
                    onMakeMemoryBuffer(memory_buffer, &idle_info);

                }
            } catch (std::bad_alloc &e) {
                //从FixedBuffer中提取memory_buffer失败
                std::cout << "initIdleBuffer->(" << e.what() << ")" << std::endl;

                for (; make_pos < idle_size; make_pos++) {
                    //销毁内存
                    uinitMemoryBuffer(buffers[make_pos]);
                }

                //记录信息
                if (getRecordInfo(MEMORY_BUFFER_SIGN_IDLE, RecordNumber()) <= 0) {
                    throw;
                }
            }
        }
    }catch (std::bad_alloc &e){
        std::cout << "initIdleBuffer->(" << e.what() << ")" << std::endl;
    }

    //将IDLE状态的MemoryBuffer转换为MEMORY_BUFFER_SIGN_USE状态
    if(getRecordInfo(MEMORY_BUFFER_SIGN_IDLE, RecordNumber()) > 0){
        onMemoryBufferReady();
    }
}

/**
 * 强制创建内存
 */
void MemoryManager::onForceCreateMemoryBuffer() {
    makeMemoryBuffer(MEMORY_BUFFER_THREAD_PERMIT_CREATE_FROCE, MEMORY_BUFFER_SIGN_DEFAULT_INIT_SIZE, &idle_info,
                     std::bind((init_func)&MemoryManager::initIdleBuffer, this, std::placeholders::_1));
}

/**
 * 调用初始化内存函数
 * @param init_
 * @param init_size
 */
void MemoryManager::onInitMemoryBuffer(std::function<void(int)> &init_, int init_size) {
    if((init_ != nullptr) && (init_size > 0)){
        init_(init_size);
    }
}

/**
 * 消耗MemoryBuffer
 * @param memory_buffer
 */
void MemoryManager::onMemoryBufferExhaust(MemoryBuffer *memory_buffer) {
    typedef void (*call_func)(MemoryProducer*, MemoryBuffer*, PermitInfo*);

    switch (memory_buffer->sign) {
        case MEMORY_BUFFER_SIGN_FIXED:
            std::call_once(memory_buffer->exhaust_flag, (call_func)&MemoryProducer::onConsumeMemoryBuffer, dynamic_cast<MemoryProducer*>(this), memory_buffer, &fixed_info);
            break;
        case MEMORY_BUFFER_SIGN_IDLE:
        case MEMORY_BUFFER_SIGN_USE:
        case MEMORY_BUFFER_SIGN_LOAD:
            std::call_once(memory_buffer->exhaust_flag, (call_func)&MemoryProducer::onConsumeMemoryBuffer, dynamic_cast<MemoryProducer*>(this), memory_buffer, &idle_info);
            break;
        default:
            break;
    }
}

/**
 * 销毁已经申请的Fixed内存
 */
void MemoryManager::uinitFixedBuffer() {
    onPopMemoryBuffer(MEMORY_BUFFER_SIGN_FIXED, [&](MemoryBuffer *buffer){
        this->uinitMemoryBuffer((CommonHead*)backCommonHead((char*)buffer));
    });
}

/**
 * 销毁已经申请的Idle内存
 */
void MemoryManager::uinitIdleBuffer() {
    //先转换没有合并的负载的MemoryBuffer
    onMemoryBufferUnReady(MEMORY_BUFFER_SIGN_LOAD);
    //判断是否还有正在合并的负载的MEmoryBuffer
    while(getRecordInfo(MEMORY_BUFFER_SIGN_LOAD, RecordNumber()) > 0);
    //再转换剩下的MemoryBuffer
    onMemoryBufferUnReady(MEMORY_BUFFER_SIGN_LOAD);
    //最后转换USE的MemoryBuffer
    onMemoryBufferUnReady(MEMORY_BUFFER_SIGN_USE);

    onPopMemoryBuffer(MEMORY_BUFFER_SIGN_IDLE, [&]( MemoryBuffer *buffer){
        this->uinitMemoryBuffer((CommonHead*)backCommonHead(buffer->buffer));
    });
}


/**
 * 初始化FixedBuffer和IdleBuffer的MemoryBuffer和MemoryBufferNote(UnLockList)
 * @param buffer
 * @param buf
 * @param len
 * @param sign
 * @return
 */
MemoryBuffer* MemoryManager::initMemoryBufferHead(MemoryBuffer *buffer, char *buf, int len, MemoryBufferSign sign) {
    //初始化MemoryBuffer
    memory_alloc.construct(buffer, sign, len, buf);
    //初始化MemoryBufferNote
    memory_alloc.construct<MemoryBufferNote>(MemoryBufferNote::createNote(buffer), buffer);

    //初始化Memory的MemoryBufferIdleInfo并链接
    linkMemoryBufferIdleInfo(initMemoryBufferIdleInfo(buffer, nullptr, 0, len), nullptr, buffer);

    return buffer;
}

/**
 * 构造MemoryBuffer
 *  FixedBuffer : 初始化MemoryBuffer
 *  IdleBuffer  : 从FixedBuffer中提取MemoryBuffer
 * @param buf   内存(SpaceHead之后的内存)
 * @param len   内存长度(SpaceHead之后的内存长度)
 * @param sign  标记（Fixed或Idle）
 * @return
 */
MemoryBuffer* MemoryManager::makeMemoryBufferHead(char *buf, int len, MemoryBufferSign sign) {
    return initMemoryBufferHead(reinterpret_cast<MemoryBuffer*>(makeInsideMemoryBuffer(memory_buffer_head, MEMORY_BUFFER_FLAG_MAKE_SPACE_HEAD)->buf), buf, len, sign);
}

/**
 * 初始化MemoryCorrelate
 * @param correlate
 * @param common_head 关联的内存
 * @return
 */
void MemoryManager::initMemoryCorrelate(MemoryCorrelate *correlate, char *buf) {
    memory_alloc.construct(correlate, buf);
}

/**
 * 构造内部内存(IdleBuffer的Memory和MemoryCorrelate)
 * @param head
 * @return
 */
CommonHead* MemoryManager::makeInsideMemoryBuffer(const SpaceHead &head, int flag) {
    //获取内存长度
    const int memory_len = reductionBufSize(head);
    CommonHead *common_head = nullptr;

    //搜索空闲内存满足memory_len的FixedBuffer
    for(;;) {
        if((common_head = lookupAvailableBuffer(MEMORY_BUFFER_SIGN_FIXED, memory_len, flag, allocFixedBufferLen(), nullptr))) {
            //makeSpaceHead需不需要给其他线程可见先？？？(不需要,因为已经对该common_head的use_info有可见性)
            makeSpaceHead(common_head->buf, head);
            break;
        } else {
            //获取失败
            //构造FixedBuffer
            makeMemoryBuffer(MEMORY_BUFFER_THREAD_PERMIT_CREATE_APPLY, MEMORY_BUFFER_SIGN_DEFAULT_INIT_SIZE, &fixed_info,
                             std::bind((init_func) &MemoryManager::initFixedBuffer, this, std::placeholders::_1));
        }
    }

    return common_head;
}

void MemoryManager::onConstructMemoryBuffer(CommonHead *common_head, int buffer_flag) {
    if(get_flag(buffer_flag, MEMORY_BUFFER_FLAG_MAKE_SPACE_HEAD)){
        //捷径
        if(get_flag(buffer_flag, MEMORY_BUFFER_FLAG_INIT_CORRELATE)){
            makeSpaceHead(common_head->buf, memory_buffer_correlate_use);
            initMemoryCorrelate(reinterpret_cast<MemoryCorrelate*>(common_head->buf), nullptr);
        }else{
            makeSpaceHead(common_head->buf, memory_buffer_head);
        }
    }
}

CommonHead* MemoryManager::onLookupMemoryBuffer(MemoryBuffer *lookup_buffer, std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> *lookup_func,
                                                int *lookup_use_len, int *lookup_use_size, int search_len, int buffer_flag) {
    std::function<void(MemoryBufferUseInfo*)> lookup_use_func = [&](MemoryBufferUseInfo *use_info) -> void {
                                                                    if(lookup_use_len) { (*lookup_use_len) += use_info->len; }
                                                                    if(lookup_use_size){ (*lookup_use_size)++; }
                                                                };
    return searchAvailableBuffer(lookup_buffer, lookup_func, &lookup_use_func, nullptr, search_len, buffer_flag);
}

/**
 * 请求负载内存
 * @return
 */
MemoryBufferHolder MemoryManager::onRequestLoadMemoryBuffer() {
    //获取并返回负载内存
    return extractLoadMemoryBuffer();
}

/**
 * 响应内存链表
 * @param load_buffers
 * @return
 */
void MemoryManager::onResponseMemoryBuffer(MemoryBufferHolder &load_holder) {
    //清空链表
//    MemoryBufferList::clear(&load_buffers,
//                            [&](MemoryBufferIterator iterator) -> void {
//                                responseLoadMemoryBuffer(*iterator);
//                            });
    load_holder.onHolderNote([&](MemoryBufferIterator iterator) -> bool {
        responseLoadMemoryBuffer(*iterator); return false;
    });
}

/**
 * 响应负载内存(重新挂载)
 * @param load_buffer
 */
void MemoryManager::responseLoadMemoryBuffer(MemoryBuffer *load_buffer) {
    //更新MemoryBuffer的状态
    load_buffer->sign = memoryBufferUseStatus(load_buffer);
    //插入并挂载MemoryBuffer
    mountMemoryBuffer(load_buffer, true);
}

