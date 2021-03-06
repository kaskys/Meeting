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
 * ??????MemoryBuffer
 * @param froce
 * @param msize
 * @param permit_info
 * @param init_
 */
void MemoryProducer::makeMemoryBuffer(bool force, int msize, PermitInfo *permit_info, std::function<void(int)> init_) {
    int (*func_)(MemoryProducer*, std::atomic_int&) = nullptr;
    //??????????????????
    if(force){
        //??????MemoryBuffer
        func_ = reinterpret_cast<int(*)(MemoryProducer*, std::atomic_int&)>(&MemoryProducer::createForce);
    }else{
        //????????????MemoryBuffer
        func_ = reinterpret_cast<int(*)(MemoryProducer*, std::atomic_int&)>(&MemoryProducer::createApply);
    }

    //??????????????????
    if((*func_)(this, permit_info->status)){
        //?????????MemoryBuffer
        onInitMemoryBuffer(init_, msize);
        //????????????MemoryBuffer
        doneMemoryBuffer(permit_info);
    }
}

/**
 * ????????????MemoryBuffer
 * @param permit_info
 */
void MemoryProducer::doneMemoryBuffer(PermitInfo *permit_info) {
    int status_ = MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST;

    //????????????
    while(!permit_info->status.compare_exchange_weak(status_, MEMORY_BUFFER_THREAD_PERMIT_STATUS_DONE, std::memory_order_release, std::memory_order_consume)){
        if(status_ != MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST){
            break;
        }
    }
}

/**
 * ????????????MemoryBuffer
 * @param permit_status
 * @return
 */
int MemoryProducer::createForce(std::atomic_int &permit_status){
    int status_ = permit_status.load(std::memory_order_consume);

    switch (status_){
        case MEMORY_BUFFER_THREAD_PERMIT_STATUS_DONE:
            //????????????
            waitCreate(permit_status);
        case MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT:
            //??????MemoryBuffer
            return createApply(permit_status);
        case MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST:
        default:
            return 0;
    }
}

/**
 * ??????MemoryBuffer
 * @param permit_status
 * @return
 */
int MemoryProducer::createApply(std::atomic_int &permit_status) {
    int status_ = MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT;

    //????????????
    while(!permit_status.compare_exchange_weak(status_, MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST, std::memory_order_release, std::memory_order_consume)){
        if(status_ != MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT){
            break;
        }
    }

    return (status_ == MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT) ? 1 : 0;
}

/**
 * ??????????????????
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
 * ????????????MemoryBuffer
 * @param buffer
 * @param permit_info
 */
void MemoryProducer::onMakeMemoryBuffer(MemoryBuffer*, PermitInfo *permit_info) {
    permit_info->permit.fetch_add(MEMORY_BUFFER_THREAD_PERMIT_CHANGE_VALUE, std::memory_order_release);
}

/**
 * ????????????MemoryBuffer
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
 * ????????????
 * @param size  MemoryBuffer?????????
 * @param len   ???????????????
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
            //??????len???????????????
            buffers[i].common = reinterpret_cast<CommonHead*>(createMemoryBuffer(len));
            memset(buffers[i].common, 0, static_cast<size_t>(len));
        }catch (std::bad_alloc &e){
            //?????????????????????,????????????
            if(!cinit_size){
                throw;
            }
            //?????????????????????????????????????????????????????????
            break;
        }

        if(buffers[i].common) {
            //?????????SpaceHead
            makeSpaceHead(buffers[i].common->buf, space_head);
        }
    }

    //??????????????????????????????
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
 * ??????MemoryBuffer?????????
 * @param size
 * @return
 */
char* MemoryProducer::createMemoryBuffer(int size) {
    return memory_alloc.allocator<char>(static_cast<uint32_t>(size));
}

/**
 * ??????MemoryBuffer?????????
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
 * ???MemoryBufferHead???sign???????????????
 * (1) ???c.next ==> prev
 * (2) prev ???????????? c, next  ==> ????????????
 * (3)???????????? b ?????????????????????, ????????????prev(prev.next.load),????????? a
 * (4)???????????????(2)?????????(?????? b ?????????????????????, a.next ?????????c,???(2)??????????????????)
 *
 *          |=h=|
 *            |
 *            |
 *            ???
 *       -- |=a=|---------------
 *       |    |                |
 *       |    |                |
 *   (3) |    |                |
 *       |    ???                |
 *   -----  |=b=|   <----      |(4)
 *   |   |    |         |      |
 *   |   |    |         |      |
 *   |   |    |         | (1)  |
 *   |   |    ???         |      |
 *   |   -> |=c=|   -----      |
 *   |        |                |
 *   |(2)     |                |
 *   |        |                |
 *   |        ???                |
 *   --->   |=d=|  <------------
 * @param prev
 * @param dsc
 */

/**
 * MemoryBuffer?????????????????????(???????????????????????????)
 * Fixed????????????
 */
MemoryBufferSign MemoryAdjudicator::memoryBufferUseStatus(MemoryBuffer *buffer) {
    if(buffer->sign != MEMORY_BUFFER_SIGN_FIXED) {
        return memoryBufferUseStatus0(buffer->use_amount.load(std::memory_order_consume), buffer->buffer_len, buffer->sign);
    }else{
        return buffer->sign;
    }
}

/**
 * ??????use_info?????????????????????idle_info
 * @param use_info
 * @param pi_info
 * @return
 */
MemoryBufferIdleInfo* MemoryAdjudicator::recoverMemoryBufferUseInfo(MemoryBufferUseInfo *use_info, MemoryBufferIdleInfo *pi_info, bool is_last) {
    MemoryBuffer *buffer = nullptr;
    MemoryBufferUseInfo *pu_info = nullptr;
    MemoryBufferIdleInfo *idle_info = nullptr;
    MemoryCorrelate *use_correlate = nullptr;

    //??????use_info???????????????use_info???????????????????????????MemoryBuffer???
    if((use_info->delay_len ==  MEMORY_BUFFER_SUBSTITUTE_INFO_DELAY_LEN)){
        //use_info->delay_len == MEMORY_BUFFER_SUBSTITUTE_INFO_DELAY_LEN ?????????use_info????????????????????????MemoryBuffer???
        use_info->delay_len = 0;
        buffer = use_info->parent;
        pu_info = use_info->prev;
    }else if(use_info->info_status.load(std::memory_order_consume) == MEMORY_BUFFER_USE_INFO_STATUS_DEATH){
        buffer = use_info->parent;
        pu_info = use_info->prev;
        use_correlate = use_info->correlate;
    }

    if(buffer){
        //??????use_info
        unlinkMemoryBufferUseInfo(use_info);
        //???????????????idle_info,?????????idle_info
        idle_info = linkMemoryBufferIdleInfo(initMemoryBufferIdleInfo(buffer, pu_info, use_info->start,
                                                                      is_last ? buffer->buffer_len - use_info->start
                                                                              : use_info->len + use_info->delay_len),
                                                                      pi_info, buffer);
        //??????FIXED????????????MemoryCorrelate??????
        if(use_correlate){
            onReleaseMemoryCorrelate(use_correlate);
        }
    }

    return idle_info;
}

/**
 * ????????????(??????)???????????????
 * @param buffer                    ????????????MemoryBuffer
 * @param use_callback_func         use_info????????????
 * @param idle_callback_func        idle_info????????????
 * @param len                       ????????????
 * @param auto_shrink_buffer        ??????idle_info?????????(????????????????????????)
 * @return
 */
CommonHead* MemoryAdjudicator::searchAvailableBuffer(MemoryBuffer *buffer, std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> *lookup_func,
                                                     std::function<void(MemoryBufferUseInfo*)> *use_callback_func,
                                                     std::function<void(MemoryBufferIdleInfo*)> *idle_callback_func, int len, int search_flag) {
    //MemoryBuffer?????????????????????(??????)
    int remain_len = buffer->buffer_len - buffer->use_amount.load(std::memory_order_consume),
        idle_len = 0, search_pos = 0;
    MemoryBufferUseInfo *lpu_info = nullptr;
    MemoryBufferIdleInfo *pidle_info = nullptr;

    /*
     * ????????????,???????????????idle_info????????????????????????????????????
     */
    auto probe_func = [&](MemoryBufferIdleInfo *idle_info) -> void {
        //???????????????????????????
        remain_len -= (idle_len = getMemoryBufferIdleLen(reinterpret_cast<char*>(idle_info)));

        if(idle_len >= len){
            //????????????,??????pidle_info
            pidle_info = idle_info;
            //?????????????????????
            search_pos = pidle_info->start;
            //??????????????????
            pidle_info->start += len;
        }

        //??????idle_info????????????pi_info???,?????????idle_info???,??????????????????(pi_info.len + idle_info.len),
        //????????????pi_info????????????pi_info.len,?????????????????????pi_info.len
        if(get_flag(search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)){
            remain_len += idle_len;
        }
    };

    auto break_func = [&]() -> bool {
        //????????????????????????MemoryBuffer???use_info???idle_info
        return (((remain_len < len) && !get_flag(search_flag, MEMORY_BUFFER_FLAG_NOSIZE_UNBREAK)) ||
                (pidle_info && !get_flag(search_flag, MEMORY_BUFFER_FLAG_FIND_UNBREAK)));
    };

    std::function<MemoryBufferIdleInfo*(SearchInfo*)> use_func = [&](SearchInfo *sinfo) -> MemoryBufferIdleInfo* {
        MemoryBufferIdleInfo *recover_use_info = nullptr;

        if(!(recover_use_info = recoverMemoryBufferUseInfo(sinfo->use_info, sinfo->pi_info, sinfo->is_last))){
            //????????????use_info
            if(use_callback_func){
                (*use_callback_func)(sinfo->use_info);
            }
        }

        return recover_use_info;
    };

    std::function<int(SearchInfo*)> idle_func = [&](SearchInfo *sinfo) -> bool {
        lpu_info = sinfo->pu_info;

        //??????????????????
        if(isMergeMemoryBufferIdleInfo(sinfo->idle_info, sinfo->pi_info)){
            mergeMemoryBufferIdleInfo(sinfo->memory_buffer, sinfo->idle_info, sinfo->pi_info, MEMORY_BUFFER_MERGE_IDLE_INFO_UNLINK);
        }

        //?????????idle_info?????????????????????????????????????????????
        if(sinfo->is_last && (buffer->buffer_len > sinfo->idle_info->end)){
            //???????????????idle_info???????????????
            sinfo->idle_info = expandMemoryBufferIdleInfo(sinfo->idle_info, sinfo->memory_buffer, sinfo->memory_buffer->buffer_len);
        }

        if(idle_callback_func){
            (*idle_callback_func)(sinfo->idle_info);
        }

        //??????idle_info
        probe_func(sinfo->idle_info);

        return break_func();
    };

    static std::function<void(MemoryAdjudicator*, SearchInfo*)> shrink_func = [](MemoryAdjudicator *adjudicator, SearchInfo *sinfo) -> void{
        adjudicator->shrinkMemoryBufferIdleInfo0(sinfo->pi_info, sinfo->upi_info, sinfo->memory_buffer);
    };

    //???????????????
    SearchInfo search_info(buffer, &use_func, &idle_func, get_flag(search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER) ? &shrink_func : nullptr);
    searchMemoryBuffer(&search_info);

    //??????????????????????????????
    if(pidle_info){
        MemoryBufferUseInfo *use_info = nullptr;
        if(lookup_func){
            use_info = (*lookup_func)(buffer, search_pos, len, 0);
            //????????????idle_info?????????use_info
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
 * ??????????????????
 * fixed?????????
 * @param load_buffer
 * @param use_func
 * @param idle_func
 */
void MemoryAdjudicator::repairLoadMemoryBuffer(MemoryBuffer *load_buffer, std::function<void(MemoryBufferUseInfo*)> *use_func, std::function<void(MemoryBufferIdleInfo*)> *idle_func) {
    int memory_use_len = load_buffer->use_amount.load(std::memory_order_consume);

    std::function<MemoryBufferIdleInfo*(SearchInfo*)> search_use_func = [&](SearchInfo *sinfo) -> MemoryBufferIdleInfo*{
        MemoryBufferIdleInfo *recover_use_info = nullptr;

        //??????use_info????????????
        if(!(recover_use_info = recoverMemoryBufferUseInfo(sinfo->use_info, sinfo->pi_info, sinfo->is_last))){
            if(use_func != nullptr){
                //??????use_info????????????
                (*use_func)(sinfo->use_info);
            }
        }

        return recover_use_info;
    };

    std::function<int(SearchInfo*)> search_idle_func = [&](SearchInfo *sinfo) -> int{
        if(isMergeMemoryBufferIdleInfo(sinfo->idle_info, sinfo->pi_info)){
            mergeMemoryBufferIdleInfo(sinfo->memory_buffer, sinfo->idle_info, sinfo->pi_info, MEMORY_BUFFER_MERGE_IDLE_INFO_UNLINK);
        }

        //?????????idle_info?????????????????????????????????????????????
        if(sinfo->is_last && (load_buffer->buffer_len > sinfo->idle_info->end)){
            //???????????????idle_info???????????????
            sinfo->idle_info = expandMemoryBufferIdleInfo(sinfo->idle_info, sinfo->memory_buffer, sinfo->memory_buffer->buffer_len);
        }

        if(idle_func != nullptr){
            //??????idle_info????????????
            (*idle_func)(sinfo->idle_info);
        }

        return 0;
    };

    SearchInfo search_info(load_buffer, &search_use_func, &search_idle_func, nullptr);
    searchMemoryBuffer(&search_info);

    //??????Buffer????????????
    load_buffer->sign = memoryBufferUseStatus0(memory_use_len, load_buffer->buffer_len ,load_buffer->sign);
}

/**
 * ???????????????????????????
 * @param buffer_info
 * @param buffer
 * @return
 */
MemoryBufferUseInfo* MemoryAdjudicator::initMemoryBufferUseInfo(MemoryBufferUseInfo *use_info, MemoryCorrelate *correlate ,MemoryBuffer *buffer, int start, int len, int delay_len) {
    memory_alloc.construct(use_info, start, len, delay_len, buffer, correlate);
    return use_info;
}

/**
 * ???????????????????????????
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
 * ???????????????????????????
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
 * ??????????????????
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
//    ??????????????????([=]() -> void {
        forceLoadMemoryBuffer(load_holder, load_use_size, load_buffer_size, alloc_buffer_len);
//    });
    }
    //??????????????????MemoryBuffer(??????????????????,????????????)
    onResponseMemoryBuffer(load_holder);
}

/**
 * ????????????????????????
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
            //???use_info???????????????merge_use_array???
            insert_use_pos = InsertSortUse(location_table + buffer_pos, reinterpret_cast<char*>(use_info), merge_use_array, use_info->len, use_pos++);
            onMemoryBufferRecorderDebris(MEMORY_ADJUDICATOR_BUFFER_INFO_INCREASE_VALUE);
        }
    };

    std::function<void(MemoryBufferIdleInfo*)> idle_func = [&](MemoryBufferIdleInfo *idle_info) -> void{
        if(insert_use_pos >= 0){
            //??????use_info?????????idle_info(idle_info?????????use_info????????????????????????,?????????????????????use_info)
            //|-...-|-use_info-|-idle_info-|-...-|
            //                    uni_info
            merge_use_array[insert_use_pos].uni_info = reinterpret_cast<char*>(idle_info);
        }
    };

    load_holder.onHolderNote([&](MemoryBufferIterator iterator) ->  bool {
        if((--load_buffer_size) <= 0){ return false; }

        //??????MemoryBuffer?????????use_info
        repairLoadMemoryBuffer(*iterator, &use_func, &idle_func);

        if(load_use_len > 0) {
            //????????????memory_buffer????????????merge_array
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
 * ??????MemoryBufferUseInfo
 * ???MemoryBufferHead????????????????????????,????????????MemoryBufferUseInfo????????????MemoryBufferHead
 * @param use_info
 * @param pu_info
 * @param buffer ??????use_info?????????????????????MemoryBufferSubstituteInfo,??????parent,?????????????????????
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
 * ??????MemoryBufferUseInfo
 * ???MemoryBufferHead?????????MemoryBufferUseInfo,?????????MemoryBufferUseInfo???????????????
 * @param use_info
 * @param pu_info
 * @param buffer ???linkMemoryBufferUseInfo
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
 * ???use_info???????????????????????????substitute_info
 * ???substitute_info?????????use_info
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
 * ??????Buffer????????????
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
 * ???????????????MemoryBuffer???use_info???idle_info
 * ??????MemoryBuffer?????????????????????idle_info,?????????MemoryBuffer????????????????????????idle_info
 * @param search_info
 */
void MemoryAdjudicator::searchMemoryBuffer(SearchInfo *search_info) {
    //use_info???idle_info?????????
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
         * ??????use_info???idle_info
         * ????????????idle_info ??? (???use_info ??? use_info???idle_info?????????)
         */
        if(!search_info->idle_info || (search_info->use_info && (search_info->use_info->start < search_info->idle_info->start))){
            //??????use_info???????????????
            MemoryBufferIdleInfo *ni_info = (*search_info->use_func)(search_info);

            //?????????????????????use_info
            if(ni_info){
                search_info->use_info = search_info->pu_info ? search_info->pu_info->next : search_info->memory_buffer->use_buf_info;

                //?????????use_info??????????????????idle_info
                //??????idle_info???use_info
                search_info->idle_info = ni_info;

                //????????????
                goto find_new_idle;
            }else{
                //?????????????????????use_info
                //??????pu_info???use_info
                search_info->pu_info = search_info->use_info;
                search_info->use_info = search_info->pu_info->next;
            }
        }else{
            //??????????????????????????????pi_info
            if(search_info->pi_info && search_info->shrink_func){
                (*search_info->shrink_func)(this, search_info);

                search_info->pi_info = search_info->idle_info->prev;
            }

            //??????idle_info???????????????
            if((*search_info->idle_func)(search_info)){
                break;
            }

            //??????upi_info???pi_info?????????use_info(shrink_func??????????????????)
            search_info->upi_info = search_info->pu_info;
            search_info->idle_info = (search_info->pi_info = search_info->idle_info)->next;

        }

        if(!msize){
            //????????????use_info???idle_info,???????????????????????????pi_info???
            break;
        }
    }
}

/**
 * ??????MemoryBufferIdleInfo(?????????????????????,?????????????????????)
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
         * ???
         * (2)|___....___|___(pi_info)____|_____(idle_info)______|___________|.....|_________|
         * ???
         * (3)|___|....|___(pi_info)____|______....____|____(idle_info)____|_____|...|____|
         */
        idle_info->prev = pi_info;
        idle_info->next = pi_info->next;

        if(pi_info->next){
            pi_info->next->prev = idle_info;
        }
        pi_info->next = idle_info;
//        if(isMergeMemoryBufferIdleInfo(idle_info, pi_info)) {
//            //(1)???(2)????????????
//            mergeMemoryBufferIdleInfo(buffer, idle_info, pi_info, MEMORY_BUFFER_MERGE_IDLE_INFO_UNLINK);
//        }
    }else{
        /*
         * |___(pu_info)___|____(idle_info)_____|______|....|______|
         * ???
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
 * ??????MemoryBufferIdleInfo
 * @param idle_info  ????????????
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
 * ???use_info?????????buf????????????
 * @param buffer    ????????????
 * @param use_info  ????????? (???MemoryBufferSubstituteInfo)
 * @param opu_info  ?????????buffer???????????????????????????
 * @param pu_info   ???????????????use_info???????????????use_info
 * @param idle_info ?????????,???buffer????????????idle_info
 */
MemoryBufferUseInfo* MemoryAdjudicator::memmoveMemoryBufferUseInfo(MemoryBuffer *buffer, MemoryBufferUseInfo *use_info, MemoryBufferIdleInfo *uni_info, MemoryBufferUseInfo *npu_info,
                                                                   MemoryBufferIdleInfo *idle_info, std::function<void(int)> move_func){
    char *move_buf = nullptr, *src_buf = nullptr;

    std::function<char*()> correlate_func = [&]() -> char* {
        memmove(move_buf, src_buf, static_cast<size_t>(use_info->len));
        return move_buf;
    };

    /*
     * ??????use_info,idle_info,???????????????,?????????????????????
     */
    if(use_info && idle_info && (move_buf = buffer->buffer + idle_info->start) && (src_buf = use_info->parent->buffer + use_info->start)){

        //???????????????MemoryBuffer?????????use_info
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

        //???use_info???MemoryBufferSubstituteInfo???,?????????????????????MemoryBuffer?????????,??????????????????idle_info.ipu_info?????????use_info
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
 * ???idle_info?????????buf????????????
 * @param buffer
 * @param idle_info
 * @param pi_info
 * @param buf
 * @return
 */
MemoryBufferIdleInfo* MemoryAdjudicator::memmoveMemoryBufferIdleInfo(MemoryBuffer*, MemoryBufferIdleInfo*, MemoryBufferIdleInfo*, char*){
    //?????????????????????
    return nullptr;
}

/**
 * ??????idle_info?????????
 * @param idle_info         ??????idle_info
 * @param pu_info           idle_info????????????use_info
 * @param len               idle_info???????????????
 * @param shrink_func       ?????????????????????????????????
 * @return
 */
int MemoryAdjudicator::shrinkMemoryBufferIdleInfo(MemoryBufferIdleInfo *idle_info, MemoryBufferUseInfo *pu_info, MemoryBuffer *buffer) {
    int idle_len = idle_info->end - idle_info->start;

    //??????idle_info?????????(end - start)??????????????????????????????(sizeof(MemoryBufferIdleInfo)),??????????????????MemoryBufferIdle
    if(idle_len <= sizeof(MemoryBufferIdleInfo)){
        shrinkMemoryBufferIdleInfo0(idle_info, pu_info, buffer);
    }else{
        idle_len = 0;
    }

    return idle_len;
}

/**
 * ??????idle_info?????????use_info,??????idle_info->next?????????use_info??????
 *  idle_info???????????????use_info????????????idle_info?????????
 *  ???????????????idle_info
 *
 * FIXED?????????,??????USE???use_info.correlate?????????FIXED?????????,??????FIXED???????????????,???use_info.correlate??????????????????
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


//    4??????????????????
//    |_idle_|    |_ni_|    -->     (1)
//    0           |_ni_|    -->     (2)
//    |_idle_|    0         -->     (3)
//    0           0         -->     (4)
    if(buffer->sign != MEMORY_BUFFER_SIGN_FIXED){

        if(shrink_idle.end > shrink_idle.start){
            //??????(1) ??? (3) ??? ??????FIXED???FIXED???????????????,????????????idle_info?????????
            /*
             * ??????use_info, ni_idle,??????????????????idle_info????????????ni_idle??????
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

        //????????????,????????????????????????idle_info
        if(isMergeMemoryBufferIdleInfo(ni_idle, &shrink_idle)) {
            mergeMemoryBufferIdleInfo(buffer, ni_idle, &shrink_idle, MEMORY_BUFFER_MERGE_IDLE_INFO_NOTUNLINK);
        }
    }

    //???idle????????????idle_info
    unlinkMemoryBufferIdleInfo(&shrink_idle, buffer);
}

/**
 * ??????idle_info????????????????????????buffer???????????????
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
//    ??????????????????UnLockLIst????????????
//    buffer_array[MEMORY_BUFFER_SIGN_USE].set_operator_concurrent(operator_concurrent);
}

/**
 * ??????????????????
 * @return
 */
MemoryBufferHolder MemoryRecorder::extractLoadMemoryBuffer() {
    MemoryBufferHolder load_holder = buffer_array[MEMORY_BUFFER_SIGN_LOAD].thread_holder();
    records[MEMORY_BUFFER_SIGN_LOAD]->setRecord(load_holder.onHolderSize(), this, RecordNumber());
    return load_holder;
}


/**
 * ??????MemoryBufferHead
 *  ?????????????????????MemoryBuffer
 *  ??????sign???MemoryBufferHead
 * unmount???MemoryBufferHead
 * @param memory_buffer
 * @param push              ??????????????????
 */
void MemoryRecorder::mountMemoryBuffer(MemoryBuffer *memory_buffer, bool push) {
    if(push){
        records[memory_buffer->sign]->setRecord(1, this, record_number);
        buffer_array[memory_buffer->sign].push_front(memory_buffer);
    }

    memory_buffer->mount.store(MEMORY_BUFFER_STATUS_MOUNT, std::memory_order_release);
}

/**
 * ??????MemoryBufferHead
 *  ??????MemoryBufferHead?????????
 *  ??????sign???MemoryBufferHead
 *  ???????????????MemoryBufferHead
 * @param memory_buffer
 * @param try_size
 * @return
 */
bool MemoryRecorder::unmountMemoryBuffer(MemoryBuffer *memory_buffer, int try_size) {
    bool buffer_status, unmount_res;

    //??????????????????MemoryBuffer?????????
    if(try_size >= std::thread::hardware_concurrency()){
        //??????????????????????????????????????????
        unmount_res = true;
        //std::memory_order_acquire?????????,????????????????????????????????????
        buffer_status = memory_buffer->mount.load(std::memory_order_acquire);
        do{
            if(buffer_status == MEMORY_BUFFER_STATUS_UNMOUNT){
                unmount_res = false;
                break;
            }
        }while(!memory_buffer->mount.compare_exchange_weak(buffer_status, MEMORY_BUFFER_STATUS_UNMOUNT, std::memory_order_release, std::memory_order_acquire));
    }else{
        //?????????????????????????????????
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
 * ??????head_array?????????????????????>=len???MemoryBufferHead
 * @param sign
 * @param len
 * @param call_func
 * @return
 */
CommonHead* MemoryRecorder::lookupAvailableBuffer(MemoryBufferSign sign, int lookup_len, int lookup_flag, int alloc_buffer_len,
                                                  std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> *lookup_func) {
    LookupInfo lookup_info = LookupInfo(lookup_flag, lookup_len, alloc_buffer_len, 0, sign, lookup_func);

    if((lookup_len > (MEMORY_BUFFER_BIG_MEMORY_FACTOR * alloc_buffer_len)) && (sign != MEMORY_BUFFER_SIGN_FIXED)){
        //????????????????????????
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
 * ????????????
 * @param lookup_info
 * @param load_buffers MEMORY_BUFFER_SIGN_FIXED?????????????????????,????????????????????????,??????load_buffers == nullptr
 * @return
 */
CommonHead* MemoryRecorder::lookupAvailableBuffer0(LookupInfo *lookup_info, MemoryBufferHolder *load_holder) {
    int unmount_res = 0;
    //????????????
    CommonHead *lookup_common = nullptr;
    MemoryBufferIterator lookup_iterator;
    MemoryBufferIterator beginp, endp;

    /*
     * ????????????
     */
    auto factor_func = [&]() -> bool {
        bool push_load = false;

        //??????MemoryBuffer???MEMORY_BUFFER_SIGN_FIXED ??? MemoryBuffer?????????MEMORY_BUFFER_SIGN_LOAD??????
        if(((*lookup_iterator)->sign == MEMORY_BUFFER_SIGN_FIXED) ||
           (push_load = (((*lookup_iterator)->sign = memoryBufferUseStatus(*lookup_iterator)) == MEMORY_BUFFER_SIGN_LOAD))){
            //????????????MemoryBuffer??????
            onMemoryBufferExhaust(*lookup_iterator);
        }

        if(load_holder && push_load){
            //??????lookup_buffer;
            records[lookup_info->sign]->setRecord(-1, this, record_number);
            try {
                buffer_array[lookup_info->sign].erase(beginp);
            }catch (MemoryBufferEraseFinal &e){
                //??????????????????????????????MemoryBuffer,????????????
                (*load_holder) << lookup_iterator;
                //load_buffers->push_front(lookup_buffer);
            }
        }

        return push_load;
    };

    //??????MemoryBuffer?????????????????????????????????????????????
    auto check_len = [&](MemoryBuffer *check_buf) -> bool {
        int remain_len = check_buf->buffer_len - check_buf->use_amount.load(std::memory_order_consume);
        return (remain_len >= lookup_info->lookup_len);
    };

    //?????????????????????MEMORY_BUFFER_SIGN_USE??????
    for(int satisfy_buffer = 0; !buffer_array[lookup_info->sign].empty(); satisfy_buffer = 0){
restart:
        beginp = buffer_array[lookup_info->sign].begin(), endp = buffer_array[lookup_info->sign].end();

        for(; beginp != endp; ++beginp){
            records[lookup_info->sign]->setRecord(1, this, record_call_size);
            lookup_iterator = beginp;

            //??????????????????
            if(!get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)){
                (lookup_info->unmount_value *= 2)++;
            }

            //?????????MemoryBuffer?????????????????????????????????????????????
            if(check_len(*lookup_iterator)){
                //??????????????????
                satisfy_buffer++;
                //??????MemoryBuffer??????
                unmount_res = unmountMemoryBuffer(*lookup_iterator, lookup_info->unmount_value);
                //????????????????????????
                if(unmount_res){
                    records[lookup_info->sign]->setRecord(1, this, record_occupy_size);

                    //??????????????????????????????????????????????????????(????????????,???????????????????????????????????????????????????????????????????????????????????????????????????)
                    if(check_len(*lookup_iterator) && (lookup_common = onLookupMemoryBuffer(*lookup_iterator, lookup_info->lookup_func,
                                                                                         nullptr, nullptr, lookup_info->lookup_len, lookup_info->search_flag))){
                        //?????????????????????,??????????????????,????????????
                        mountMemoryBuffer(*lookup_iterator, false);
                        records[lookup_info->sign]->setRecord(1, this, record_success_size);
                        break;
                    }else{
                        //????????????,??????????????????
                        satisfy_buffer--;
                        //????????????????????????MemoryBuffer?????????MEMORY_BUFFER_SIGN_LOAD??????
                        if(!get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER) && factor_func()){
                            //????????????
                            goto restart;
                        }
                        //??????????????????
                        mountMemoryBuffer(*lookup_iterator, false);
                    }
                }else{
                    records[lookup_info->sign]->setRecord(1, this, record_unoccupy_size);
                }
            }else{
                //????????????????????????????????????
                if(!get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)) {
                    //????????????MemoryBuffer??????
                    onMemoryBufferExhaust(*lookup_iterator);
                }
            }
        }

        //?????????????????????,????????????
        if(lookup_common){
            break;
        }

        /*
         * USE?????????????????????????????????????????????len????????????????????????len???????????????
         */
        if(satisfy_buffer <= 0){
            //???LOAD??????????????????????????????(??????)????????????,????????????(??????????????????)
            if((lookup_common = lookupAvailableBufferOnLoad(lookup_info, load_holder)) || !get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)){
                break;
            }

            if(get_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_BIG_BUFFER)) {
                //?????????,??????????????????,????????????
                onForceCreateMemoryBuffer();
            }else{
                break;
            }
        }
    }

    /**
     * ?????????????????????LOAD???????????????MemoryBuffer
     */
    if(load_holder && (load_holder->onHolderSize() > 0)){
        treatmentLoadMemoryBuffer(*load_holder, lookup_info->load_use_len, lookup_info->load_use_size, lookup_info->load_buffer_len,
                                  lookup_info->load_buffer_size, lookup_info->alloc_buffer_len);
    }
    return lookup_common;
}

/**
 * ??????LOAD??????
 * @param lookup_info
 * @param load_buffers
 * @return
 */
CommonHead* MemoryRecorder::lookupAvailableBufferOnLoad(LookupInfo *lookup_info, MemoryBufferHolder *load_buffers) {
    int lookup_use_len = 0, lookup_use_size = 0;
    //????????????
    CommonHead *lookup_common = nullptr;

    //????????????LOAD???????????????????????????
    if(lookup_info->search_load_buffer && load_buffers){
        //??????LOAD????????????
//        MemoryBufferHolder load_sign_buffers = onRequestLoadMemoryBuffer() + *load_buffers;
        (*load_buffers) += onRequestLoadMemoryBuffer();
//      if (!load_sign_buffers.empty()) {
//          //????????????LOAD??????????????????????????????LOAD???????????????
//          load_buffers->merge(load_sign_buffers);
//      }

        load_buffers->onHolderNote([&](MemoryBufferIterator iterator) -> bool {
            bool on_lookup = false;
            if(!lookup_common && (lookup_common = onLookupMemoryBuffer(*iterator, lookup_info->lookup_func, &lookup_use_len, &lookup_use_size,
                                                                       lookup_info->lookup_len, set_flag(lookup_info->search_flag, MEMORY_BUFFER_FLAG_NOSIZE_UNBREAK)))){
                //??????????????????????????????MemoryBuffer
                mountMemoryBuffer(*iterator, true);
                on_lookup = true;
            }else{
                //??????use_info??????
                if(!lookup_common){
                    //??????????????????,?????????????????????onLookupMemoryBuffer??????(???lookup_use_len???lookup_use_size???????????????),??????????????????????????????
                    lookup_info->load_use_len += lookup_use_len;
                    lookup_info->load_use_size += lookup_use_size;
                }else{
                    //???????????????????????????,????????????onLookupMemoryBuffer??????(???lookup_use_len???lookup_use_size???????????????)
                    lookup_info->load_use_len += (*iterator)->use_amount.load(std::memory_order_consume);
                    lookup_info->load_use_size += (*iterator)->use_info_size;
                }

                //??????MemoryBuffer??????
                lookup_info->load_buffer_len += (*iterator)->buffer_len;
                lookup_info->load_buffer_size++;
            }

            lookup_use_len = 0;
            lookup_use_size = 0;
            return on_lookup;
        });

        //????????????????????????len???MemoryBuffer?????????
//        MemoryBufferIterator lookup_iterator = load_buffers->end();
//        for(auto beginp = load_buffers->begin(), endp = load_buffers->end(); beginp != endp; ++beginp, lookup_use_len = 0, lookup_use_size = 0){
//
//        }

        //???????????????
//        if(lookup_iterator != load_buffers->end()){
//            //???????????????????????????len???MemoryBuffer???LOAD?????????????????????
//            load_buffers->erase(lookup_iterator);
//            //????????????
//            mountMemoryBuffer(*lookup_iterator, true);
//        }

        lookup_info->search_load_buffer = false;
    }


    return lookup_common;
}

/**
 * ??????MemoryBuffer???use_info???idle_info????????????MemoryBuffer??????????????????
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
 * ??????????????????
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
 * MemoryBuffer??????????????????(??????Idle?????????Use)
 */
void MemoryRecorder::onMemoryBufferReady() {
    MemoryBufferSign sign = MEMORY_BUFFER_SIGN_IDLE;
    //??????MemoryBuffer
    MemoryBufferList::clear(&buffer_array[sign],
                            [&](decltype(buffer_array[MEMORY_BUFFER_SIGN_IDLE].begin()) iterator) -> void {
                                records[sign]->setRecord(-1, this, record_number);
                                //??????MemoryBuffer?????????
                                (*iterator)->sign = memoryBufferUseStatus(*iterator);
                                //???????????????MemoryBuffer
                                mountMemoryBuffer(*iterator, true);
                            });
}

/**
 * MemoryBuffer???????????????????????????Load???Use?????????IDLE???
 * @param buffers
 */
void MemoryRecorder::onMemoryBufferUnReady(MemoryBufferSign sign) {
    onMemoryBufferUnReady0(&buffer_array[sign]);
}

void MemoryRecorder::onMemoryBufferUnReady0(MemoryBufferList *buffers) {
    //??????MemoryBuffer(MemoryManager????????????????????????)
    MemoryBufferList::clear(buffers,
                            [&](decltype(buffers->begin()) iterator) -> void {
                                records[(*iterator)->sign]->setRecord(-1, this, record_number);
                                //?????????IDLE?????????
                                (*iterator)->sign = MEMORY_BUFFER_SIGN_IDLE;
                                //???????????????MemoryBuffer
                                mountMemoryBuffer(*iterator, true);
                            });
}

/**
 * ???????????????sign???MemoryBuffer
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
 * ????????????
 * @param head
 * @return
 */
MemoryCorrelate* MemoryManager::obtainBuffer(uint32_t obtain_len) {
    char *obtain_buf = nullptr;
    MemoryBufferUseInfo *use_info = nullptr;
    MemoryCorrelate *correlate = nullptr;

    //??????MEMORY_BUFFER_SIGN_FIXED???????????????MemoryCorrelate???MemoryBufferUseInfo?????????
    if((correlate = (reinterpret_cast<MemoryCorrelate*>(makeInsideMemoryBuffer(memory_buffer_correlate_use, MEMORY_BUFFER_FLAG_MAKE_SPACE_HEAD | MEMORY_BUFFER_FLAG_INIT_CORRELATE)->buf)))){
        //??????use_info
        use_info = getMemoryBufferUseInCorrelate(correlate);

        typedef MemoryBufferUseInfo* (*init_use_func)(MemoryAdjudicator*, MemoryBufferUseInfo*, MemoryCorrelate*, MemoryBuffer*, int, int, int);
        std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> lookup_func = std::bind(reinterpret_cast<init_use_func>(&MemoryAdjudicator::initMemoryBufferUseInfo),
                                                                                                  dynamic_cast<MemoryAdjudicator*>(this), use_info, correlate, std::placeholders::_1,
                                                                                                  std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

        for(;;){
            //????????????
            if((obtain_buf = reinterpret_cast<char*>(lookupAvailableBuffer(MEMORY_BUFFER_SIGN_USE, obtain_len, MEMORY_BUFFER_FLAG_NONE, allocIdleBufferLen(), &lookup_func)))){
                //??????????????????
                correlate->setCorrelateBuffer0(obtain_buf);
                break;
            }else{
                //????????????,????????????
                makeMemoryBuffer(MEMORY_BUFFER_THREAD_PERMIT_CREATE_APPLY, MEMORY_BUFFER_SIGN_DEFAULT_INIT_SIZE, &idle_info,
                                 std::bind((init_func)&MemoryManager::initIdleBuffer, this, std::placeholders::_1));
            }
        }
    }

    return correlate;
}


/**
 * ????????????
 * @param rp common->buf
 * @param rh ??????????????????
 */
void MemoryManager::returnBuffer(MemoryCorrelate *correlate){ //char *rp, const SpaceHead &rh) {
//    {
//        int buffer_len = 0;
        //???????????????,???????????????????????????,????????????????????????
//        char *rb = correlate->getCorrelateBuffer();
//        SpaceHead rh;
//        if (rb) {
//            releaseMemoryBuffer(rb, getMemoryBufferUseInfo(backCommonHead(rb), (buffer_len = reductionBufSize((rh = extractSpaceHead(rb))))), rh, buffer_len);
//        }
    MemoryBufferUseInfo *release_use = nullptr;
    if((release_use = getMemoryBufferUseInCorrelate(correlate))){
        releaseMemoryBuffer(release_use);
    }
//        //???????????????
//        correlate->releaseCorrelateBuffer();
//    }
//    releaseMemoryCorrelate(correlate);
}

/**
 * ??????MemoryBufferUseInfo
 * @param rp
 * @param use_info      MemoryBuffer????????????
 * @param space_head    ??????????????????
 * @param buffer_len    ????????????
 */
void MemoryManager::releaseMemoryBuffer(MemoryBufferUseInfo *use_info){//(char *rp, MemoryBufferUseInfo *use_info, const SpaceHead &space_head, int buffer_len) {
    int use_len = getMemoryBufferUseLen(reinterpret_cast<char*>(use_info)),
        info_status = MEMORY_BUFFER_USE_INFO_STATUS_LIVE;
    MemoryBuffer *parent = use_info->parent;

//    {
//        SpaceHead head = space_head;
//        //???????????????
//        head.control = 0;
//        //????????????????????????
//        clearSpaceHead(rp);
//        //????????????????????????
//        makeSpaceHead(rp, head);
//    }


    while(!use_info->info_status.compare_exchange_weak(info_status, MEMORY_BUFFER_USE_INFO_STATUS_DEATH, std::memory_order_release, std::memory_order_acquire)){
        parent = use_info->parent;
        info_status = MEMORY_BUFFER_USE_INFO_STATUS_LIVE;
    }

    //??????MemoryBuffer?????????????????????
    parent->use_amount.fetch_sub(use_len, std::memory_order_release);
}

/**
 * ??????MemoryCorrelate???????????????????????????FIXED??????????????????,FIXED????????????????????????
 * @param correlate
 */
void MemoryManager::onReleaseMemoryCorrelate(MemoryCorrelate *correlate) {
    int buffer_len = 0;
    //????????????????????????
    SpaceHead correlate_head = extractSpaceHead(reinterpret_cast<char*>(correlate));
    MemoryBuffer *parent = nullptr;
    MemoryBufferUseInfo *use_info = nullptr;

    if(correlate){
        //??????MemoryBufferUseInfo
        use_info = getMemoryBufferUseInfo(backCommonHead(reinterpret_cast<char*>(correlate)), (buffer_len = reductionBufSize(correlate_head)));
        parent = use_info->parent;

        //???????????????
//        correlate_head.control = 0;
//        //????????????????????????
//        clearSpaceHead((char*)correlate);
        //????????????????????????
//        makeSpaceHead((char*)correlate, correlate_head);
        use_info->info_status.store(MEMORY_BUFFER_USE_INFO_STATUS_DEATH, std::memory_order_release);

//      ????????????MemoryCorrelate???Fixed?????????????????????
        parent->use_amount.fetch_sub(buffer_len, std::memory_order_release);
    }

}


/**
 * ??????????????????FixedBuffer
 * @param fixed_size FixedBuffer????????????????????????????????????
 *
 * |__SpaceHead__|__MemoryBuffer__|__MemoryBufferNote(UnLockList)__|___buffer___| ==> fixed_size
 *
 */
void MemoryManager::initFixedBuffer(int fixed_size)  throw(std::bad_alloc){
    int fixed_hlen = sizeof(MemoryBuffer), fixed_nlen = sizeof(MemoryBufferNote);
    MemoryBuffer *memory_buffer = nullptr;

    InitMemoryBuffer buffers[fixed_size];
    //??????fixed_size???????????????
    try {
        if ((fixed_size = initMemoryBuffer(fixed_size, allocFixedBufferLen(), buffers)) > 0) {
            for (int i = 0; i < fixed_size; i++) {
                //?????????FixedBuffer
                memory_buffer = initMemoryBufferHead(reinterpret_cast<MemoryBuffer*>(buffers[i].common->buf),
                                                     buffers[i].common->buf + fixed_hlen + fixed_nlen,
                                                     buffers[i].len - fixed_hlen - fixed_nlen,
                                                     MEMORY_BUFFER_SIGN_FIXED);
                //???????????????FixedBuffer
                mountMemoryBuffer(memory_buffer, true);

                //??????
                onMakeMemoryBuffer(memory_buffer, &fixed_info);
            }
        }
    }catch (std::bad_alloc &e){
        std::cout << "initFixedBuffer->(" << e.what() << ")" << std::endl;
        throw;
    }
}

/**
 * ??????????????????IdleBuffer
 * @param idle_size IdleBuffer?????????(?????????????????????)
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
        //??????idle_size???????????????
        if ((idle_size = initMemoryBuffer(idle_size, allocIdleBufferLen(), buffers)) > 0) {
            try {
                for (int i = 0; i < idle_size; i++, make_pos++) {
                    //?????????IdleBuffer(???FixedBuffer?????????????????????memory_buffer?????????????????????)
                    memory_buffer = makeMemoryBufferHead(buffers[i].common->buf, buffers[i].len,
                                                         MEMORY_BUFFER_SIGN_IDLE);
                    //???????????????IdleBuffer
                    mountMemoryBuffer(memory_buffer, true);

                    //??????
                    onMakeMemoryBuffer(memory_buffer, &idle_info);

                }
            } catch (std::bad_alloc &e) {
                //???FixedBuffer?????????memory_buffer??????
                std::cout << "initIdleBuffer->(" << e.what() << ")" << std::endl;

                for (; make_pos < idle_size; make_pos++) {
                    //????????????
                    uinitMemoryBuffer(buffers[make_pos]);
                }

                //????????????
                if (getRecordInfo(MEMORY_BUFFER_SIGN_IDLE, RecordNumber()) <= 0) {
                    throw;
                }
            }
        }
    }catch (std::bad_alloc &e){
        std::cout << "initIdleBuffer->(" << e.what() << ")" << std::endl;
    }

    //???IDLE?????????MemoryBuffer?????????MEMORY_BUFFER_SIGN_USE??????
    if(getRecordInfo(MEMORY_BUFFER_SIGN_IDLE, RecordNumber()) > 0){
        onMemoryBufferReady();
    }
}

/**
 * ??????????????????
 */
void MemoryManager::onForceCreateMemoryBuffer() {
    makeMemoryBuffer(MEMORY_BUFFER_THREAD_PERMIT_CREATE_FROCE, MEMORY_BUFFER_SIGN_DEFAULT_INIT_SIZE, &idle_info,
                     std::bind((init_func)&MemoryManager::initIdleBuffer, this, std::placeholders::_1));
}

/**
 * ???????????????????????????
 * @param init_
 * @param init_size
 */
void MemoryManager::onInitMemoryBuffer(std::function<void(int)> &init_, int init_size) {
    if((init_ != nullptr) && (init_size > 0)){
        init_(init_size);
    }
}

/**
 * ??????MemoryBuffer
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
 * ?????????????????????Fixed??????
 */
void MemoryManager::uinitFixedBuffer() {
    onPopMemoryBuffer(MEMORY_BUFFER_SIGN_FIXED, [&](MemoryBuffer *buffer){
        this->uinitMemoryBuffer((CommonHead*)backCommonHead((char*)buffer));
    });
}

/**
 * ?????????????????????Idle??????
 */
void MemoryManager::uinitIdleBuffer() {
    //?????????????????????????????????MemoryBuffer
    onMemoryBufferUnReady(MEMORY_BUFFER_SIGN_LOAD);
    //??????????????????????????????????????????MEmoryBuffer
    while(getRecordInfo(MEMORY_BUFFER_SIGN_LOAD, RecordNumber()) > 0);
    //??????????????????MemoryBuffer
    onMemoryBufferUnReady(MEMORY_BUFFER_SIGN_LOAD);
    //????????????USE???MemoryBuffer
    onMemoryBufferUnReady(MEMORY_BUFFER_SIGN_USE);

    onPopMemoryBuffer(MEMORY_BUFFER_SIGN_IDLE, [&]( MemoryBuffer *buffer){
        this->uinitMemoryBuffer((CommonHead*)backCommonHead(buffer->buffer));
    });
}


/**
 * ?????????FixedBuffer???IdleBuffer???MemoryBuffer???MemoryBufferNote(UnLockList)
 * @param buffer
 * @param buf
 * @param len
 * @param sign
 * @return
 */
MemoryBuffer* MemoryManager::initMemoryBufferHead(MemoryBuffer *buffer, char *buf, int len, MemoryBufferSign sign) {
    //?????????MemoryBuffer
    memory_alloc.construct(buffer, sign, len, buf);
    //?????????MemoryBufferNote
    memory_alloc.construct<MemoryBufferNote>(MemoryBufferNote::createNote(buffer), buffer);

    //?????????Memory???MemoryBufferIdleInfo?????????
    linkMemoryBufferIdleInfo(initMemoryBufferIdleInfo(buffer, nullptr, 0, len), nullptr, buffer);

    return buffer;
}

/**
 * ??????MemoryBuffer
 *  FixedBuffer : ?????????MemoryBuffer
 *  IdleBuffer  : ???FixedBuffer?????????MemoryBuffer
 * @param buf   ??????(SpaceHead???????????????)
 * @param len   ????????????(SpaceHead?????????????????????)
 * @param sign  ?????????Fixed???Idle???
 * @return
 */
MemoryBuffer* MemoryManager::makeMemoryBufferHead(char *buf, int len, MemoryBufferSign sign) {
    return initMemoryBufferHead(reinterpret_cast<MemoryBuffer*>(makeInsideMemoryBuffer(memory_buffer_head, MEMORY_BUFFER_FLAG_MAKE_SPACE_HEAD)->buf), buf, len, sign);
}

/**
 * ?????????MemoryCorrelate
 * @param correlate
 * @param common_head ???????????????
 * @return
 */
void MemoryManager::initMemoryCorrelate(MemoryCorrelate *correlate, char *buf) {
    memory_alloc.construct(correlate, buf);
}

/**
 * ??????????????????(IdleBuffer???Memory???MemoryCorrelate)
 * @param head
 * @return
 */
CommonHead* MemoryManager::makeInsideMemoryBuffer(const SpaceHead &head, int flag) {
    //??????????????????
    const int memory_len = reductionBufSize(head);
    CommonHead *common_head = nullptr;

    //????????????????????????memory_len???FixedBuffer
    for(;;) {
        if((common_head = lookupAvailableBuffer(MEMORY_BUFFER_SIGN_FIXED, memory_len, flag, allocFixedBufferLen(), nullptr))) {
            //makeSpaceHead?????????????????????????????????????????????(?????????,??????????????????common_head???use_info????????????)
            makeSpaceHead(common_head->buf, head);
            break;
        } else {
            //????????????
            //??????FixedBuffer
            makeMemoryBuffer(MEMORY_BUFFER_THREAD_PERMIT_CREATE_APPLY, MEMORY_BUFFER_SIGN_DEFAULT_INIT_SIZE, &fixed_info,
                             std::bind((init_func) &MemoryManager::initFixedBuffer, this, std::placeholders::_1));
        }
    }

    return common_head;
}

void MemoryManager::onConstructMemoryBuffer(CommonHead *common_head, int buffer_flag) {
    if(get_flag(buffer_flag, MEMORY_BUFFER_FLAG_MAKE_SPACE_HEAD)){
        //??????
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
 * ??????????????????
 * @return
 */
MemoryBufferHolder MemoryManager::onRequestLoadMemoryBuffer() {
    //???????????????????????????
    return extractLoadMemoryBuffer();
}

/**
 * ??????????????????
 * @param load_buffers
 * @return
 */
void MemoryManager::onResponseMemoryBuffer(MemoryBufferHolder &load_holder) {
    //????????????
//    MemoryBufferList::clear(&load_buffers,
//                            [&](MemoryBufferIterator iterator) -> void {
//                                responseLoadMemoryBuffer(*iterator);
//                            });
    load_holder.onHolderNote([&](MemoryBufferIterator iterator) -> bool {
        responseLoadMemoryBuffer(*iterator); return false;
    });
}

/**
 * ??????????????????(????????????)
 * @param load_buffer
 */
void MemoryManager::responseLoadMemoryBuffer(MemoryBuffer *load_buffer) {
    //??????MemoryBuffer?????????
    load_buffer->sign = memoryBufferUseStatus(load_buffer);
    //???????????????MemoryBuffer
    mountMemoryBuffer(load_buffer, true);
}

