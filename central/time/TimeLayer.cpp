//
// Created by abc on 21-1-5.
//

#include "../BasicControl.h"

BasicLayer* TimerLayerUtil::createLayer(BasicControl *control) noexcept {
    return (new (std::nothrow) TimeLayer(control));
}

void TimerLayerUtil::destroyLayer(BasicControl*, BasicLayer *layer) noexcept {
    delete layer;
}

//--------------------------------------------------------------------------------------------------------------------//

TimeInfo TimeLayer::empty_info{};

TimeLayer::TimeLayer(BasicControl *control) : BasicLayer(control), timeout_threshold(0), delay_info_size(0), delay_time_frame(0),
                                              last_submit_sequence(0), time_parameter(), time_info_lock(), delay_time_info(nullptr),
                                              time_manager(nullptr) {}

TimeLayer::~TimeLayer() {
    onDestroyDelayTimeInfo(delay_time_info, delay_info_size);
}

//不调用
void TimeLayer::initLayer() {}

/**
 * 销毁存储资源的缓存
 * @param time_info     缓存
 * @param destroy_size  数量
 */
void TimeLayer::onDestroyDelayTimeInfo(TimeInfo **delay_info, uint32_t delay_size) {
    if(delay_time_info){
        destroyBuffer(reinterpret_cast<char*>(delay_info), sizeof(TimeInfo*) * delay_size);
    }
}

/**
 * 资源输入
 * @param msg  资源信息
 */
void TimeLayer::onInput(MsgHdr *msg) {
    MediaData *input_data = nullptr;

    /*
     * 判断信息是否有效及将缓存转换成资源
     * 这里的msg->serial_number提交资源的序号,所有不判断长度是否有效(默认有效)
     */
    if(msg && (input_data = reinterpret_cast<MediaData*>(msg->buffer))){
        //向时间管理器提交资源
        if(time_manager->onSubmitBuffer(input_data ,msg->serial_number)){
            //提交存储成功
            time_parameter.setIncreaseStoreNumber(1);
        }else{
            //提交存储失败（超时资源或其他原因）
            time_parameter.setIncreaseThrowNumber(1);
        }
    }
}

/**
 * 输入
 */
void TimeLayer::onOutput() {}

/**
 * 驱动函数
 * @param drive_msg 驱动信息
 */
void TimeLayer::onDrive(MsgHdr *drive_msg) {
    //向时间管理器更新序号
    time_manager->onUpdateTime(drive_msg->serial_number);
}

/**
 * 参数函数
 * @param msg 参数信息(获取时间层参数)
 */
void TimeLayer::onParameter(MsgHdr *msg) {
    TimeLayerParameter *msg_parameter = nullptr;
    /*
     * 参数信息有效
     * 将时间层参数拷贝
     */
    if((msg->serial_number >= sizeof(TimeLayerParameter)) && (msg_parameter = reinterpret_cast<TimeLayerParameter*>(msg->buffer))){
        msg_parameter->setIncreaseStoreNumber(time_parameter.getStoreNumber());
        msg_parameter->setIncreaseThrowNumber(time_parameter.getThrowNumber());
        msg_parameter->setIncreaseTimeOutNumber(time_parameter.getTimeOutNumber());
        msg->serial_number = sizeof(TimeLayerParameter);
    }else{
        msg->serial_number = 0;
    }
}

/**
 * 控制输入
 * @param msg 控制信息
 */
void TimeLayer::onControl(MsgHdr *msg) {
    auto control_len = static_cast<uint32_t>(msg->serial_number),
         control_type = static_cast<uint32_t>(msg->master_type);

    switch (control_type){
        case LAYER_CONTROL_STATUS_STOP:     //内部状态变更
            onStopLayer();
            break;
        case LAYER_CONTROL_STATUS_START:    //内部状态变更
            if(control_len < sizeof(StatusInfo) || !onLaunchLayer()){
                msg->master_type = LAYER_CONTROL_STATUS_THROW;
            }else{
                onTimeStatusStart(reinterpret_cast<StatusInfo*>(msg->buffer));
            }
            break;
        case TIME_LAYER_TIMEOUT_THRESHOLD:  //超时门栏
            if(control_len >= sizeof(uint32_t)){ timeout_threshold = *(reinterpret_cast<uint32_t*>(msg->buffer)); }
            break;
        case TIME_LAYER_DELAY_TIME:         //延迟时间帧
            if(control_len >= sizeof(uint32_t)) { onInitDelayTimeInfo(*(reinterpret_cast<uint32_t*>(msg->buffer))); }
            break;
        case TIME_LAYER_BUFFER_INCREASE:    //地址数量增加
            time_manager->setNeedSubmitNumberIncrease();
            break;
        case TIME_LAYER_BUFFER_REDUCE:      //地址数量减少
            time_manager->setNeedSubmitNumberReduce();
            break;
        case TIME_LAYER_TIMEOUT_FRAME:      //超时帧数量
            if(control_len >= sizeof(uint32_t)) { time_manager->onUpdateSpace(*(reinterpret_cast<uint32_t*>(msg->buffer))); }
        default:
            break;
    }
}

bool TimeLayer::onLaunchLayer() {
    return static_cast<bool>((time_manager = new (std::nothrow) TimeManager(this, 0)));
}

void TimeLayer::onStopLayer() {
    delete time_manager;
    time_manager = nullptr;
    onTimeStatusStop();
}

/**
 * 启动时间层
 * @param status_info
 */
void TimeLayer::onTimeStatusStart(StatusInfo *status_info) {
    //设置参数,构造缓存
    timeout_threshold = status_info->timeout_threshold;
    last_submit_sequence = status_info->last_sequence;
    time_manager->onReuseManager(status_info->timeout_frame, status_info->need_submit);
    onInitDelayTimeInfo(status_info->delay_frame);
}

/**
 * 停止时间层
 */
void TimeLayer::onTimeStatusStop() {
    //复位参数,销毁缓存
    onDestroyDelayTimeInfo(delay_time_info, delay_info_size);
    timeout_threshold = 0;
    delay_info_size = 0;
    delay_time_frame = 0;
    last_submit_sequence = 0;
    time_parameter.clearParameter();
    time_manager->onUnuseManager();
}

/**
 * 构造并初始化延迟资源缓存（时间管理层提交的资源,如果数量未超时延迟的帧数量,则会存储在资源缓存,直到延迟的资源超过缓存）
 * @param delay_frame 延迟帧数量
 */
void TimeLayer::onInitDelayTimeInfo(uint32_t delay_frame) {
    uint32_t destroy_delay_size = 0;
    TimeInfo **init_delay_info = nullptr, **destroy_delay_info = nullptr;

    /*
     * 延迟帧数量与当前的数量比较,如果不等则可能需要构造新的缓存
     * 相等 -> 不改变
     * 不等 -> 与当前缓存大小判断是否需要构造新的缓存
     *          小于 -> 使用原来的缓存,改变延迟帧数量
     *          大于 -> 构造新缓存并拷贝数量,销毁旧缓存,改变延迟帧数量及缓存大小
     */
    if((delay_frame > 0) && (delay_frame != delay_time_frame)){
        if(delay_frame > delay_info_size){
            init_delay_info = reinterpret_cast<TimeInfo**>(createBuffer(sizeof(TimeInfo*) * delay_frame));
        }

        {
            std::unique_lock<std::mutex> ulock(time_info_lock);

            if(init_delay_info) {
                memcpy(init_delay_info, (destroy_delay_info = delay_time_info), sizeof(TimeInfo*) * delay_info_size);

                destroy_delay_size = delay_info_size;
                delay_info_size = delay_frame;
                delay_time_info = init_delay_info;
            }

            delay_time_frame = delay_frame;
        }

        //销毁缓存,如果没有构造新的缓存,则destroy_delay_info = null,函数执行不会销毁
        onDestroyDelayTimeInfo(destroy_delay_info, destroy_delay_size);
    }
}

/**
 * 初始化提交资源信息
 * @param submit_data 资源信息
 * @return            时间层资源信息
 */
SubmitInfo* TimeLayer::createSubmitTimeInfo(const MediaData &submit_data){
    SubmitInfo *info = nullptr;

    if((info = reinterpret_cast<SubmitInfo*>(createBuffer(sizeof(SubmitInfo))))){
        //设置提交资源
        info->setTimeBuffer(submit_data);
    }

    return info;
}

void TimeLayer::destroySubmitTimeInfo(SubmitInfo *info) {
    if(info) { destroyBuffer(reinterpret_cast<char*>(info), sizeof(SubmitInfo)); }
}

/**
 * 创建缓存（1.时间管理器, 2.延迟资源缓存, 3.时间管理器：超时资源缓存,存储资源缓存）
 * @param create_len    长度
 * @return
 */
char* TimeLayer::createBuffer(uint32_t create_len) {
    return onCreateBuffer(create_len);
}

/**
 * 销毁缓存
 * @param buffer        缓存
 * @param destroy_len   长度
 */
void TimeLayer::destroyBuffer(const char *buffer, uint32_t destroy_len) {
    onDestroyBuffer(sizeof(char*),
                    [&](MsgHdr *destroy_msg) -> void {
                        MsgHdrUtil<const char*>::initMsgHdr(destroy_msg, destroy_len, LAYER_CONTROL_REQUEST_DESTROY_FIXED, buffer);
                    });
}

/**
 * 时间管理器按序号提交的资源
 * 提交资源给控制层,先将延迟的资源一次性发送给控制层,让控制层按MeetingAddressNote提供的信息再分一次性或多次发送
 * @param info          时间层资源信息
 * @param sequence      序号
 * @param callback      回调函数
 */
void TimeLayer::submitTimeInfo(TimeInfo *info, uint32_t sequence, const std::function<void(TimeLayer*, TimeInfo*)> &callback) {
#define TIME_SINGLE_SUBMIT_INFO     1
    uint32_t info_size = delay_info_size;

    auto submit_func = [&](TimeInfo *submit_info, uint32_t submit_size, uint32_t master_type) -> void {
        TimeInfo *submit_array[submit_size];

        for(int i = 0; i < submit_size; i++){
            submit_array[i] = (submit_info + i);
        }
        BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(TimeSubmitInfo),
                                              [&](MsgHdr *submit_msg) -> void {
                                                  MsgHdrUtil<TimeSubmitInfo>::initMsgHdr(submit_msg, sizeof(TimeSubmitInfo), master_type,
                                                                                                      TimeSubmitInfo(timeout_threshold, sequence, submit_size, *submit_array, callback));
                                                  basic_control->onInput(submit_msg, nullptr, *this, CONTROL_INPUT_FLAG_INPUT);
                                              });
    };

    /*
     * 判断提交的资源是否超出延迟资源的缓存数量
     *  否 -> 不需要向控制层发送资源
     *  是 -> 向控制层发送全部的延迟资源
     */
    {
        std::unique_lock<std::mutex> ulock(time_info_lock);
        delay_time_info[sequence - last_submit_sequence] = info;
        submit_func(info, TIME_SINGLE_SUBMIT_INFO, LAYER_MASTER_NOTE_DISPLAY);

        if(((sequence - last_submit_sequence) > delay_time_frame) || ((sequence - last_submit_sequence) > info_size)){
            last_submit_sequence = sequence;
            submit_func(*delay_time_info, info_size, LAYER_MASTER_MEDIA);
        }
    }
}
