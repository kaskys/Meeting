//
// Created by abc on 21-2-16.
//
#include "../BasicControl.h"

//显示实例化定义
template class TransmitTypePosition<0x01>;
template class TransmitTypePosition<0x02>;
template class TransmitTypePosition<0x04>;
template class TransmitTypePosition<0x08>;
template class TransmitTypePosition<0x10>;
template class TransmitTypePosition<0x20>;

//--------------------------------------------------------------------------------------------------------------------//

void TransmitTypeIntercept::setMasterInterceptFunc(int master, InterceptFunc master_func) {
    master_intercept[master] = master_func;
}

void TransmitTypeIntercept::setSharedInterceptFunc(int shared, InterceptFunc shared_func) {
    shared_intercept[convertTransmitTypeToPosition(static_cast<short>(shared))] = shared_func;
}

void TransmitTypeIntercept::setResponseInterceptFunc(int response, InterceptFunc response_func) {
    response_intercept[convertTransmitTypeToPosition(static_cast<short>(response))] = response_func;
}

bool TransmitTypeIntercept::callMasterInterceptFunc(int master, MsgHdr *msg) {
    if(!master_intercept[master]){
        return true;
    }else{
        return (*master_intercept[master])(TransmitLayer::onTransmitLayerControl(transmit_layer), msg);
    }
}

bool TransmitTypeIntercept::callSharedInterceptFunc(int shared, MsgHdr *msg) {
    if(!shared_intercept[shared]){
        return true;
    }else {
        return (*shared_intercept[shared])(TransmitLayer::onTransmitLayerControl(transmit_layer), msg);
    }
}

bool TransmitTypeIntercept::callResponseInterceptFunc(int response, MsgHdr *msg) {
    if(!response_intercept[response]){
        return true;
    }else {
        return (*response_intercept[response])(TransmitLayer::onTransmitLayerControl(transmit_layer), msg);
    }
}

//--------------------------------------------------------------------------------------------------------------------//

void TransmitType::onTypeAnalysis() {
    onTypeAnalysis0(readResourceFinish);
}

void TransmitType::onTypeCompile() {
    onTypeCompile0(writeResourceFinish);
}

/**
 * 通用验证解析类型
 * @param info      解析后的输入信息
 * @param func      类型解析回调函数
 */
void TransmitType::onVerifyAnalysis(TransmitInfo *info, const std::function<int()> &func) throw(ParameterError) {
#define VERIFY_ANALYSIS_OFFSET_VALUE    (0x0001)

    short len = 0;
    int func_res = 0;

    //解析错误函数
    static auto parameter_func = [=](TransmitInfo *pinfo, int type) -> void {
        throw ParameterError(type, pinfo);
    };

    //设置输入信息
    setTypeInfo(info);
    /**
     * 判断类型是否有资源信息
     */
    if(isLen()){
        if((info->hdr_offset >= info->hdr_len) || ((info->hdr_len - info->hdr_offset) & VERIFY_ANALYSIS_OFFSET_VALUE)){ //判断长度偏移量是否是2的倍数
            parameter_func(info, TRANSMIT_PARAMETER_LEN);
        }

        //获取资源长度信息
        len = readResourceLen(info);

        //资源是否超出输入信息
        if((info->resource_offset + len) > info->total_len){
            parameter_func(info, TRANSMIT_PARAMETER_LEN);
        }

        //验证类型对应长度和资源信息
        if(onTypeVerify(len)){
            parameter_func(info, TRANSMIT_PARAMETER_LEN);
        }
    }

    //回调类型解析函数
    if(func && (func_res = func())){
        parameter_func(info, func_res);
    }
}

/**
 * 通用验证编译类型
 * @param info      类型信息类
 * @param func      类型编译回调函数
 */
void TransmitType::onVerifyCompile(TransmitInfo *info, const std::function<int()> &func) throw(ParameterError) {
#define VERIFY_ANALYSIS_OFFSET_VALUE    (0x0001)
    short len = 0;
    int func_res = 0;

    static auto parameter_func = [=](TransmitInfo *pinfo, int type) -> void {
        throw ParameterError(type, pinfo);
    };

    //设置输出信息
    setTypeInfo(info);

    if(isLen()){
        if((info->hdr_offset >= info->hdr_len) || ((info->hdr_len - info->hdr_offset) & VERIFY_ANALYSIS_OFFSET_VALUE)){
            parameter_func(info, TRANSMIT_PARAMETER_LEN);
        }

        //获取写入资源长度信息
        len = writeResourceLen(this);

        if((info->resource_offset + len) > info->total_len){
            parameter_func(info, TRANSMIT_PARAMETER_LEN);
        }
    }

    if(func && (func_res = func())){
        parameter_func(info, func_res);
    }
}

/**
 * 读取资源长度信息
 * @param info  解析后的输入信息
 * @return      资源长度
 */
short TransmitType::readResourceLen(TransmitInfo *info) {
    return static_cast<short>(TransmitBasicUtil::onDecodeNetWord(static_cast<uint16_t>(*reinterpret_cast<short*>(info->info_msg->buffer + info->hdr_offset))));
}

/**
 * 发送资源长度信息
 * @param type  发送类型
 * @return      类型资源长度
 */
short TransmitType::writeResourceLen(TransmitType *type) {
    return type->onLen();
}

/**
 * 读取资源缓存信息
 * @param info  解析后的输入信息
 * @return      资源缓存
 */
char* TransmitType::readResourceBuffer(TransmitInfo *info) {
    return (info->info_msg->buffer + info->resource_offset);
}

//--------------------------------------------Master------------------------------------------------------------------//

bool TransmitMaster::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    intercept_msg->master_type = static_cast<char>(onType());
    return intercept_info->callMasterInterceptFunc(onType(), intercept_msg);
}

/**
 * 加入类型参数
 * @param parameter 传输层参数（线程）
 */
void TransmitJoin::onParameter(TransmitParameter *parameter) {
    parameter->onInputJoin(1, 0);
    parameter->onInputOther();
}

void TransmitJoin::onMasterInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitMaster<TransmitJoin>(this);
}

void TransmitJoin::onTypeCompile0(void (*)(TransmitType *, short)) {
    type_info->info_msg->master_type = LAYER_MASTER_JOIN;
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 链接类型参数
 * @param parameter 传输层参数（线程）
 */
void TransmitLink::onParameter(TransmitParameter *parameter) {
    parameter->onInputLink();
    parameter->onInputOther();
}

void TransmitLink::onMasterInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitMaster<TransmitLink>(this);
}

void TransmitLink::onTypeCompile0(void (*)(TransmitType *, short)) {
    type_info->info_msg->master_type = LAYER_MASTER_LINK;
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 退出类型参数
 * @param parameter 传输层参数（线程）
 */
void TransmitExit::onParameter(TransmitParameter *parameter) {
    parameter->onInputExit(1, 0);
    parameter->onInputOther();
}

void TransmitExit::onMasterInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitMaster<TransmitExit>(this);
}

void TransmitExit::onTypeCompile0(void (*)(TransmitType *, short)) {
    type_info->info_msg->master_type = LAYER_MASTER_EXIT;
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 音视频资源解析
 */
void TransmitMedia::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)) {
    short analysis_media_len = 0, total_media_len = 0;

    onFixedMediaInfo();
    //遍历地址数量
    for(int i = 0; i < (media_size = type_info->info_msg->address_number); i++){
        //解析音视频信息中的资源长度
        analysis_media_len = readResourceLen(type_info);
        total_media_len += analysis_media_len;
        callback_func(this, analysis_media_len);
    }

    media_len = total_media_len;
}

void TransmitMedia::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    uint32_t note_size = static_cast<uint32_t>(type_info->info_msg->address_number),
             compile_remain_len = static_cast<uint32_t>(type_info->total_len - type_info->resource_offset),
             status_remain_len = note_size * sizeof(TransmitNoteStatus);
    NoteMediaInfo media_info{0, 0, TransmitNoteStatus(), nullptr };

    if((note_size <= 0) || !type_info->code_info || (compile_remain_len < status_remain_len)){
        return;
    }

    for(uint32_t pos = 0, copy_len = 0; pos < note_size; pos++){
        TransmitBasicUtil::onCodeTransmitMedia(type_info, &media_info, pos);
        type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(&media_info.note_status),
                             static_cast<uint32_t>(type_info->resource_offset), (copy_len = sizeof(TransmitNoteStatus)));

        if(status_remain_len > (compile_remain_len - media_info.media_len)){
            media_info.media_data = nullptr;
        }

        if(media_info.media_data){
            if(media_info.media_offset) {
                type_info->copy_func(type_info->info_msg, media_info.media_data->media_reader.readMemory<MsgHdr>()->buffer + media_info.media_offset,
                                     type_info->resource_offset + copy_len, static_cast<uint32_t>(media_info.media_len));
            }else{
                type_info->copy_func(type_info->info_msg, media_info.media_data->media_reader.readMemory<MsgHdr>()->buffer,
                                     type_info->resource_offset + copy_len, static_cast<uint32_t>(media_info.media_len));
            }
            copy_len += media_len;
        }

        callback_func(this, static_cast<short>(copy_len));
        status_remain_len -= sizeof(TransmitNoteStatus);
        compile_remain_len -= copy_len;
    }

    type_info->info_msg->serial_number = type_info->code_info->transmit_sequence;
    type_info->info_msg->master_type = LAYER_MASTER_MEDIA;
}

void TransmitMedia::onFixedMediaInfo() {
    media_len_offset = static_cast<short>(type_info->hdr_offset);
    media_buffer_offset = static_cast<short>(type_info->resource_offset);
}

/**
 * 音视频资源解析实现函数
 * @return  音视频资源长度
 */
void TransmitMedia::onAnalysisMedia(MediaData *media_data, NoteMediaInfo *media_info, int note_size) {
    NoteMediaInfo *note_media_info = nullptr;

    for(int pos = 0; pos < note_size; pos++){
        note_media_info = (media_info + pos);
        note_media_info->media_len = *reinterpret_cast<short *>((media_data->media_reader.readMemory<MsgHdr>()->buffer + media_data->media_len_offset));
        note_media_info->media_offset = media_data->media_buffer_offset;
        note_media_info->note_status = *reinterpret_cast<TransmitNoteStatus*>(media_data->media_reader.readMemory<MsgHdr>()->buffer + note_media_info->media_offset);

        TransmitBasicUtil::onDecodeTransmitMedia(note_media_info);

        note_media_info->media_len -= sizeof(TransmitNoteStatus);
        note_media_info->media_offset += sizeof(TransmitNoteStatus);
        note_media_info->media_data = media_data;

        media_data->media_len_offset += sizeof(short);
        media_data->media_buffer_offset += (note_media_info->media_len + sizeof(TransmitNoteStatus));
    }
}

/**
 * 验证长度信息及地址数量
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitMedia::onTypeVerify(short len) {
    //无效长度或无效地址数量
    return ((len <= 0) || (type_info->info_msg->address_number <= 0));
}

/**
 * 音视频类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitMedia::onParameter(TransmitParameter *parameter) {
    parameter->onInputMedia(static_cast<uint32_t>(media_len));
}

void TransmitMedia::onMasterInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitMaster<TransmitMedia>(this);
}

void TransmitMedia::onInitCompileType(char *buffer) {
    auto compile_info = reinterpret_cast<TransmitCodeInfo*>(buffer);
    media_len = static_cast<short>(compile_info->media_len);
    media_size = static_cast<short>(compile_info->media_size);
    media_len_offset = 0; media_buffer_offset = 0;
}

MediaData TransmitMedia::getMediaData(){
    using tm_func = void(*)(MediaData *media_data, NoteMediaInfo *media_info, int note_size);
    return MediaData{ media_size, media_len_offset, media_buffer_offset,
                      getTransmitNote(), *type_info->info_reader,
                      std::bind((tm_func)&TransmitMedia::onAnalysisMedia, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) };
}

//--------------------------------------------Shared------------------------------------------------------------------//

bool TransmitShared::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    intercept_msg->shared_type = onType();
    return intercept_info->callSharedInterceptFunc(actual_shared_pos, intercept_msg);
}

/**
 * 同步类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitSychro::onParameter(TransmitParameter *parameter) {
    parameter->onInputSynchro(1, 0);
    parameter->onInputOther(sizeof(SynchroData));
}

bool TransmitSychro::onTypeIntercept(TransmitTypeIntercept *intercept_fun, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &synchro_data, sizeof(SynchroData*));
    return TransmitShared::onTypeIntercept(intercept_fun, intercept_msg);
}

void TransmitSychro::onSharedInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitShared<TransmitSychro>(this);
}

/**
 * 同步资源解析
 * @param callback_func
 */
void TransmitSychro::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)) {
    //获取同步信息
    TransmitBasicUtil::onDecodeTransmitSynchro(&(synchro_data = *reinterpret_cast<SynchroData*>(readResourceBuffer(type_info))));
    callback_func(this, readResourceLen(type_info));
}

void TransmitSychro::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    SynchroData data = synchro_data;
    TransmitBasicUtil::onCodeTransmitSynchro(&data);

    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(&data), static_cast<uint32_t>(type_info->resource_offset), sizeof(SynchroData));

    callback_func(this, onLen());
    type_info->info_msg->shared_type |= LAYER_SHARED_SYNCHRO;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitSychro::onTypeVerify(short len) {
    return (len < sizeof(synchro_data));
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 序号类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitSequence::onParameter(TransmitParameter *parameter) {
    parameter->onInputSequence();
    parameter->onInputOther(sizeof(SequenceData));
}

bool TransmitSequence::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &sequence_data, sizeof(SequenceData*));
    return TransmitShared::onTypeIntercept(intercept_info, intercept_msg);
}

void TransmitSequence::onSharedInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitShared<TransmitSequence>(this);
}

/**
 * 序号类型资源解析
 */
void TransmitSequence::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)){
    //获取序号信息
    TransmitBasicUtil::onDecodeTransmitSequence(
                                    &(sequence_data = *reinterpret_cast<SequenceData*>(readResourceBuffer(type_info))));
    callback_func(this, readResourceLen(type_info));
}

void TransmitSequence::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    SequenceData data = sequence_data;
    TransmitBasicUtil::onCodeTransmitSequence(&data);

    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(&data), static_cast<uint32_t>(type_info->resource_offset), sizeof(SequenceData));
    callback_func(this, sizeof(SequenceData));
    type_info->info_msg->shared_type |= LAYER_SHARED_SEQUENCE;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitSequence::onTypeVerify(short len) {
    return (len < sizeof(SequenceData));
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 转移类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitTransfer::onParameter(TransmitParameter *parameter) {
    parameter->onInputOther(sizeof(transfer_addr));
}

bool TransmitTransfer::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &transfer_addr, sizeof(sockaddr_in*));
    return TransmitShared::onTypeIntercept(intercept_info, intercept_msg);
}

void TransmitTransfer::onSharedInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitShared<TransmitTransfer>(this);
}

/**
 * 转移类型资源解析
 */
void TransmitTransfer::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)){
    //获取转移地址
    TransmitBasicUtil::onDecodeTransmitTransfer(
                                     &(transfer_addr = *reinterpret_cast<sockaddr_in*>(readResourceBuffer(type_info))));
    callback_func(this, readResourceLen(type_info));
}

void TransmitTransfer::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    sockaddr_in data = transfer_addr;
    TransmitBasicUtil::onCodeTransmitTransfer(&data);

    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(&data), static_cast<uint32_t>(type_info->resource_offset), sizeof(sockaddr_in));
    callback_func(this, sizeof(sockaddr_in));
    type_info->info_msg->shared_type |= LAYER_SHARED_TRANSFER;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitTransfer::onTypeVerify(short len) {
    return (len < sizeof(sockaddr_in));
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 查询类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitQuery::onParameter(TransmitParameter *parameter) {
    parameter->onInputQuery(1, 0);
    parameter->onInputOther(sizeof(query_type));
}

bool TransmitQuery::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &query_type, sizeof(int*));
    return TransmitShared::onTypeIntercept(intercept_info, intercept_msg);
}

void TransmitQuery::onSharedInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitShared<TransmitQuery>(this);
}

/**
 * 查询类型资源解析
 */
void TransmitQuery::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)){
    //获取查询类型
    query_type = TransmitBasicUtil::onDecodeNetWord(*reinterpret_cast<uint32_t*>(readResourceBuffer(type_info)));
    callback_func(this, readResourceLen(type_info));
}

void TransmitQuery::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    uint32_t type = TransmitBasicUtil::onCodeNetWord(query_type);

    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(&type), static_cast<uint32_t>(type_info->resource_offset), sizeof(uint32_t));
    callback_func(this, sizeof(uint32_t));
    type_info->info_msg->shared_type |= LAYER_SHARED_QUERY;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitQuery::onTypeVerify(short len) {
    return (len < sizeof(int));
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 文本类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitText::onParameter(TransmitParameter *parameter) {
    parameter->onInputText(static_cast<uint32_t>(text_data.info_len));
}

bool TransmitText::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &text_data, sizeof(InfoData*));
    return TransmitShared::onTypeIntercept(intercept_info, intercept_msg);
}

void TransmitText::onSharedInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitShared<TransmitText>(this);
}

/**
 * 文本类型资源解析
 */
void TransmitText::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)){
    //获取文本资源长度
    text_data.info_len = readResourceLen(type_info);
    //获取文本资源缓存在内存中的偏移量
    text_data.info_offset = static_cast<short>(TransmitBasicUtil::onDecodeNetWord(static_cast<uint16_t>(type_info->resource_offset)));
    callback_func(this, static_cast<short>(text_data.info_len));
}

void TransmitText::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, text_data.info_buffer, static_cast<uint32_t>(type_info->resource_offset), static_cast<uint32_t>(text_data.info_len));
    callback_func(this, static_cast<short>(text_data.info_len));
    type_info->info_msg->shared_type |= LAYER_SHARED_TEXT;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitText::onTypeVerify(short len) {
    return (len <= 0);
}

ResourceData TransmitText::getTextData() {
    return ResourceData{ static_cast<short>(text_data.info_len),
                         static_cast<short>(text_data.info_offset),
                         *type_info->info_reader };
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 错误类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitError::onParameter(TransmitParameter *parameter) {
    parameter->onInputError(sizeof(error_type));
}

bool TransmitError::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &error_type, sizeof(int*));
    return TransmitShared::onTypeIntercept(intercept_info, intercept_msg);
}

void TransmitError::onSharedInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitShared<TransmitError>(this);
}

/**
 * 错误类型资源解析
 */
void TransmitError::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)){
    //获取错误类型
    error_type = TransmitBasicUtil::onDecodeNetWord(*reinterpret_cast<uint32_t*>(readResourceBuffer(type_info)));
    callback_func(this, readResourceLen(type_info));
}

void TransmitError::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    uint32_t type = TransmitBasicUtil::onCodeNetWord(error_type);

    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(&type), static_cast<uint32_t>(type_info->resource_offset), sizeof(uint32_t));
    callback_func(this, sizeof(uint32_t));
    type_info->info_msg->shared_type |= LAYER_SHARED_ERROR;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitError::onTypeVerify(short len) {
    return (len < sizeof(int));
}

//--------------------------------------------Response----------------------------------------------------------------//

bool TransmitResponse::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    intercept_msg->response_type = onType();
    return intercept_info->callResponseInterceptFunc(actual_response_pos, intercept_msg);
}

/**
 * 加入响应类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitRJoin::onParameter(TransmitParameter *parameter) {
    parameter->onInputJoin(1, 1);
    parameter->onInputOther();
}

void TransmitRJoin::onResponseInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitResponse<TransmitRJoin>(this);
}
void TransmitRJoin::onTypeCompile0(void (*)(TransmitType *, short)){
    type_info->info_msg->response_type |= LAYER_RESPONSE_JOIN;
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 同步响应类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitRSychro::onParameter(TransmitParameter *parameter) {
    parameter->onInputSynchro(1, 1);
    parameter->onInputOther(sizeof(SynchroData));
}

bool TransmitRSychro::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &synchro_data, sizeof(SynchroData*));
    return TransmitResponse::onTypeIntercept(intercept_info, intercept_msg);
}

void TransmitRSychro::onResponseInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitResponse<TransmitRSychro>(this);
}

/**
 * 解析同步资源
 * @param callback_func 回调函数
 */
void TransmitRSychro::onTypeAnalysis0(void (*callback_func)(TransmitType *, short)) {
    TransmitBasicUtil::onDecodeTransmitSynchro(
                                      &(synchro_data = *reinterpret_cast<SynchroData*>(readResourceBuffer(type_info))));
    callback_func(this, readResourceLen(type_info));
}

void TransmitRSychro::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    SynchroData data = synchro_data;
    TransmitBasicUtil::onCodeTransmitSynchro(&data);

    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(&data), static_cast<uint32_t>(type_info->resource_offset), sizeof(SynchroData));
    callback_func(this, sizeof(SynchroData));
    type_info->info_msg->response_type |= LAYER_RESPONSE_SYNCHRO;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitRSychro::onTypeVerify(short len) {
    return (len < sizeof(SynchroData));
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 传输响应类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitRTransfer::onParameter(TransmitParameter *parameter) {
    parameter->onInputOther();
}

void TransmitRTransfer::onResponseInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitResponse<TransmitRTransfer>(this);
}

void TransmitRTransfer::onTypeCompile0(void (*)(TransmitType *, short)){
    type_info->info_msg->response_type |= LAYER_RESPONSE_TRANSFER;
}

/**
 * 状态响应类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitRStatus::onParameter(TransmitParameter *parameter) {
    parameter->onInputOther(sizeof(status_sequence));
}

bool TransmitRStatus::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &status_sequence, sizeof(int*));
    return TransmitResponse::onTypeIntercept(intercept_info, intercept_msg);
}

void TransmitRStatus::onResponseInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitResponse<TransmitRStatus>(this);
}

/**
 * 状态响应类型资源解析
 */
void TransmitRStatus::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)) {
    //获取响应状态序号
    status_sequence = TransmitBasicUtil::onDecodeNetWord(*reinterpret_cast<uint32_t*>(readResourceBuffer(type_info)));
    callback_func(this, readResourceLen(type_info));
}

void TransmitRStatus::onTypeCompile0(void (*callback_func)(TransmitType*, short)) {
    uint32_t status = TransmitBasicUtil::onCodeNetWord(status_sequence);

    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(&status), static_cast<uint32_t>(type_info->resource_offset), sizeof(uint32_t));
    callback_func(this, sizeof(uint32_t));
    type_info->info_msg->response_type |= LAYER_RESPONSE_STATUS;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitRStatus::onTypeVerify(short len) {
    return (len < sizeof(status_sequence));
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 查询响应类型参数
 * @param parameter 传输层参数（线性）
 */
void TransmitRQuery::onParameter(TransmitParameter *parameter) {
    parameter->onInputQuery(1, 1);
    parameter->onInputOther(static_cast<uint32_t>(query_data.info_len));
}

bool TransmitRQuery::onTypeIntercept(TransmitTypeIntercept *intercept_info, MsgHdr *intercept_msg) {
    memcpy(intercept_msg->buffer, &query_data, sizeof(InfoData*));
    return TransmitResponse::onTypeIntercept(intercept_info, intercept_msg);
}

void TransmitRQuery::onResponseInput(TransmitInputInfo *input_info) {
    input_info->onControlInputTransmitResponse<TransmitRQuery>(this);
}

/**
 * 查询响应类型资源解析
 */
void TransmitRQuery::onTypeAnalysis0(void (*callback_func)(TransmitType*, short)){
    //获取查询资源长度
    query_data.info_len = readResourceLen(type_info);
    //获取查询资源缓存的偏移量
    query_data.info_offset = static_cast<short>(type_info->resource_offset);

    callback_func(this, static_cast<short>(query_data.info_len));
}

void TransmitRQuery::onTypeCompile0(void (*callback_func)(TransmitType *, short)) {
    type_info->copy_func(type_info->info_msg, reinterpret_cast<char*>(onLen()), static_cast<uint32_t>(type_info->hdr_offset), sizeof(short));
    type_info->copy_func(type_info->info_msg, query_data.info_buffer, static_cast<uint32_t>(type_info->resource_offset), static_cast<uint32_t>(query_data.info_len));
    callback_func(this, static_cast<short>(query_data.info_len));
    type_info->info_msg->response_type |= LAYER_RESPONSE_QUERY;
}

/**
 * 验证长度信息
 * @param len   长度信息
 * @return      是否有效
 */
bool TransmitRQuery::onTypeVerify(short len) {
    return (len <= 0);
}

ResourceData TransmitRQuery::getQueryData() {
    return ResourceData{ static_cast<short>(query_data.info_len),
                         static_cast<short>(query_data.info_offset),
                         *type_info->info_reader };
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

char TransmitUtil::onMasterType(MsgHdr *msg) {
    return msg->master_type;
}

short TransmitUtil::onSharedType(MsgHdr *msg) {
    return msg->shared_type;
}

short TransmitUtil::onResponseType(MsgHdr *msg) {
    return msg->response_type;
}

//--------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------网络编解码工具----------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

void TransmitBasicUtil::onCodeTransmitMsgHdr(MsgHdr *msg) {
    msg->shared_type = static_cast<short>(onCodeNetWord(static_cast<uint16_t>(msg->shared_type)));
    msg->response_type = static_cast<short>(onCodeNetWord(static_cast<uint16_t>(msg->response_type)));
    msg->address_number = static_cast<short>(onCodeNetWord(static_cast<uint16_t>(msg->address_number)));
    msg->len_source = onCodeNetWord(msg->len_source);
    msg->serial_number = onCodeNetWord(msg->serial_number);
}

void TransmitBasicUtil::onDecodeTransmitMsgHdr(MsgHdr *msg) {
    msg->shared_type = static_cast<short>(onDecodeNetWord(static_cast<uint16_t>(msg->shared_type)));
    msg->response_type = static_cast<short>(onDecodeNetWord(static_cast<uint16_t>(msg->response_type)));
    msg->address_number = static_cast<short>(onDecodeNetWord(static_cast<uint16_t>(msg->address_number)));
    msg->len_source = onDecodeNetWord(msg->len_source);
    msg->serial_number = onDecodeNetWord(msg->serial_number);
}

void TransmitBasicUtil::onCodeNetWordForNoteStatus(TransmitNoteStatus *status) {
    status->note_status = onCodeNetWord(status->note_status);
    status->note_position = onCodeNetWord(status->note_position);
    status->note_address = onCodeNetWord(status->note_address);
}

void TransmitBasicUtil::onDecodeNetWordForNoteStatus(TransmitNoteStatus *status) {
    status->note_status = onDecodeNetWord(status->note_status);
    status->note_position = onDecodeNetWord(status->note_position);
    status->note_address = onDecodeNetWord(status->note_address);
}

void TransmitBasicUtil::onCodeTransmitMedia(TransmitInfo *info, NoteMediaInfo *media_info, uint32_t note_pos) {
#define MASTER_ANALYSIS_SINGLE_VALUE    1

    uint32_t media_pos = 0;
    MediaData *media_data = nullptr;
    TransmitCodeInfo *code_info = info->code_info;
    MeetingAddressNote *media_note = code_info->note_func(note_pos);

    if ((media_pos = (*code_info->search_func)(code_info, media_note)) < code_info->media_size) {
        media_data = (code_info->media_data + media_pos);
        media_data->analysis_media_func(media_data, media_info, MASTER_ANALYSIS_SINGLE_VALUE);
    } else {
        media_info->media_len = 0;
        media_info->media_data = nullptr;
    }

    media_info->note_status = (*code_info->status_func)(media_note);
    if(code_info->is_ignore_note && (info->transmit_note == media_note)){
        media_info->note_status.note_position = 0;
    }

    onCodeNetWordForNoteStatus(&media_info->note_status);
}

void TransmitBasicUtil::onDecodeTransmitMedia(NoteMediaInfo *media_info) {
    media_info->media_len = static_cast<short>(onDecodeNetWord(static_cast<uint16_t>(media_info->media_len)));
    onDecodeNetWordForNoteStatus(&media_info->note_status);
}

void TransmitBasicUtil::onCodeTransmitSynchro(SynchroData *synchro_data) {
    synchro_data->synchro_send_time = onCodeNetWord(synchro_data->synchro_send_time);
    synchro_data->synchro_recv_time = onCodeNetWord(synchro_data->synchro_recv_time);
}

void TransmitBasicUtil::onDecodeTransmitSynchro(SynchroData *synchro_data) {
    synchro_data->synchro_send_time = onDecodeNetWord(synchro_data->synchro_send_time);
    synchro_data->synchro_recv_time = onDecodeNetWord(synchro_data->synchro_recv_time);
}

void TransmitBasicUtil::onCodeTransmitSequence(SequenceData *sequence_data) {
    sequence_data->link_rtt = onCodeNetWord(sequence_data->link_rtt);
    sequence_data->link_frame = onCodeNetWord(sequence_data->link_frame);
    sequence_data->link_delay = onCodeNetWord(sequence_data->link_delay);
    sequence_data->serial_number = onCodeNetWord(sequence_data->serial_number);
}

void TransmitBasicUtil::onDecodeTransmitSequence(SequenceData *sequence_data) {
    sequence_data->link_rtt = onDecodeNetWord(sequence_data->link_rtt);
    sequence_data->link_frame = onDecodeNetWord(sequence_data->link_frame);
    sequence_data->link_delay = onDecodeNetWord(sequence_data->link_delay);
    sequence_data->serial_number = onDecodeNetWord(sequence_data->serial_number);
}

void TransmitBasicUtil::onCodeTransmitTransfer(sockaddr_in *addr) {
    addr->sin_port = onCodeNetWord(addr->sin_port);
    addr->sin_addr.s_addr = onCodeNetWord(addr->sin_addr.s_addr);
}

void TransmitBasicUtil::onDecodeTransmitTransfer(sockaddr_in *addr) {
    addr->sin_port = onDecodeNetWord(addr->sin_port);
    addr->sin_addr.s_addr = onDecodeNetWord(addr->sin_addr.s_addr);
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * 传输层解析工具集合构造函数
 *  创建所有主类型、共享类型、响应类型工具
 */
TransmitAnalysisUtil::TransmitAnalysisUtil() : TransmitBasicUtil() {
    memset(analysis_shared_type, 0, sizeof(TransmitShared*) * TRANSMIT_SHARED_TYPE_SIZE);
    memset(analysis_response_type, 0, sizeof(TransmitResponse*) * TRANSMIT_RESPONSE_TYPE_SIZE);

    createMasterUtil<TransmitJoin>(&analysis_master_type, LAYER_MASTER_JOIN);
    createMasterUtil<TransmitLink>(&analysis_master_type, LAYER_MASTER_LINK, LAYER_SHARED_SYNCHRO | LAYER_SHARED_SEQUENCE);
    createMasterUtil<TransmitMedia>(&analysis_master_type, LAYER_MASTER_MEDIA);
    createMasterUtil<TransmitExit>(&analysis_master_type, LAYER_MASTER_EXIT);

    createSharedUtil<TransmitSychro>(analysis_shared_type, LAYER_SHARED_SYNCHRO);
    createSharedUtil<TransmitSequence>(analysis_shared_type, LAYER_SHARED_SEQUENCE);
    createSharedUtil<TransmitTransfer>(analysis_shared_type, LAYER_SHARED_TRANSFER);
    createSharedUtil<TransmitQuery>(analysis_shared_type, LAYER_SHARED_QUERY);
    createSharedUtil<TransmitText>(analysis_shared_type, LAYER_SHARED_TEXT);
    createSharedUtil<TransmitError>(analysis_shared_type, LAYER_SHARED_ERROR);

    createResponseUtil<TransmitRJoin>(analysis_response_type, LAYER_RESPONSE_JOIN, LAYER_MASTER_JOIN);
    createResponseUtil<TransmitRSychro>(analysis_response_type, LAYER_RESPONSE_SYNCHRO, LAYER_SHARED_SYNCHRO);
    createResponseUtil<TransmitRTransfer>(analysis_response_type, LAYER_RESPONSE_TRANSFER, LAYER_SHARED_TRANSFER);
    createResponseUtil<TransmitRQuery>(analysis_response_type, LAYER_RESPONSE_QUERY, LAYER_SHARED_QUERY);
    createResponseUtil<TransmitRStatus>(analysis_response_type, LAYER_RESPONSE_QUERY, LAYER_RESPONSE_STATUS);
}

/**
 * 析够函数
 *  销毁所有主类型、共享类型、响应类型工具
 */
TransmitAnalysisUtil::~TransmitAnalysisUtil() {
    for(auto begin : analysis_master_type){
        destroyTransmitUtil(begin.second);
    }
    analysis_master_type.clear();

    for(int i = 0; i < TRANSMIT_SHARED_TYPE_SIZE; i++) {
        destroyTransmitUtil(analysis_shared_type[i]);
        analysis_shared_type[i] = nullptr;
    }

    for(int i = 0; i < TRANSMIT_RESPONSE_TYPE_SIZE; i++){
        destroyTransmitUtil(analysis_response_type[i]);
        analysis_response_type[i] = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------------------------//

TransmitCompileUtil::TransmitCompileUtil() : TransmitBasicUtil(), compile_shared(0), compile_shared_size(0), compile_resource_len(0) {
    memset(analysis_shared_type, 0, sizeof(TransmitShared*) * TRANSMIT_SHARED_TYPE_SIZE);

    createSharedUtil<TransmitSychro>(analysis_shared_type, LAYER_SHARED_SYNCHRO);
    createSharedUtil<TransmitSequence>(analysis_shared_type, LAYER_SHARED_SEQUENCE);
    createSharedUtil<TransmitTransfer>(analysis_shared_type, LAYER_SHARED_TRANSFER);
    createSharedUtil<TransmitQuery>(analysis_shared_type, LAYER_SHARED_QUERY);
    createSharedUtil<TransmitText>(analysis_shared_type, LAYER_SHARED_TEXT);
    createSharedUtil<TransmitError>(analysis_shared_type, LAYER_SHARED_ERROR);
}

TransmitCompileUtil::~TransmitCompileUtil() {
    for(int i = 0; i < TRANSMIT_SHARED_TYPE_SIZE; i++){
        destroyTransmitUtil(analysis_shared_type[i]);
        analysis_shared_type[i] = nullptr;
    }
}