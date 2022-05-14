//
// Created by abc on 20-12-30.
//
#include "../central/time/TimeLayer.h"

TimeManager::~TimeManager() {
    destroyBuffer(time_control, reinterpret_cast<char*>(time_buffer), sizeof(TimeBuffer) * range_size);
}

/**
 * 根据需要创建的数量来创建TimeBuffer缓存
 * @param time_control  控制器（TimeLayer）
 * @param create_size   数量
 * @return 创建的缓存或nullptr
 */
char* TimeManager::createBuffer(TimeLayer *time_control, uint32_t create_size) {
    return time_control->createBuffer(create_size);
}

/**
 * 销毁TimeBuffer缓存
 * @param time_control  控制器（TimeLayer）
 * @param buffer   缓存
 */
void TimeManager::destroyBuffer(TimeLayer *time_control, char *buffer, uint32_t size) {
    time_control->destroyBuffer(buffer, size);
}

/**
 * //重用管理器
 * @param frame     帧数量
 * @param number    提交资源数量
 */
void TimeManager::onReuseManager(uint32_t frame, uint32_t number) {
    //更改数组
    onUpdateSpace(frame);
    //设置提交资源数量
    onNeedSubmitNumber(number);
    //根据first序号更新last序号
    last_sequence.store(first_sequence.load(std::memory_order_acquire), std::memory_order_release);
}

/**
 * //停用管理器
 */
void TimeManager::onUnuseManager() {
    //清空数组
    onUpdateSpace(0);
    //清空提交资源数量
    onNeedSubmitNumber(0);
    //根据first序号更新last序号
    last_sequence.store(first_sequence.load(std::memory_order_acquire), std::memory_order_release);
}

/**
 * 更新TimeBuffer大小
 * @param range     数量
 */
void TimeManager::onUpdateBufferRange(uint32_t range) {
    initTimeBuffer(range);
}

/**
 * 清空存储提交资源数组
 */
void TimeManager::onClearBufferRange() {
    if(time_buffer){
        //清空并销毁提交的资源（SubmitInfo）、存储资源（TimeInfo）及存储资源缓存（TimeBuffer）
        TimeInfo *clear_buffer = nullptr;
        for(int first = first_sequence.load(std::memory_order_consume), last = last_sequence.load(std::memory_order_consume); last < first; last++){
            if((clear_buffer = (time_buffer + size_round_down(last, range_factor))->getTimeInfo()) != &TimeLayer::empty_info){
                unitTimeInfo(time_control, clear_buffer);
            }
        }
        destroyBuffer(time_control, reinterpret_cast<char*>(time_buffer), sizeof(TimeBuffer) * range_size);
    }
    setRangeSize(0);
}

/**
 * 销毁TimeInfo及SubmitInfo
 * @param layer     时间层
 * @param time_info 存储提交资源
 */
void TimeManager::unitTimeInfo(TimeLayer *layer, TimeInfo *time_info) {
    for(uint32_t pos = 0, end = time_info->getSubmitInfoSize(); pos < end; pos++){
        layer->destroySubmitTimeInfo(time_info->getSubmitInfo(pos));
    }
    destroyBuffer(layer, reinterpret_cast<char*>(time_info), sizeof(TimeInfo) + sizeof(SubmitInfo*) * time_info->getAllInfoSize());
}

/**
 * 初始化TimeBuffer缓存
 * @param range     数量
 */
void TimeManager::initTimeBuffer(uint32_t range) {
    TimeBuffer *create_buffer = nullptr;
    //创建新的TimeBuffer缓存
    if((create_buffer = reinterpret_cast<TimeBuffer*>(createBuffer(time_control, sizeof(TimeBuffer) * range)))){
        //初始化
        initTimeBufferToCreate(time_buffer, range);
        if(!time_buffer){
            time_buffer = create_buffer;
            setRangeSize(range);
        }else{
            onTransferTimeBufferWithDestroy(create_buffer, range);
        }
    }
}

void TimeManager::initTimeBufferToCreate(TimeBuffer *create_buffer, uint32_t range) {
    for(int i = 0; i < range; i++){
        new ((void*)(create_buffer + i)) TimeBuffer();
    }
}

/**
 * 根据TimeBuffer创建TimeInfo
 * @param time_buffer 需要创建TimeInfo的TimeBuffer
 */
void TimeManager::initTimeBufferToPosition(TimeBuffer *time_buffer, uint32_t sequence) {
    uint32_t need_number = need_submit_number, need_len = sizeof(TimeInfo) + (sizeof(SubmitInfo*) *need_number);
    TimeInfo *time_info = nullptr;

    //根据需要提交的资源数量创建TimeInfo
    if((time_info = reinterpret_cast<TimeInfo*>(createBuffer(time_control, need_len)))){
        //创建成功,初始化TimeBuffer和TimeInfo
        time_info->initTimeInfo(need_number, sequence);
        new ((void*)time_buffer) TimeBuffer(sequence, time_info);
    }else{
        //创建失败,设置empty_info
        new ((void*)time_buffer) TimeBuffer(sequence, &TimeLayer::empty_info);
    }
}


/**
 * 转移TimeBuffer
 * (1)|------------------|last|-------(cpy_func)------|first|-----------|
 * (2)|-(cpy_func_lower)-|first|----------------------|last|-(cpy_func)-|
 * @param first             转移头
 * @param last              转移尾
 * @param size              转移数量
 * @param cpy_func          转移全部或上部分函数
 * @param cpy_func_lower    转移下部分函数
 */
void TimeManager::transferTimeBuffer(uint32_t first, uint32_t last, uint32_t size,
                                     const std::function<void(uint32_t, uint32_t)> &cpy_func,
                                     const std::function<void(uint32_t, uint32_t)> &cpy_func_lower) {
    if(first < last){
        cpy_func(last, size - last);
        cpy_func_lower(size - last, first + 1);
    }else{
        cpy_func(last, first - last + 1);
    }
}

/**
 * 转移新的TimeBuffer并销毁旧的TimeBuffer
 * @param create_buffer 被转移的TimeBuffer
 * @param range         数量
 */
void TimeManager::onTransferTimeBufferWithDestroy(TimeBuffer *create_buffer, uint32_t range) {
    uint32_t first = 0, last = 0, destroy_size = range_size;

    UniqueDoor transfer_door(&buffer_door);
    buffer_mutex.lock();

    first = first_sequence.load(std::memory_order_consume);
    last = last_sequence.load(std::memory_order_consume);

    if(first > last) {
        TimeBuffer transfer_buffer[range_size];

        transferTimeBuffer(size_round_down(first, range_factor), size_round_down(last, range_factor), range_size,
                           [&](uint32_t position, uint32_t size) -> void{
                                memcpy(transfer_buffer, time_buffer + position, sizeof(TimeBuffer) * size);
                        }, [&](uint32_t position, uint32_t size) -> void{
                                memcpy(transfer_buffer + position, time_buffer, sizeof(TimeBuffer) * size);
                        });
        setRangeSize(range);
        transferTimeBuffer(size_round_down(first, range_factor), size_round_down(last, range_factor), range_factor,
                           [&](uint32_t position, uint32_t size) -> void{
                                memcpy(transfer_buffer + position, transfer_buffer, sizeof(TimeBuffer) * size);
                        }, [&](uint32_t position, uint32_t size) -> void{
                                memcpy(create_buffer, transfer_buffer + position, sizeof(TimeBuffer) * size);
                        });
    }

    destroyBuffer(time_control, reinterpret_cast<char*>(time_buffer), sizeof(TimeBuffer) * destroy_size);
    time_buffer = create_buffer;
}

/**
 * 按序号提交资源
 * @param buffer    资源
 * @param sequence  序号
 * @return
 */
bool TimeManager::onSubmitBuffer(MediaData *submit_data, uint32_t sequence) {
    bool is_submit;
    uint32_t first = 0, last = 0, submit_pos = 0;
    SubmitInfo *submit_info = nullptr;

    if(!(time_control->createSubmitTimeInfo(*submit_data))){
        return false;
    }

    auto submit_func = [&](TimeBuffer *submit_time) -> bool {
        TimeInfo *time_info = submit_time->getTimeInfo();
        /*
         * 判断该序号是否已经被释放
         */
        if(submit_time->onSubmitSequence() == sequence){
            for(;;){
                /*
                 * 获取提交资源位置（多线程竞争）
                 * (2):提交竞争
                 * (3):释放与提交竞争
                 */
                submit_pos = submit_time->onSubmitPosition();

                /*
                 * 提交失败
                 *  1.其他提交线程获取的位置导致资源满载
                 *  2.驱动线程释放该序号
                 */
                if((submit_pos <= 0) || (submit_pos >= time_info->getAllInfoSize())){
                    break;
                }

                if(submit_time->setSubmitPosition(submit_pos)){
                    time_info->setSubmitInfo(submit_info, submit_pos);
                    return true;
                }
            }
        }
        /*
         * 超出提交范围,提交失败或序号已释放(time_buffer的使用序号不是当前提交的序号 ==> use_sequence != sequence)
         *  1:该序号的容纳buffer_size个资源,新的远程地址加入,但序号需要容纳buffer_size + 1个,超出的无法接收
         *  2:申请内存失败,buffer_size为0
         */
        return false;
    };

    buffer_door.pass();
    buffer_mutex.lock_shared();

    //获取当前尾序号和头序号
    first = first_sequence.load(std::memory_order_consume);
    last = last_sequence.load(std::memory_order_consume);

    if(sequence <= last){
        //序号过时,提交失败
        is_submit = false;
    }else{
        if(sequence > first){
            if((sequence - last_sequence) > range_size){
                is_submit = false;
            } else{
                //(1):first序号竞争
                for(;;) {
                    if (first_sequence.compare_exchange_weak(first, sequence, std::memory_order_release, std::memory_order_consume)) {
                        break;
                    }

                    if(first >= sequence){
                        break;
                    }
                }
                is_submit = submit_func(time_buffer + size_round_down(sequence, range_factor));
            }
        }else{
            is_submit = submit_func(time_buffer + size_round_down(sequence, range_factor));
        }
    }

    buffer_mutex.unlock_shared();
    return is_submit;
}

/**
 * 更新尾序号
 *  单线程调用（驱动线程）
 *  序号不会块幅度更新（只会按顺序更新）
 * @param sequence 尾序号
 */
void TimeManager::onUpdateTime(uint32_t sequence) {
    TimeInfo *submit_info = nullptr;
    TimeBuffer *update_buffer = nullptr;

    buffer_door.pass();
    buffer_mutex.lock_shared();

    //这里可以不需要判断,first不会小于等于last
    if((sequence <= first_sequence.load(std::memory_order_consume)) && (sequence > last_sequence.load(std::memory_order_consume))){
        //获取当前尾序号资源
        submit_info = (update_buffer = time_buffer + size_round_down(sequence, range_factor))->getTimeInfo();
        //设置下一个序号
        update_buffer->setSubmitSequence(sequence + range_size);
        //初始化内存(这里的初始化需要修改,应该有提交时才初始化)
        initTimeBufferToPosition(update_buffer, sequence);
        //获取并设置提交资源的数量并清空提交数量
        submit_info->setSubmitInfoSize(update_buffer->updateSubmitPosition());
        //设置尾序号（单线程设置）
        last_sequence.store(sequence, std::memory_order_release);
    }

    buffer_mutex.unlock_shared();

    if(submit_info && (submit_info != &TimeLayer::empty_info)) {
        time_control->submitTimeInfo(submit_info, sequence, [&](TimeLayer *layer, TimeInfo *info) -> void {
                                                                unitTimeInfo(layer, info);
                                                            });
    }
}