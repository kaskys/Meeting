//
// Created by abc on 21-2-13.
//

#include "../BasicControl.h"

//--------------------------------------------------------------------------------------------------------------------//
/*
 * ×××暂未实现×××
 * 该变量只被onDrive函数（LoopThread线程）和onParameter函数（不需要实时数据）调用
 */
static SpecificParameter specific_parameter{};

//该变量只有在ThreadParameter析构函数和onParameter函数的调用
TransmitParameter history_parameter{};

std::mutex history_lock{};
std::atomic<ThreadParameter*> parameter_link{};

//--------------------------------------------------------------------------------------------------------------------//

bool TransmitOutputInfo::isNoteCompile() const {
    return static_cast<bool>((TransmitLayer::onRemoteNoteCompileLen(thread_util, output_note)));
}

void TransmitOutputInfo::onNoteCompileOutput(TransmitAnalysisUtil *analysis_util, MsgHdr *transmit_msg) {
    output_len = TransmitLayer::onNoteCompileOutput(thread_util->getThreadParameter(),
                                                    analysis_util, output_note, transmit_msg, output_msg, msg_len);
    old_hdr_len = output_msg->len_source;
    old_resource_len = output_len - sizeof(MsgHdr) - old_hdr_len;
}

void TransmitOutputInfo::onNoteCompileOutput() {
    if(isNoteCompile()){
        output_len = TransmitLayer::onNoteCompileOutput(thread_util->getThreadParameter(),
                                                        thread_util->getTransmitCompileUtil(MeetingAddressManager::getNotePermitPos(output_note)),
                                                        output_note, output_msg, output_len);
    }

}

void TransmitOutputInfo::onNoteCompileReset() {
    if(output_len > (sizeof(MsgHdr) + old_hdr_len + old_resource_len)){
        memmove(output_msg->buffer + old_hdr_len, output_msg->buffer + output_msg->len_source, static_cast<size_t>(old_resource_len));
        output_msg->len_source = static_cast<uint32_t>(old_hdr_len);
    }
}

void TransmitOutputInfo::onTransmitOutput(const std::function<void(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*, int)> &output_func) {
    //是否特殊输出（地址没有加入）
    bool compile_output = verifyNote();

    //非特殊输出，编译共享类型
    if(compile_output) {
        onNoteCompileOutput();
    }

    output_func(thread_util, output_note, output_msg, output_len);

    if(compile_output) {
        onNoteCompileReset();
    }

}

//--------------------------------------------------------------------------------------------------------------------//

void TransmitErrorUtil::onParameterError(TransmitParameter *transmit_parameter, int type) {
    switch (type){
        case TRANSMIT_PARAMETER_EMPTY_TYPE:
        case TRANSMIT_PARAMETER_UNKNOWN_MASTER:
        case TRANSMIT_PARAMETER_NORESPONSE:
        case TRANSMIT_PARAMETER_MUTEX:
            transmit_parameter->onTypeError();
            break;
        case TRANSMIT_PARAMETER_VERSION:
        case TRANSMIT_PARAMETER_LEN:
        case TRANSMIT_PARAMETER_SOCKADDR:
            transmit_parameter->onParameterError();
            break;
        case TRANSMIT_PARAMETER_UNSOCKET:
            transmit_parameter->onUnSocket();
            break;
        case TRANSMIT_PARAMETER_UNMEMORY:
            transmit_parameter->onUnObtain();
            break;
        case TRANSMIT_PARAMETER_OUTPUT_MAX:
//            transmit_parameter->onUnOuput();
            break;
        default:
            break;
    }

}

void TransmitErrorUtil::onParameterError(TransmitParameter *transmit_parameter, ParameterError *error) {
    onParameterError(transmit_parameter, error->getParameterErrorType());
}

//--------------------------------------------------------------------------------------------------------------------//

BasicLayer* TransmitLayerUtil::createLayer(BasicControl *control) noexcept {
    return (new (std::nothrow) TransmitLayer(control));
}

void TransmitLayerUtil::destroyLayer(BasicControl *, BasicLayer *layer) noexcept {
    delete layer;
}

//--------------------------------------------------------------------------------------------------------------------//

TransmitLayer::TransmitLayer(BasicControl *control) : BasicLayer(control), transmit_repeat_number(0),
                                                      wait_correlate_thread(nullptr), transmit_status(),
                                                      transmit_intercept_info(this) {}

void TransmitLayer::initLayer() {}

void TransmitLayer::onOutput() {}

void TransmitLayer::onInput(MsgHdr*) {}

void TransmitLayer::onNoteTermination(MeetingAddressNote *note) {
    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + sizeof(MeetingAddressNote*),
                                          [&](MsgHdr *termination_msg) -> void {
                                              basic_control->onLayerCommunication(MsgHdrUtil<MeetingAddressNote*>::initMsgHdr(
                                                      termination_msg, sizeof(MeetingAddressNote*), LAYER_CONTROL_NOTE_TERMINATION ,note), LAYER_ADDRESS_TYPE);
                                          });
}

/**
 * 向远程端的输出数据
 * @param thread_util
 * @param output_note
 * @param output_msg
 * @param output_len
 */
void TransmitLayer::onOutputLayer(TransmitOutputInfo *output_info) {
    if(!output_info || output_info->verifyOutput() ||
       !output_info->onTransmitIntercept(&transmit_intercept_info, (bool(*)(TransmitTypeIntercept*, MsgHdr*, MeetingAddressNote*))&TransmitTypeIntercept::callOutputFunc)){
        return;
    }

    output_info->onTransmitOutput(
            [&](TransmitThreadUtil *thread_util, MeetingAddressNote *output_note, MsgHdr *output_msg, int output_len) -> void {
                output_msg->version = static_cast<char>(CONTROL_MSG_VERSION);
                //转换网络序
                TransmitBasicUtil::onCodeTransmitMsgHdr(output_msg);
                //关联SocketThread输出
                thread_util->onSocketOutput(output_msg, output_note, TransmitLayer::onOutputDone, TransmitLayer::onOutputCancel, output_len);
            }
    );


}

void TransmitLayer::onOutputDone(TransmitParameter *parameter, MsgHdr *output_msg) {
    auto output_len = static_cast<uint32_t>(output_msg->serial_number);

    if(output_msg->master_type == LAYER_MASTER_MEDIA){
        parameter->onOutputMedia(output_len);
    }else if(output_msg->shared_type | LAYER_SHARED_TEXT){
        parameter->onOutputText(output_len);
    }else if(output_msg->shared_type | LAYER_SHARED_ERROR){
        parameter->onOutputError(output_len);
    }else {
        parameter->onOutputOther(output_len);
    }
}

void TransmitLayer::onOutputCancel(MsgHdr *output_msg) {
    output_msg->version = 0; output_msg->serial_number = 0;
}

/**
 * 远程端输入数据
 * @param transmit_parameter    传输层参数（线程）
 * @param analysis_util         解析工具（集合）
 * @param input_reader          内存读取器
 * @param addr                  地址
 * @param len                   输入数据长度
 */
void TransmitLayer::onInputLayer(TransmitParameter *transmit_parameter, TransmitThreadUtil *thread_util, TransmitInputData *input_data) {
    try {
        onAnalysisInput(transmit_parameter, thread_util, input_data, &transmit_intercept_info,
                        [&](const sockaddr_in &addr) -> MeetingAddressNote* {
                            return requestControlInput(transmit_parameter, addr);
                        },
                        [&](TransmitInfo *transmit_info) -> void {
                            onReceiveInput(transmit_parameter, thread_util->getTransmitAnalysisUtil(), transmit_info);
                        });
    }catch (std::logic_error &e){
        onError(transmit_parameter, TRANSMIT_PARAMETER_EMPTY_TYPE);
    }
}

/**
 * 解析输入信息
 * @param transmit_parameter    传输层参数（线程）
 * @param input_msg             输入的内存信息
 * @param addr                  输入地址
 * @param len                   输入内存长度
 * @param intercept_func        控制层是否拦截远程端函数
 * @param callback_func         解析成功回调函数
 */
void TransmitLayer::onAnalysisInput(TransmitParameter *transmit_parameter, TransmitThreadUtil *transmit_util, TransmitInputData *input_data,
                                    TransmitTypeIntercept *intercept_info, const std::function<MeetingAddressNote*(const sockaddr_in&)> &match_func,
                                    const std::function<void(TransmitInfo*)> &callback_func) {
    int resource_len = 0;
    MeetingAddressNote *input_note = nullptr;
    MsgHdr input_msg{};

    //解析失败函数
    static auto analysis_error_func = [&](TransmitParameter *parameter, int error_type) -> void {
        onError(parameter, error_type);
    };

    if(input_data->transmit_len < sizeof(MsgHdr)){
        analysis_error_func(transmit_parameter, TRANSMIT_PARAMETER_LEN);
        return;
    }

    {
        //读取MsgHdr
        input_msg = *input_data->store_reader.readMemory<MsgHdr>();
        //转换主机序
        TransmitBasicUtil::onDecodeTransmitMsgHdr(&input_msg);
    }

    //输入版本是否有效
    if(input_msg.version != CONTROL_MSG_VERSION){
        analysis_error_func(transmit_parameter, TRANSMIT_PARAMETER_VERSION);
        return;
    }

    //判断是否有输入类型
    if((!input_msg.master_type) && (!input_msg.shared_type)){
        analysis_error_func(transmit_parameter, TRANSMIT_PARAMETER_EMPTY_TYPE);
        return;
    }

    if((input_msg.len_source + sizeof(MsgHdr)) <= input_data->transmit_len){
        analysis_error_func(transmit_parameter, TRANSMIT_PARAMETER_LEN);
        return;
    }
    resource_len = input_data->transmit_len - sizeof(MsgHdr) - input_msg.len_source;

    //判断输入端口号及地址
    if((input_data->addr.sin_port < IPPORT_RESERVED) || (input_data->addr.sin_addr.s_addr == INADDR_ANY) || (input_data->addr.sin_addr.s_addr == INADDR_NONE)){
        analysis_error_func(transmit_parameter, TRANSMIT_PARAMETER_SOCKADDR);
        return;
    }

    //向控制层请求是否允许该远程端输入
    if(!(input_note = match_func(input_data->addr))){
        analysis_error_func(transmit_parameter, TRANSMIT_PARAMETER_CONTROL_INTERCEPT);
        return;
    }

    //向SocketThread验证是否关联note（加入请求除外）
    if((input_msg.master_type != LAYER_MASTER_JOIN) && !transmit_util->onSocketInputVerify(input_note)){
        analysis_error_func(transmit_parameter, TRANSMIT_PARAMETER_CONTROL_READ_CLOSE);
        return;
    }

    if(!intercept_info->callVerifyFunc(&input_msg, input_note)){
        analysis_error_func(transmit_parameter, TRANSMIT_PARAMETER_CONTROL_VERIFY);
        return;
    }

    {
        TransmitInfo transmit_info{};
        transmit_info.setTotalLen(input_data->transmit_len - sizeof(MsgHdr))
                     .setHdrLen(input_msg.len_source)
                     .setResourceLen(resource_len)
                     .setHdrOffSet(0)
                     .setResourceOffSet(input_msg.len_source)
                     .setInfoMsg(&input_msg)
                     .setInfoReadre(&input_data->store_reader)
                     .setInfoNote(input_note)
                     .setInfoCode(nullptr)
                     .setCopyFunc(nullptr);
        callback_func(&transmit_info);
    }
}

/**
 * 编译输出类型（主类型、响应类型）
 * @param parameter
 * @param analysis_util
 * @param output_note
 * @param compile_msg
 * @param output_msg
 * @param msg_len
 * @return
 */
int TransmitLayer::onNoteCompileOutput(TransmitParameter *parameter, TransmitAnalysisUtil *analysis_util,
                                       MeetingAddressNote *output_note, MsgHdr *compile_msg, MsgHdr *output_msg, int msg_len) {
    uint32_t hdr_len = sizeof(short), resource_len = 0;
    TransmitUtil *transmit_util = nullptr;
    TransmitCodeInfo *code_info = nullptr;

    if(compile_msg->response_type){
        transmit_util = analysis_util->getResponseAnalysisUtil(compile_msg->response_type);
    }else if(compile_msg->master_type){
        transmit_util = analysis_util->getMasterAnalysisUtil(compile_msg->master_type);
        hdr_len = (sizeof(short) * compile_msg->address_number);
        code_info = reinterpret_cast<TransmitCodeInfo*>(compile_msg->buffer);
    }else if(compile_msg->shared_type){
        transmit_util = analysis_util->getSharedAnalysisUtil(compile_msg->shared_type);
    }

    if(!transmit_util){
        return 0;
    }

    transmit_util->onTransmitType()->onInitCompileType(compile_msg->buffer);
    resource_len = static_cast<uint32_t>(transmit_util->onTransmitType()->onLen());
    {
        TransmitInfo compile_info{};
        compile_info.setTotalLen(msg_len - sizeof(MsgHdr))
                    .setHdrLen(hdr_len)
                    .setResourceLen(resource_len)
                    .setHdrOffSet(0)
                    .setResourceOffSet(hdr_len)
                    .setInfoMsg(output_msg)
                    .setInfoReadre(nullptr)
                    .setInfoNote(output_note)
                    .setInfoCode(code_info)
                    .setCopyFunc((void (*)(MsgHdr *, char *, uint32_t, uint32_t))&BasicControl::copyMsgHdr);
        onCompileOutput0(parameter, &compile_info, transmit_util);
        return static_cast<int>(sizeof(MsgHdr) + hdr_len + resource_len);
    }
}

/**
 * 编译共享类型
 * @param parameter
 * @param compile_util
 * @param output_note
 * @param output_msg
 * @param msg_len
 * @return
 */
int TransmitLayer::onNoteCompileOutput(TransmitParameter *parameter, TransmitCompileUtil *compile_util,
                                       MeetingAddressNote *output_note, MsgHdr *output_msg, int msg_len) {
    auto hdr_len = static_cast<uint32_t>(compile_util->onCompileSharedSize()),
         resource_len = static_cast<uint32_t>(compile_util->onCompileResourceLen());

    memmove(output_msg->buffer + output_msg->len_source + hdr_len, output_msg->buffer + output_msg->len_source, hdr_len);

    hdr_len += output_msg->len_source;
    resource_len += (msg_len - sizeof(MsgHdr) - output_msg->len_source);

    {
        short compile_type = compile_util->onCompileSharedType();
        TransmitInfo compile_info{};
        compile_info.setTotalLen(hdr_len + resource_len)
                    .setHdrLen(hdr_len)
                    .setResourceLen(resource_len)
                    .setHdrOffSet(output_msg->len_source)
                    .setResourceOffSet(hdr_len + (msg_len - sizeof(MsgHdr) - output_msg->len_source))
                    .setInfoMsg(output_msg).setInfoReadre(nullptr)
                    .setInfoNote(output_note).setInfoCode(nullptr)
                    .setCopyFunc((void (*)(MsgHdr *, char *, uint32_t, uint32_t)) &BasicControl::copyMsgHdr);

        for(short type_pos = 0, len = sizeof(compile_type) * LAYER_BIT_SIZE; type_pos < len; type_pos++){
            if(!compile_type){
                break;
            }

            compile_type &= (~type_pos);
            onCompileOutput0(parameter, &compile_info, compile_util->transmitSharedType(type_pos));
        }

        return static_cast<int>(sizeof(MsgHdr) + hdr_len + resource_len);
    }
}

void TransmitLayer::onCompileOutput0(TransmitParameter *parameter, TransmitInfo *transmit_info, TransmitUtil *transmit_util) {
    try {
        transmit_util->compileTransmitType(transmit_info);
        transmit_util->onTransmitType()->onTypeCompile();
    }catch (ParameterError &error){
        onError(parameter, &error);
    }
}

/**
 * 共享类型待输出,下一次主类型输出会携带共享类型
 * @param thread_util
 * @param note
 * @param msg
 */
void TransmitLayer::onNoteCompileType(TransmitThreadUtil *thread_util, MeetingAddressNote *note, MsgHdr *msg) {
    TransmitCompileUtil *compile_util = thread_util->getTransmitCompileUtil(MeetingAddressManager::getNotePermitPos(note));
    if(compile_util && msg){
        compile_util->compileSharedType(msg->shared_type, msg->buffer);
    }
}

/**
 * 请求控制层是否允许该远程端输入的数据
 * @param addr  远程端地址
 * @return      本地存储地址
 */
MeetingAddressNote* TransmitLayer::requestControlInput(TransmitParameter *transmit_parameter, sockaddr_in addr) {
    MeetingAddressNote *note = nullptr;

    BasicControl::callOutputOnStackMemory(sizeof(MsgHdr) + std::max(sizeof(TransmitRequestInfo), sizeof(MeetingAddressNote*)),
                                          [&](MsgHdr *request_msg) -> void {
                                              MsgHdrUtil<TransmitRequestInfo>::initMsgHdr(request_msg, sizeof(sockaddr_in), LAYER_CONTROL_REQUEST_ADDRESS_BLACK,
                                                         TransmitRequestInfo{ .request_addr = addr,
                                                                              .request_func = (bool(*)(TransmitLayer*, TransmitThreadUtil*))&TransmitLayer::requestControlThread});

                                              basic_control->onCommonInput(request_msg, CONTROL_INPUT_FLAG_CONTROL);
                                              if(request_msg->serial_number >= sizeof(MeetingAddressNote*)){
                                                  note = reinterpret_cast<MeetingAddressNote*>(request_msg->buffer);
                                              }
                                          });

    transmit_parameter->onInputMatch((note && (static_cast<RemoteNoteInfo*>(note)->getStatusNote() == NOTE_STATUS_REMOTE)));
    return note;
}

/**
 * 接收远程端的输入数据
 * @param transmit_parameter    传输层参数（线程）
 * @param analysis_util         解析工具（集合）
 * @param input_reader          内存读取器
 * @param input_note            地址
 * @param input_len             输入数据长度
 */
void TransmitLayer::onReceiveInput(TransmitParameter *transmit_parameter, TransmitAnalysisUtil *analysis_util, TransmitInfo *transmit_info) {
    /*
     * 解析类型资源失败（主类型、共享类型、响应类型）
     */
    static auto analysis_error_func = [&] (TransmitParameter *parameter) -> void{
        onError(parameter, TRANSMIT_PARAMETER_LEN);
    };

    if(transmit_info->info_msg){
        //响应类型与主类型和共享类型互斥（但如有响应类型置为,那么对应的主类型置为或共享类型必须置为）
        if(transmit_info->info_msg->response_type){
            try {
                //解析响应类型
                onAnalysisType(transmit_parameter, transmit_info, &transmit_intercept_info, transmit_info->info_msg->response_type,
                               [&](short response_pos) -> TransmitUtil* {
                                   return analysis_util->getResponseAnalysisUtil(response_pos);
                               },
                               [&](TransmitParameter *transmit_parameter, TransmitInfo *transmit_info, TransmitUtil *response_util) -> TransmitResponse* {
                                   return onAnalysisResponse(transmit_parameter, transmit_info, response_util);
                               },
                               [&](TransmitType *transmit_type) -> void {
                                   basic_control->onTransmitInput(dynamic_cast<TransmitResponse*>(transmit_type));
                               });
            } catch (AnalysisError &error){
                //解析失败
                analysis_error_func(transmit_parameter);
            }
        }else {
            TransmitUtil *master_util = analysis_util->getMasterAnalysisUtil(transmit_info->info_msg->master_type);
            //判断主类型是否有效
            if (master_util) {
                try {
                    //解析主类型
                    onAnalysisMater(transmit_parameter, transmit_info, master_util,
                                    [&](TransmitMaster *transmit_master) -> void {
                                        basic_control->onTransmitInput(transmit_master);
                                    });
                } catch (AnalysisError &error) {
                    //解析错误
                    analysis_error_func(transmit_parameter);
                }
            } else {
                //无效的主类型
                ParameterError error = ParameterError(TRANSMIT_PARAMETER_UNKNOWN_MASTER, transmit_info);
                onError(transmit_parameter, &error);
            }

            try {
                //解析共享类型
                onAnalysisType(transmit_parameter, transmit_info, &transmit_intercept_info, transmit_info->info_msg->shared_type,
                               [&](short shared_pos) -> TransmitUtil* {
                                   return analysis_util->getSharedAnalysisUtil(shared_pos);
                               },
                               [&](TransmitParameter *transmit_parameter, TransmitInfo *transmit_info, TransmitUtil *shared_util) -> TransmitShared* {
                                   return onAnalysisShared(transmit_parameter, transmit_info, shared_util);
                               },
                               [&](TransmitType *transmit_type) -> void {
                                   basic_control->onTransmitInput(dynamic_cast<TransmitShared*>(transmit_type));
                               });
            } catch (AnalysisError &error) {
                //解析失败
                analysis_error_func(transmit_parameter);
            }
        }
    }
}

/**
 * 解析类型（共享类型和响应类型的公有函数）
 * @param transmit_parameter    传输层参数（线程）
 * @param info                  解析后的输入信息
 * @param analysis_type         解析类型集合
 * @param get_analysis_func     获取指定解析工具函数
 * @param analysis_func         解析函数（共享类型或响应类型）
 * @param callback_func         回调函数
 */
void TransmitLayer::onAnalysisType(TransmitParameter *transmit_parameter, TransmitInfo *info, TransmitTypeIntercept *intercept_type, short analysis_type,
                                   const std::function<TransmitUtil*(short)> &get_analysis_func,
                                   const std::function<TransmitType*(TransmitParameter*, TransmitInfo*, TransmitUtil*)> &analysis_func,
                                   const std::function<void(TransmitType*)> &callback_func) throw(AnalysisError) {
    char buffer[sizeof(MsgHdr) + sizeof(char*)];
    TransmitUtil *transmit_util = nullptr;
    TransmitType *transmit_type = nullptr;
    auto intercept_msg = reinterpret_cast<MsgHdr*>(buffer);

    /*
     * 遍历解析类型的所有可能类型（因为共享类型会包含多个类型）
     */
    for(short type_pos = 0, len = sizeof(analysis_type) * LAYER_BIT_SIZE; type_pos < len; type_pos++){
        //没有需要解析的类型,结束循环
        if(!analysis_type){
            break;
        }

        /*
         * 获取对应解析类型的解析工具,如果没有解析工具则跳过这个解析类型
         * 这里的type不是共享类型或响应类型,而是指向解析工具集合数组的位置
         */
        if((transmit_util = get_analysis_func((type_pos)))){
            //从解析类型集合中取消当前类型
            analysis_type &= (~transmit_util->getTransmitType());

            try {
                //调用解析函数解析当前类型
                transmit_type = analysis_func(transmit_parameter, info, transmit_util);
            }catch (ParameterError &error) {
                //解析失败
                onError(transmit_parameter, &error);
                return;
            }
            //初始化拦截Msg信息
            initInterceptMsg(info->info_msg, intercept_msg);

            if(transmit_type && transmit_type->onTypeIntercept(intercept_type, intercept_msg)){
                //解析成功,调用回调函数
                callback_func(transmit_type);
            }
        }
    }
}

/**
 * 解析主类型
 * @param transmit_parameter    传输层参数（线程）
 * @param info                  解析后的输入信息
 * @param master_util           主类型解析工具
 * @param func                  回调函数
 */
void TransmitLayer::onAnalysisMater(TransmitParameter *transmit_parameter, TransmitInfo *info, TransmitUtil *master_util,
                                    const std::function<void(TransmitMaster*)> &callback_func) throw(AnalysisError){
    char buffer[sizeof(MsgHdr) + sizeof(char*)];

    auto transmit_master = dynamic_cast<TransmitMaster*>(master_util->onTransmitType());
    auto intercept_msg = reinterpret_cast<MsgHdr*>(buffer);

    /*
     * 判断主类型是否过滤
     *  是：直接返回,不用发送给控制层
     *  否：解析对应的主类型（因为一次输入信息只能对应一个主类型）
     */
    if(!transmit_status.onFilterMaster(master_util->getTransmitType())){
        try {
            //解析主类型信息
            master_util->varietyTransmitType(info);
        }catch (ParameterError &error){
            //解析失败
            onError(transmit_parameter, &error);
            return;
        }
        //初始化拦截Msg信息
        initInterceptMsg(info->info_msg, intercept_msg);
        /*
         * 解析成功,将主类型的资源信息从输入信息中提取处理
         */
        transmit_master->onTypeAnalysis();
        transmit_master->onParameter(transmit_parameter);

        if(transmit_master->onTypeIntercept(&transmit_intercept_info, intercept_msg)) {
            //回调解析后函数
            callback_func(transmit_master);
        }
    }
}

/**
 * 解析共享类型
 * @param transmit_parameter    传输层参数（线程）
 * @param info                  解析后的输入信息
 * @param shared_util           共享类型解析工具
 * @return                      传输共享类型信息类
 */
TransmitShared* TransmitLayer::onAnalysisShared(TransmitParameter *transmit_parameter, TransmitInfo *transmit_info,
                                                TransmitUtil *shared_util) throw(ParameterError, AnalysisError){
    //获取传输共享类型信息类
    auto transmit_shared = dynamic_cast<TransmitShared*>(shared_util->onTransmitType());

    /*
     * 判断输入信息是否包含该共享类型及共享类型是否被过滤
     */
    if(shared_util->isContain(transmit_info->info_msg->shared_type) && !transmit_status.onFilterShared(shared_util->getTransmitType())){
        //解析共享信息
        shared_util->varietyTransmitType(transmit_info);
        //解析类型
        transmit_shared->onTypeAnalysis();
        //设置参数
        transmit_shared->onParameter(transmit_parameter);
    }else{
        transmit_shared = nullptr;
    }

    return transmit_shared;
}

/**
 * 解析响应类型
 * @param transmit_parameter    传输层参数（线程）
 * @param info                  解析后的输入信息
 * @param response_util         响应类型解析工具
 * @return                      传输共享类型信息类
 */
TransmitResponse* TransmitLayer::onAnalysisResponse(TransmitParameter *transmit_parameter, TransmitInfo *transmit_info,
                                                    TransmitUtil *response_util) throw(ParameterError, AnalysisError){
    auto transmit_response = dynamic_cast<TransmitResponse*>(response_util->onTransmitType());

    /*
     * 判断输入信息是否包含该响应类型及响应类型是否被过滤
     */
    if(response_util->isContain(transmit_info->info_msg->response_type) && !transmit_status.onFilterResponse(response_util->getTransmitType())){
        //解析响应类型
        response_util->varietyTransmitType(transmit_info);
        //设置参数
        transmit_response->onTypeAnalysis();
        transmit_response->onParameter(transmit_parameter);
    }else{
        transmit_response = nullptr;
    }

    return transmit_response;
}

void TransmitLayer::initInterceptMsg(MsgHdr *src, MsgHdr *intercept_msg) {
    memcpy(intercept_msg, src, sizeof(MsgHdr));
    intercept_msg->master_type = 0;
    intercept_msg->shared_type = 0;
    intercept_msg->response_type = 0;
}

/**
 * 创建内存,根据回调函数及控制层创建动态或固定内存
 * @param transmit_parameter    传输层参数（线程）
 * @param transmit_layer        传输层指针
 * @param create_len            创建长度
 * @param dynamic_func          创建动态内存回调函数
 * @param fixed_func            创建固定内存回调函数
 * @return                      是否创建成功
 */
bool TransmitLayer::onTransmitCreateBuffer(TransmitParameter *transmit_parameter, TransmitLayer *transmit_layer, int create_len,
                                           const std::function<void(InitLocator&)> &dynamic_func, const std::function<void(char*)> &fixed_func) {
    if(!dynamic_func && !fixed_func){
        return false;
    }

    bool is_create = true;

    /*
     * 调用传输层的创建内存函数
     */
    transmit_layer->onCreateBuffer(
            [&](MsgHdr *init_msg) -> void {
                //构造创建信息头
                MsgHdrUtil<char*>::initMsgHdr(init_msg, static_cast<uint32_t>(create_len),
                                                                 dynamic_func ? LAYER_CONTROL_REQUEST_ALLOC_DYNAMIC : LAYER_CONTROL_REQUEST_ALLOC_FIXED, nullptr);
            }, [&](MsgHdr *create_msg) -> void {
                //判断是否创建成功
                if((create_msg->serial_number < std::max(sizeof(InitLocator), sizeof(char*)))){
                    //创建失败
                    is_create = false;
                    onError(transmit_parameter, TRANSMIT_PARAMETER_UNMEMORY);
                }else{
                    /*
                     * 创建成功,判断内存类型
                     */
                    if(create_msg->master_type == LAYER_CONTROL_REQUEST_ALLOC_DYNAMIC){
                        //创建动态内存
                        auto init_locator = reinterpret_cast<InitLocator*>(create_msg->buffer);
                        if(dynamic_func){
                            //回调动态内存
                            dynamic_func(*init_locator);
                        }else{
                            //没有回调函数,释放内存
                            is_create = false;
                            //自动释放内存
                            MemoryReader reader = init_locator->locatorCorrelate(static_cast<uint32_t>(create_len));
                        }
                    }else{
                        //创建固定内存
                        if(fixed_func){
                            //回调固定内存
                            fixed_func(create_msg->buffer);
                        }else{
                            //没有回调函数,释放内存
                            is_create = false;
                            onTransmitDestroyBuffer(transmit_parameter, transmit_layer, create_msg->buffer, create_msg->serial_number);
                        }
                    }
                }
            });

    return is_create;
}

/**
 * 销毁固定内存函数（动态内存自动销毁及释放）
 * @param transmit_layer    传输层指针
 * @param buffer            销毁内存
 */
void TransmitLayer::onTransmitDestroyBuffer(TransmitParameter*, TransmitLayer *transmit_layer, const char *buffer, int len) {
    if(!buffer || (len <= 0)){
        return;
    }

    transmit_layer->onDestroyBuffer(sizeof(char*),
                                    [&](MsgHdr *destroy_msg) -> void {
                                        MsgHdrUtil<const char*>::initMsgHdr(destroy_msg, static_cast<uint32_t>(len),
                                                                            LAYER_CONTROL_REQUEST_DESTROY_FIXED, buffer);
                                    });
}

/**
 * 提取传输层参数
 * @param msg   提取信息头
 */
void TransmitLayer::onParameter(MsgHdr *msg) {
    uint32_t parameter_len = msg->serial_number, parameter_type = static_cast<uint32_t>(msg->shared_type);

    if(parameter_len <= sizeof(ParameterTransmit)){
        //长度不够,提取失败
        parameter_len = 0;
    }else {
        //提取类型
        switch (parameter_type) {
            case TRANSMIT_LAYER_RUNTIME_PARAMETER:
                //运行参数
                onParameterRuntime(reinterpret_cast<ParameterTransmit*>(msg->buffer));
                break;
            case TRANSMIT_LAYER_HISTORY_PARAMETER:
                //历史参数（已退出的线程参数）
                onParameterHistory(reinterpret_cast<ParameterTransmit*>(msg->buffer));
                break;
            case TRANSMIT_LAYER_SPECIFIC_PARAMETER:
                //特殊参数
                onParameterSpecific(reinterpret_cast<ParameterTransmit*>(msg->buffer));
                break;
            default:
                break;
        }
        parameter_len = sizeof(ParameterTransmit);
    }

    msg->serial_number = parameter_len;
}

/**
 * 驱动函数（暂不实现）
 * @param msg
 */
void TransmitLayer::onDrive(MsgHdr*) {}

/**
 * 输入控制函数
 * @param msg   控制信息头
 */
void TransmitLayer::onControl(MsgHdr *msg) {
    uint32_t control_len = msg->serial_number;
    /*
     * 控制信息类型
     */
    switch (msg->master_type){
        case LAYER_CONTROL_STATUS_START:
            //启动传输层
            if((msg->serial_number < sizeof(TransmitInitInfo)) && !onLaunchLayer(reinterpret_cast<TransmitInitInfo*>(msg->buffer))){
                msg->master_type = LAYER_CONTROL_STATUS_THROW;
            }
            break;
        case LAYER_CONTROL_STATUS_STOP:
            //停止传输层
            onStopLayer();
            break;
        case LAYER_CONTROL_EXECUTOR_CREATE_THREAD:
            msg->serial_number = (wait_correlate_thread ? 1 : 0);
            break;
        case LAYER_CONTROL_TRANSMIT_INIT_THREAD:
            onTransmitUtilInit(msg);
            break;
        case LAYER_CONTROL_TRANSMIT_UNINIT_THREAD:
            onTransmitUtilUnInit(msg);
            break;
        case LAYER_CONTROL_TRANSMIT_CORRELATE_THREAD:
            onTransmitUtilCorrelate(msg);
            break;
        case TRANSMIT_LAYER_SET_FILTER:
            //设置过滤类型
            if(control_len >= sizeof(FilterInfo)) { setTransmitFilter(*reinterpret_cast<FilterInfo*>(msg->buffer)); }
            break;
        case TRANSMIT_LAYER_CLR_FILTER:
            //取消过滤类型
            if(control_len >= sizeof(FilterInfo)) { clrTransmitFilter(*reinterpret_cast<FilterInfo*>(msg->buffer)); }
            break;
        case TRANSMIT_LAYER_REPEAT:
            //设置输出失败时重输次数
            if(control_len >= sizeof(uint32_t)) { onTransmitRepeat(*reinterpret_cast<uint32_t*>(msg->buffer)); }
            break;
        case TRANSMIT_LAYER_VERIFY:
            if(control_len >= sizeof(std::function<bool(MsgHdr*, MeetingAddressNote*)>)){
                transmit_intercept_info.setVerifyFunc(*reinterpret_cast<std::function<bool(MsgHdr*, MeetingAddressNote*)>*>(msg->buffer));
            }
            break;
        case TRANSMIT_LAYER_OUTPUT_INTERRCEPT:
            if(control_len >= sizeof(std::function<bool(MsgHdr*, MeetingAddressNote*)>)){
                transmit_intercept_info.setOutputFunc(*reinterpret_cast<std::function<bool(MsgHdr*, MeetingAddressNote*)>*>(msg->buffer));
            }
            break;
        case TRANSMIT_LAYER_MASTER_INTERCEPT:
            if(control_len >= sizeof(InterceptFunc)){
                transmit_intercept_info.setMasterInterceptFunc(msg->master_type, reinterpret_cast<InterceptFunc>(msg->buffer));
            }
        case TRANSMIT_LAYER_SHARED_INTERCEPT:
            if(control_len >= sizeof(InterceptFunc)){
                transmit_intercept_info.setSharedInterceptFunc(msg->master_type, reinterpret_cast<InterceptFunc>(msg->buffer));
            }
            break;
        case TRANSMIT_LAYER_RESPONSE_INTERCEPT:
            if(control_len >= sizeof(InterceptFunc)){
                transmit_intercept_info.setResponseInterceptFunc(msg->master_type, reinterpret_cast<InterceptFunc>(msg->buffer));
            }
            break;
        default:
            break;
    }
}

/**
 * 启动传输层
 */
bool TransmitLayer::onLaunchLayer(TransmitInitInfo *launch_info) {
    if((launch_info->init_address.sin_family != AF_INET)
       || (launch_info->init_address.sin_addr.s_addr == INADDR_NONE)
       || (launch_info->init_address.sin_addr.s_addr == INADDR_ANY)){
        return false;
    }

    //设置状态
    transmit_status.setStatus(TRANSMIT_STATUS_LAUNCH);

    //清空历史参数
    history_parameter.onClear();
    //情况特许参数
    specific_parameter.onClear();
    //释放参数链表
    parameter_link.store(nullptr, std::memory_order_release);

    //设置传输层地址
    transmit_addr = launch_info->init_address;
    transmit_addr.sin_port = 0;

    return true;
}

/**
 * 停止传输层
 */
void TransmitLayer::onStopLayer() {
    //设置状态
    transmit_status.setStatus(TRANSMIT_STATUS_TERMINATE);

    //复位重输次数
    transmit_repeat_number = 0;
    //情况过滤信息
    transmit_status.clear();

    //清空历史参数
    history_parameter.onClear();
    //清空特殊参数
    specific_parameter.onClear();
    //释放参数链表
    parameter_link.store(nullptr, std::memory_order_release);

    memset(&transmit_addr, 0, sizeof(sockaddr_in));
    transmit_intercept_info.onClearIntercept();

    if(wait_correlate_thread){
        onTransmitUtilUnInit(nullptr);
        onTransmitDestroyBuffer(nullptr, this, reinterpret_cast<char*>(wait_correlate_thread), sizeof(TransmitThreadUtil));
        wait_correlate_thread = nullptr;
    }
}

/**
 * 设置输出失败时重输次数
 * @param repeat    重输次数
 */
void TransmitLayer::onTransmitRepeat(uint32_t repeat) {
    transmit_repeat_number = repeat;
}

/**
 * 设置过滤信息
 * @param filter    过滤信息
 */
void TransmitLayer::setTransmitFilter(FilterInfo filter) {
    transmit_status.setFilter(filter);
}

/**
 * 取消过滤信息
 * @param filter    过滤信息
 */
void TransmitLayer::clrTransmitFilter(FilterInfo filter) {
    transmit_status.clrFilter(filter);
}

void TransmitLayer::onTransmitUtilInit(MsgHdr *init_msg) {
    if(!wait_correlate_thread){
        try {
            wait_correlate_thread = new (init_msg->buffer) TransmitThreadUtil(&transmit_addr);
            MsgHdrUtil<uint32_t>::initMsgHdr(init_msg, sizeof(uint32_t), static_cast<uint32_t>(init_msg->master_type), wait_correlate_thread->getThreadCorrelateId());
        }catch (std::runtime_error &e){
            init_msg->serial_number = 0;
        }
    }
}

void TransmitLayer::onTransmitUtilUnInit(MsgHdr *uninit_msg){
    uint32_t correlate_id = 0;
    TransmitThreadUtil *uninit_correlate_thread = nullptr;

    if(uninit_msg){
        uninit_correlate_thread = TransmitThreadUtil::onTransmitThreadUtil(reinterpret_cast<SocketWordThread*>(uninit_msg->buffer));
    }else{
        uninit_correlate_thread = wait_correlate_thread;
    }
    correlate_id = uninit_correlate_thread->getThreadCorrelateId();

    if(uninit_correlate_thread){
        uninit_correlate_thread->~TransmitThreadUtil();
    }

    if(uninit_msg){
        if(wait_correlate_thread){
            MsgHdrUtil<TransmitThreadUtil*>::initMsgHdr(uninit_msg, correlate_id, static_cast<uint32_t>(uninit_msg->master_type), uninit_correlate_thread);
        }else {
            wait_correlate_thread = uninit_correlate_thread;
            uninit_msg->serial_number = 0;
        }
    }
}

void TransmitLayer::onTransmitUtilCorrelate(MsgHdr *correlate_msg) {
    auto correlate_socket_thread = reinterpret_cast<SocketWordThread*>(correlate_msg->buffer);

    wait_correlate_thread->correlateTransmit(this);
    wait_correlate_thread->correlateThread(correlate_socket_thread);

    wait_correlate_thread = nullptr;
}

/**
 * 提取运行参数
 * @param parameter 参数
 */
void TransmitLayer::onParameterRuntime(ParameterTransmit *parameter) {
    TransmitParameter stack_parameter;

    for(ThreadParameter *current_parameter = parameter_link.load(std::memory_order_consume);
                                             current_parameter != nullptr; current_parameter = current_parameter->next){

        stack_parameter = current_parameter->parameter;
        if(!current_parameter->valid.load(std::memory_order_acquire)){
            continue;
        }

        stack_parameter.onCopyData(parameter);
    }
}

/**
 * 提取历史参数
 * @param parameter 参数
 */
void TransmitLayer::onParameterHistory(ParameterTransmit *parameter) {
    std::unique_lock<std::mutex> ulock(history_lock, std::try_to_lock);
    history_parameter.onCopyData(parameter);
}

/**
 * 提取特殊参数
 * @param parameter 参数
 */
void TransmitLayer::onParameterSpecific(ParameterTransmit *parameter) {
    specific_parameter.onCopyData(parameter);
}
