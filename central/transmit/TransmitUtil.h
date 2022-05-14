//
// Created by abc on 21-2-16.
//

#ifndef TEXTGDB_TRANSMITUTIL_H
#define TEXTGDB_TRANSMITUTIL_H

#include <memory>

#include "TransmitParameter.h"
#include "../BasicLayer.h"

#define TRANSMIT_MASTER_TYPE_SIZE               16
#define TRANSMIT_SHARED_TYPE_SIZE               16
#define TRANSMIT_RESPONSE_TYPE_SIZE             16

#define TRANSMIT_PARAMETER_EMPTY_TYPE           1
#define TRANSMIT_PARAMETER_UNKNOWN_MASTER       2
#define TRANSMIT_PARAMETER_VERSION              3
#define TRANSMIT_PARAMETER_LEN                  4
#define TRANSMIT_PARAMETER_MUTEX                5
#define TRANSMIT_PARAMETER_NORESPONSE           6
#define TRANSMIT_PARAMETER_UNSOCKET             7
#define TRANSMIT_PARAMETER_SOCKADDR             8
#define TRANSMIT_PARAMETER_UNWRITE              9
#define TRANSMIT_PARAMETER_UNMEMORY             10
#define TRANSMIT_PARAMETER_CONTROL_INTERCEPT    11
#define TRANSMIT_PARAMETER_CONTROL_VERIFY       12
#define TRANSMIT_PARAMETER_CONTROL_READ_CLOSE   13
#define TRANSMIT_PARAMETER_OUTPUT_MAX           14

class TransmitLayer;
class TransmitInfo;
class TransmitMemoryUtil;
class TransmitInputInfo;
class TransmitCodeInfo;

struct MsgHdr;
struct NoteMediaInfo;

/**
 * 运行时计算
 * @param convert_type
 * @return
 */
static int convertTransmitTypeToPosition(short convert_type){
    int convert_pos = 0;

    for(short type_pos = 0, type_bit = 1, type_len = sizeof(short) * LAYER_BIT_SIZE; type_pos < type_len; type_pos++){
        if((convert_type & type_bit)){
            break;
        }

        convert_pos++;
        type_bit <<= 1;
    }
    return convert_pos;
}

/**
 * 编译时计算
 * @tparam Type
 */
template <short Type> class TransmitTypePosition {
public:
    static const int position = (1 + TransmitTypePosition<(Type >> 1)>::position);
};

template <> class TransmitTypePosition<1>{
public:
    static const int position = 0;
};

//显示实例化声明
extern template class TransmitTypePosition<0x01>;
extern template class TransmitTypePosition<0x02>;
extern template class TransmitTypePosition<0x04>;
extern template class TransmitTypePosition<0x08>;
extern template class TransmitTypePosition<0x10>;
extern template class TransmitTypePosition<0x20>;

/**
 * 错误参数类
 */
struct ParameterError : public std::exception{
public:
    explicit ParameterError(int type, TransmitInfo *info) : error_type(type), error_info(info) {}
    ~ParameterError() override = default;

    const char* what() const noexcept override { return nullptr; }

    int getParameterErrorType() const { return error_type; }
    TransmitInfo* getParameterErrorInfo() const { return error_info; }
private:
    int error_type;             //错误类型
    TransmitInfo *error_info;   //错误传输信息
};

/**
 * 解析错误异常
 */
struct AnalysisError : public std::exception {
public:
    using exception::exception;
    ~AnalysisError() override = default;
    const char* what() const noexcept override { return nullptr; }
};

using InterceptFunc = bool(*)(BasicControl*, MsgHdr*);

/**
 * 传输类型拦截数据结构(主要输入、一个输出)
 */
class TransmitTypeIntercept final {
public:
    explicit TransmitTypeIntercept(TransmitLayer *layer) : transmit_layer(layer), verify_func(nullptr), output_func(nullptr) {
        std::uninitialized_fill(std::begin(master_intercept), std::end(master_intercept), nullptr);
        std::uninitialized_fill(std::begin(shared_intercept), std::end(shared_intercept), nullptr);
        std::uninitialized_fill(std::begin(response_intercept), std::end(response_intercept), nullptr);
    }
    ~TransmitTypeIntercept() = default;

    void onClearIntercept() {
        std::uninitialized_fill(std::begin(master_intercept), std::end(master_intercept), nullptr);
        std::uninitialized_fill(std::begin(shared_intercept), std::end(shared_intercept), nullptr);
        std::uninitialized_fill(std::begin(response_intercept), std::end(response_intercept), nullptr);
        verify_func = nullptr;
        output_func = nullptr;
    }

    /*
     * 设置类型拦截函数
     */
    void setMasterInterceptFunc(int, InterceptFunc);
    void setSharedInterceptFunc(int, InterceptFunc);
    void setResponseInterceptFunc(int, InterceptFunc);

    /*
     * 调用类型拦截函数
     */
    bool callMasterInterceptFunc(int, MsgHdr*);
    bool callSharedInterceptFunc(int, MsgHdr*);
    bool callResponseInterceptFunc(int, MsgHdr*);

    /*
     * 复位类型拦截函数
     */
    void clrMasterInterceptFunc(int master) { master_intercept[master] = nullptr; }
    void clrSharedInterceptFunc(int shared) { shared_intercept[convertTransmitTypeToPosition(shared)] = nullptr; }
    void clrResponseInterceptFunc(int response) { response_intercept[convertTransmitTypeToPosition(response)] = nullptr; }

    /*
     * 判断类型是否有拦截函数
     */
    bool isMasterInterceptFunc(int master) const { return static_cast<bool>(master_intercept[master]); }
    bool isSharedInterceptFunc(int shared) const { return static_cast<bool>(shared_intercept[shared]); }
    bool isResponseInterceptFunc(int response) const { return static_cast<bool>(response_intercept[response]); }

    /*
     * 设置及调用地址验证函数
     */
    void setVerifyFunc(const std::function<bool(MsgHdr*, MeetingAddressNote*)> &func) { verify_func = func; }
    bool callVerifyFunc(MsgHdr *msg, MeetingAddressNote *note_info) { return verify_func(msg, note_info); }

    /*
     * 设置及调用输出拦截函数
     */
    void setOutputFunc(const std::function<bool(MsgHdr*, MeetingAddressNote*)> &func) { output_func = func; }
    bool callOutputFunc(MsgHdr *msg, MeetingAddressNote *note_info) { return output_func(msg, note_info); }
private:
    TransmitLayer *transmit_layer;                                  //用于获取控制层
    InterceptFunc master_intercept[TRANSMIT_MASTER_TYPE_SIZE];      //主类型拦截函数
    InterceptFunc shared_intercept[TRANSMIT_SHARED_TYPE_SIZE];      //共享类型拦截函数
    InterceptFunc response_intercept[TRANSMIT_RESPONSE_TYPE_SIZE];  //响应类型拦截函数
    std::function<bool(MsgHdr*, MeetingAddressNote*)> verify_func;  //地址验证函数
    std::function<bool(MsgHdr*, MeetingAddressNote*)> output_func;  //输出拦截函数
};

/**
 * 解析后的输入信息
 */
struct TransmitInfo{
    TransmitInfo() : total_len(0), hdr_len(0), resources_len(0), hdr_offset(0), resource_offset(0), info_msg(nullptr),
                     info_reader(nullptr), transmit_note(nullptr), copy_func(nullptr) {}
    ~TransmitInfo() = default;

    TransmitInfo& setTotalLen(int len) { total_len = len; return *this; }
    TransmitInfo& setHdrLen(int len) { hdr_len = len; return *this; }
    TransmitInfo& setResourceLen(int len) { resources_len = len; return *this; }
    TransmitInfo& setHdrOffSet(int offset) { hdr_offset = offset; return *this; }
    TransmitInfo& setResourceOffSet(int offset) { resource_offset = offset; return *this; }
    TransmitInfo& setInfoMsg(MsgHdr *msg) { info_msg = msg; return *this; }
    TransmitInfo& setInfoReadre(MemoryReader *reader) { info_reader = reader; return *this; }
    TransmitInfo& setInfoNote(MeetingAddressNote *note) { transmit_note = note; return *this; }
    TransmitInfo& setInfoCode(TransmitCodeInfo *code) { code_info = code; return *this; }
    TransmitInfo& setCopyFunc(void (*func)(MsgHdr*, char*, uint32_t , uint32_t)) { copy_func = func;return *this; }

    int total_len;                                              //总长度
    int hdr_len;                                                //以short为单位的长度资源
    int resources_len;                                          //资源长度
    int hdr_offset;                                             //长度资源的偏移量
    int resource_offset;                                        //资源长度的偏移量
    MsgHdr *info_msg;                                           //资源信息头
    MemoryReader *info_reader;                                  //资源读取器
    MeetingAddressNote *transmit_note;                          //本地存储地址
    TransmitCodeInfo *code_info;                                //视音频编码信息
    void (*copy_func)(MsgHdr*, char*, uint32_t , uint32_t);     //拷贝函数
};

//--------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------类型信息类-------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * 类型信息基类
 */
class TransmitType {
public:
    TransmitType() = default;
//    explicit TransmitType(TransmitInfo *info) : type_info(info) {}
    virtual ~TransmitType() = default;

    TransmitType(const TransmitType&) = default;
    TransmitType(TransmitType&&) = default;
    TransmitType& operator=(const TransmitType&) = default;
    TransmitType& operator=(TransmitType&&) = default;

    //类型资源长度
    virtual bool isLen() const = 0;
    //类型长度资源
    virtual short onLen() const = 0;
    //信息类型
    virtual short onType() const = 0;
    //类型拦截函数
    virtual bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) = 0;
    //类型参数
    virtual void onParameter(TransmitParameter*) = 0;
    //初始化类型数据
    virtual void onInitCompileType(char*) = 0;

    //类型解析
    void onTypeAnalysis();
    void onTypeCompile();
    //设置解析后的输入信息
    void setTypeInfo(TransmitInfo *info) { this->type_info = info; }
    //验证输入信息
    void onVerifyAnalysis(TransmitInfo*, const std::function<int()> &func = nullptr) throw(ParameterError);
    void onVerifyCompile(TransmitInfo*, const std::function<int()> &func = nullptr) throw(ParameterError);

    MsgHdr* getTransmitMsg() const { return type_info->info_msg; }
    MeetingAddressNote* getTransmitNote() const { return type_info->transmit_note; }
protected:
    //读取长度信息
    static short readResourceLen(TransmitInfo*);
    static short writeResourceLen(TransmitType*);
    //读取资源信息
    static char* readResourceBuffer(TransmitInfo*);
    //结束读取信息
    static void  readResourceFinish(TransmitType *transmit_type, short len){
        transmit_type->onAnalysisFinal(len);
    }
    static void  writeResourceFinish(TransmitType *transmit_type, short len){
        transmit_type->onCompileFinal(len);
    }

    //类型解析实现函数
    virtual void onTypeAnalysis0(void(*)(TransmitType*, short)) = 0;
    //类型编译实现函数
    virtual void onTypeCompile0(void(*)(TransmitType*, short)) = 0;
    //验证类型及对应长度和资源信息
    virtual bool onTypeVerify(short) = 0;

    //解析结束
    void onAnalysisFinal(short len) {
        //设置已读取的类型长度信息
        type_info->hdr_offset += sizeof(short);
        //如果类型有长度信息（存在资源）,则设置已读取的资源信息
        if(len) {
            type_info->resource_offset += len;
        }
    }
    //编译接收
    void onCompileFinal(short len){
        //设置已经拷贝的类型长度信息
        type_info->hdr_offset += sizeof(short);
        //设置已经拷贝的资源长度
        type_info->resource_offset += len;
    }
    TransmitInfo *type_info;    //解析后的输入信息
};

/**
 * 主类型信息派生类
 */
class TransmitMaster : public TransmitType {
public:
    TransmitMaster() : TransmitType() {}
    ~TransmitMaster() override = default;

    TransmitMaster(const TransmitMaster&) = default;
    TransmitMaster(TransmitMaster&&) = default;
    TransmitMaster& operator=(const TransmitMaster&) = default;
    TransmitMaster& operator=(TransmitMaster&&) = default;

    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    virtual void onMasterInput(TransmitInputInfo*) = 0;
};

/**
 * 加入类型信息
 *  1：该类型是主类型的派生类
 *  2：该类型没有资源
 *  3：该类型不需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitJoin : public TransmitMaster{
public:
    TransmitJoin() : TransmitMaster() {}
    ~TransmitJoin() override = default;

    TransmitJoin(const TransmitJoin&) = default;
    TransmitJoin(TransmitJoin&&) = default;
    TransmitJoin& operator=(const TransmitJoin&) = default;
    TransmitJoin& operator=(TransmitJoin&&) = default;

    bool isLen() const override { return false; }
    short onLen() const override { return 0; }
    short onType() const override { return LAYER_MASTER_JOIN; }
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char*) override {}

    void onMasterInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override {}
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override { return false; }
};

/**
 * 链接类型信息
 *  1：该类型是主类型的派生类
 *  2：该类型没有资源
 *  3：该类型不需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitLink : public TransmitMaster{
public:
    TransmitLink() : TransmitMaster() {}
    ~TransmitLink() override = default;

    TransmitLink(const TransmitLink&) = default;
    TransmitLink(TransmitLink&&) = default;
    TransmitLink& operator=(const TransmitLink&) = default;
    TransmitLink& operator=(TransmitLink&&) = default;

    short onType() const override { return LAYER_MASTER_LINK; }
    bool isLen() const override { return false; }
    short onLen() const override { return 0; }
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char*) override {}

    void onMasterInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override {}
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
     bool onTypeVerify(short) override { return false; }
};

/**
 * 退出类型信息
 *  1：该类型是主类型的派生类
 *  2：该类型没有资源
 *  3：该类型不需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitExit : public TransmitMaster{
public:
    TransmitExit() : TransmitMaster() {}
    ~TransmitExit() override = default;

    TransmitExit(const TransmitExit&) = default;
    TransmitExit(TransmitExit&&) = default;
    TransmitExit& operator=(const TransmitExit&) = default;
    TransmitExit& operator=(TransmitExit&&) = default;

    short onType() const override { return LAYER_MASTER_EXIT; }
    bool isLen() const override { return false; }
    short onLen() const override { return 0; }
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char*) override {}

    void onMasterInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override {}
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override { return false; }
};

/**
 * 音视频类型信息
 *  1：该类型是主类型的派生类
 *  2：该类型存在视音频资源
 *  3：该类型需要验证长度和资源信息
 *  4：该类型包含音视频缓存
 */
class TransmitMedia : public TransmitMaster {
public:
    TransmitMedia() : TransmitMaster() {}
    ~TransmitMedia() override = default;

    TransmitMedia(const TransmitMedia&) = default;
    TransmitMedia(TransmitMedia&&) = default;
    TransmitMedia& operator=(const TransmitMedia&) = default;
    TransmitMedia& operator=(TransmitMedia&&) = default;

    bool isLen() const override { return true; }
    short onLen() const override { return media_len; }
    short onType() const override { return LAYER_MASTER_MEDIA; }
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char*) override;

    void onMasterInput(TransmitInputInfo*) override;
    MediaData getMediaData();
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    static void onAnalysisMedia(MediaData*, NoteMediaInfo*, int);
    
    void onFixedMediaInfo();
    
    short media_len;                  //视音频资源总长度
    short media_size;                 //视音频资源总数量
    short media_len_offset;           //资源长度信息的偏移量（在输入内存中的地址偏移量）
    short media_buffer_offset;        //资源地址信息的偏移量（在输入内存中的地址偏移量）
};

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 共享类型信息派生类
 */
class TransmitShared : public TransmitType {
public:
    TransmitShared() : TransmitType(), actual_shared_pos(0) {}
//    explicit TransmitShared(TransmitInfo *info) : TransmitType(info), actual_shared_pos(convertTransmitTypeToPosition(onType())) {}
    ~TransmitShared() override = default;

    TransmitShared(const TransmitShared&) = default;
    TransmitShared(TransmitShared&&) = default;
    TransmitShared& operator=(const TransmitShared&) = default;
    TransmitShared& operator=(TransmitShared&&) = default;

    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    virtual void onSharedInput(TransmitInputInfo*) = 0;
protected:
    void setActualSharedPos(int pos) { actual_shared_pos = pos; }
private:
    int actual_shared_pos;
};

/**
 * 同步类型信息
 *  1：该类型是共享类型的派生类
 *  2：该类型存在同步时间信息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 * 同步不需要发送时间信息,时间信息有中心化主机存储并发送序号,远程端收到同步请求后直接响应同步,由中心化主机根据需要确定该同步是否有效并计算RTT
 */
class TransmitSychro : public TransmitShared {
public:
    TransmitSychro() : TransmitShared() {
//        setActualSharedPos(convertTransmitTypeToPosition(onType()));
        setActualSharedPos(TransmitTypePosition<LAYER_SHARED_SYNCHRO>::position);
    }
    ~TransmitSychro() override = default;

    TransmitSychro(const TransmitSychro&) = default;
    TransmitSychro(TransmitSychro&&) = default;
    TransmitSychro& operator=(const TransmitSychro&) = default;
    TransmitSychro& operator=(TransmitSychro&&) = default;

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(sizeof(SynchroData)); }
    short onType() const override { return LAYER_SHARED_SYNCHRO; }
    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char *data) override { synchro_data = *reinterpret_cast<SynchroData*>(data); }

    void onSharedInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    //同步信息
    SynchroData synchro_data;
};

/**
 * 序号类型信息
 *  1：该类型是共享类型的派生类
 *  2：该类型存在序号信息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitSequence : public TransmitShared {
public:
    TransmitSequence() : TransmitShared() {
//        setActualSharedPos(convertTransmitTypeToPosition(onType()));
        setActualSharedPos(TransmitTypePosition<LAYER_SHARED_SEQUENCE>::position);
    }
    ~TransmitSequence() override = default;

    TransmitSequence(const TransmitSequence&) = default;
    TransmitSequence(TransmitSequence&&) = default;
    TransmitSequence& operator=(const TransmitSequence&) = default;
    TransmitSequence& operator=(TransmitSequence&&) = default;

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(sizeof(SequenceData)); }
    short onType() const override { return LAYER_SHARED_SEQUENCE; }
    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char *data) override { sequence_data = *reinterpret_cast<SequenceData*>(data); }

    void onSharedInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    //序号信息
    SequenceData sequence_data;
};

/**
 * 转移地址类型信息
 *  1：该类型是共享类型的派生类
 *  2：该类型存在转移地址信息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitTransfer : public TransmitShared{
public:
    TransmitTransfer() : TransmitShared() {
//        setActualSharedPos(convertTransmitTypeToPosition(onType()));
        setActualSharedPos(TransmitTypePosition<LAYER_SHARED_TRANSFER>::position);
    }
    ~TransmitTransfer() override = default;

    TransmitTransfer(const TransmitTransfer&) = default;
    TransmitTransfer(TransmitTransfer&&) = default;

    TransmitTransfer& operator=(const TransmitTransfer&) = default;
    TransmitTransfer& operator=(TransmitTransfer&&) = default;

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(sizeof(sockaddr_in)); }
    short onType() const override { return LAYER_SHARED_TRANSFER; }

    void onParameter(TransmitParameter*) override;
    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onInitCompileType(char *data) override { transfer_addr = *reinterpret_cast<sockaddr_in*>(data); }

    void onSharedInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    sockaddr_in transfer_addr;      //转移地址
};

/**
 * 查询类型信息
 *  1：该类型是共享类型的派生类
 *  2：该类型存在查询类型息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitQuery : public TransmitShared {
public:
    TransmitQuery() : TransmitShared() {
//        setActualSharedPos(convertTransmitTypeToPosition(onType()));
        setActualSharedPos(TransmitTypePosition<LAYER_SHARED_QUERY>::position);
    }
    ~TransmitQuery() override = default;

    TransmitQuery(const TransmitQuery&) = default;
    TransmitQuery(TransmitQuery&&) = default;

    TransmitQuery& operator=(const TransmitQuery&) = default;
    TransmitQuery& operator=(TransmitQuery&&) = default;

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(sizeof(uint32_t)); }
    short onType() const override { return LAYER_SHARED_QUERY; }

    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char *data) override { query_type = *reinterpret_cast<uint32_t*>(data); }

    void onSharedInput(TransmitInputInfo*) override;

    uint32_t getTransmitQuery() const { return query_type; }
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    uint32_t query_type;     //查询类型
};

/**
 * 文本类型信息
 *  1：该类型是共享类型的派生类
 *  2：该类型存在文本信息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型包含文本缓存
 */
class TransmitText : public TransmitShared{
public:
    TransmitText() : TransmitShared() {
//        setActualSharedPos(convertTransmitTypeToPosition(onType()));
        setActualSharedPos(TransmitTypePosition<LAYER_SHARED_TEXT>::position);
    }
    ~TransmitText() override = default;

    TransmitText(const TransmitText &text) noexcept : TransmitShared(text), text_data(text.text_data) {}
    TransmitText(TransmitText &&text) noexcept : TransmitShared(std::move(text)), text_data(text.text_data) {}
    TransmitText& operator=(const TransmitText &text) noexcept {
        TransmitShared::operator=(text); text_data = text.text_data; return *this;
    }
    TransmitText& operator=(TransmitText &&text) noexcept {
        TransmitShared::operator=(std::move(text)); text_data = text.text_data; return *this;
    }

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(text_data.info_len); }
    short onType() const override { return LAYER_SHARED_TEXT; }
    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char *data) override { text_data = *reinterpret_cast<InfoData*>(data); }

    void onSharedInput(TransmitInputInfo*) override;

    ResourceData getTextData();
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    //文本数据
    InfoData text_data;
};

/**
 * 错误类型信息
 *  1：该类型是共享类型的派生类
 *  2：该类型存在错误类型信息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitError : public TransmitShared{
public:
    TransmitError() : TransmitShared() {
//        setActualSharedPos(convertTransmitTypeToPosition(onType()));
        setActualSharedPos(TransmitTypePosition<LAYER_SHARED_ERROR>::position);
    }
    ~TransmitError() override = default;

    TransmitError(const TransmitError&) = default;
    TransmitError(TransmitError&&) = default;
    TransmitError& operator=(const TransmitError&) = default;
    TransmitError& operator=(TransmitError&&) = default;

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(sizeof(uint32_t)); }
    short onType() const override { return LAYER_SHARED_ERROR; }

    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char *data) override { error_type = *reinterpret_cast<uint32_t*>(data); }

    void onSharedInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    uint32_t error_type;
};

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 响应类型信息派生类
 */
class TransmitResponse : public TransmitType{
public:
    TransmitResponse() : TransmitType(), actual_response_pos(0) {}
//    explicit TransmitResponse(TransmitInfo *info) : TransmitType(info), actual_response_pos(convertTransmitTypeToPosition(onType())) {}
    ~TransmitResponse() override = default;

    TransmitResponse(const TransmitResponse&) = default;
    TransmitResponse(TransmitResponse&&) = default;

    TransmitResponse& operator=(const TransmitResponse&) = default;
    TransmitResponse& operator=(TransmitResponse&&) = default;

    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    virtual void onResponseInput(TransmitInputInfo*) = 0;
protected:
    void setActualResponsePos(int pos) { actual_response_pos = pos; }
private:
    int actual_response_pos;
};

/**
 * 加入响应类型信息
 *  1：该类型是响应类型的派生类
 *  2：该类型不存在资源信息
 *  3：该类型不需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitRJoin : public TransmitResponse{
public:
    TransmitRJoin() : TransmitResponse() {
//        setActualResponsePos(convertTransmitTypeToPosition(onType()));
        setActualResponsePos(TransmitTypePosition<LAYER_RESPONSE_JOIN>::position);
    }
    ~TransmitRJoin() override = default;

    TransmitRJoin(const TransmitRJoin&) = default;
    TransmitRJoin(TransmitRJoin&&) = default;
    TransmitRJoin& operator=(const TransmitRJoin&) = default;
    TransmitRJoin& operator=(TransmitRJoin&&) = default;

    bool isLen() const override { return false; }
    short onLen() const override { return 0; }
    short onType() const override { return LAYER_RESPONSE_JOIN; }
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char*) override {}

    void onResponseInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override {}
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override { return false; }
};

/**
 * 同步响应类型信息
 *  1：该类型是响应类型的派生类
 *  2：该类型不存在资源信息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitRSychro : public TransmitResponse{
public:
    TransmitRSychro() : TransmitResponse() {
//        setActualResponsePos(convertTransmitTypeToPosition(onType()));
        setActualResponsePos(TransmitTypePosition<LAYER_RESPONSE_SYNCHRO>::position);
    }
    ~TransmitRSychro() override = default;

    TransmitRSychro(const TransmitRSychro&) = default;
    TransmitRSychro(TransmitRSychro&&) = default;
    TransmitRSychro& operator=(const TransmitRSychro&) = default;
    TransmitRSychro& operator=(TransmitRSychro&&) = default;

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(sizeof(SynchroData)); }
    short onType() const override { return LAYER_RESPONSE_SYNCHRO; }

    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char *data) override { synchro_data = *reinterpret_cast<SynchroData*>(data); }

    void onResponseInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void (*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    //同步数据
    SynchroData synchro_data;
};

/**
 * 转移响应类型信息
 *  1：该类型是响应类型的派生类
 *  2：该类型不存在资源信息
 *  3：该类型不需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitRTransfer : public TransmitResponse{
public:
    TransmitRTransfer() : TransmitResponse() {
//        setActualResponsePos(convertTransmitTypeToPosition(onType()));
        setActualResponsePos(TransmitTypePosition<LAYER_RESPONSE_TRANSFER>::position);
    }
    ~TransmitRTransfer() override = default;

    TransmitRTransfer(const TransmitRTransfer&) = default;
    TransmitRTransfer(TransmitRTransfer&&) = default;

    TransmitRTransfer& operator=(const TransmitRTransfer&) = default;
    TransmitRTransfer& operator=(TransmitRTransfer&&) = default;

    bool isLen() const override { return false; }
    short onLen() const override { return 0; }
    short onType() const override { return LAYER_RESPONSE_TRANSFER; }
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char*) override {}

    void onResponseInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override {}
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override { return false; }
};

/**
 * 状态响应信息（中心化主机向运行的远程端发送（包含在音视频资源里）已退出及正在被动处理的远程端信息）
 *  1：该类型是响应类型的派生类
 *  2：该类型存在状态序号信息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型不包含缓存信息
 */
class TransmitRStatus : public TransmitResponse{
public:
    TransmitRStatus() : TransmitResponse() {
//        setActualResponsePos(convertTransmitTypeToPosition(onType()));
        setActualResponsePos(TransmitTypePosition<LAYER_RESPONSE_STATUS>::position);
    }
    ~TransmitRStatus() override = default;

    TransmitRStatus(const TransmitRStatus&) = default;
    TransmitRStatus(TransmitRStatus&&) = default;
    TransmitRStatus& operator=(const TransmitRStatus&) = default;
    TransmitRStatus& operator=(TransmitRStatus&&) = default;

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(sizeof(uint32_t)); }
    short onType() const override { return LAYER_RESPONSE_STATUS; }

    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char *data) override { status_sequence = *reinterpret_cast<uint32_t*>(data); }

    void onResponseInput(TransmitInputInfo*) override;
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    uint32_t status_sequence;    //响应序号（对应发送状态的序号）
};

/**
 * 查询响应信息
 *  1：该类型是响应类型的派生类
 *  2：该类型存在查询信息
 *  3：该类型需要验证长度和资源信息
 *  4：该类型包含查询缓存
 */
class TransmitRQuery : public TransmitResponse{
public:
    TransmitRQuery() : TransmitResponse() {
//        setActualResponsePos(convertTransmitTypeToPosition(onType()));
        setActualResponsePos(TransmitTypePosition<LAYER_RESPONSE_QUERY>::position);
    }
    ~TransmitRQuery() override = default;

    TransmitRQuery(const TransmitRQuery &rquery) noexcept : TransmitResponse(rquery), query_data(rquery.query_data) {}
    TransmitRQuery(TransmitRQuery &&rquery) noexcept : TransmitResponse(std::move(rquery)), query_data(rquery.query_data) {}

    TransmitRQuery& operator=(const TransmitRQuery &rquery) noexcept {
        TransmitResponse::operator=(rquery); query_data = rquery.query_data; return *this;
    }
    TransmitRQuery& operator=(TransmitRQuery &&rquery) noexcept {
        TransmitResponse::operator=(std::move(rquery)); query_data = rquery.query_data; return *this;
    }

    bool isLen() const override { return true; }
    short onLen() const override { return static_cast<short>(query_data.info_len); }
    short onType() const override { return LAYER_RESPONSE_QUERY; }

    bool onTypeIntercept(TransmitTypeIntercept*, MsgHdr*) override;
    void onParameter(TransmitParameter*) override;
    void onInitCompileType(char *data) override { query_data = *reinterpret_cast<InfoData*>(data); }

    void onResponseInput(TransmitInputInfo*) override;
    ResourceData getQueryData();
protected:
    void onTypeAnalysis0(void(*)(TransmitType*, short)) override;
    void onTypeCompile0(void(*)(TransmitType*, short)) override;
    bool onTypeVerify(short) override;
private:
    InfoData query_data;
};

//--------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------类型解析工具---------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * 类型解析工具基类
 */
class TransmitUtil{
public:
    explicit TransmitUtil(short type) : transmit_type(type) {}
    virtual ~TransmitUtil() = default;

    //类型关系
    virtual int isRelation(TransmitInfo*) const = 0;
    //是否包含
    virtual bool isContain(short) const = 0;
    //设置解析后的输入信息
    virtual void setTransmitInfo(TransmitInfo*) = 0;
    //解析类型
    virtual void varietyTransmitType(TransmitInfo*) throw(ParameterError) = 0;
    //编译类型
    virtual void compileTransmitType(TransmitInfo*) throw(ParameterError) = 0;
    //返回解析后的类型信息类
    virtual TransmitType* onTransmitType() = 0;
    //返回解析类型
    short getTransmitType() const { return transmit_type; }
protected:
    static inline char onMasterType(MsgHdr*);
    static inline short onSharedType(MsgHdr*);
    static inline short onResponseType(MsgHdr*);

    short transmit_type;    //解析类型
};

/**
 * 解析主类型工具派生类
 * @tparam T    主类型信息类
 */
template <typename T> class TransmitMasterUtil final : public TransmitUtil{
public:
    TransmitMasterUtil(short type) : TransmitUtil(type), ignore_shared(0), master_type() {}
    ~TransmitMasterUtil() override = default;

    //是否存在共享类型而忽略主类型（link与synchro）
    int isRelation(TransmitInfo *transmit_info) const override { return isIgnore(onSharedType(transmit_info->info_msg)); }
    bool isContain(short type) const override { return false; }
    void setTransmitInfo(TransmitInfo *info) override { master_type.setTypeInfo(info); }
    bool isIgnore(short type) const  { return static_cast<bool>(ignore_shared & type); }
    void setIgnore(short type) { ignore_shared |= type; }

    //主类型解析工具类解析主类型
    void varietyTransmitType(TransmitInfo *transmit_info) throw(ParameterError) override {
        //由主类型信息类解析
        master_type.onVerifyAnalysis(transmit_info, [&]() -> int { return isRelation(transmit_info); });
    }
    //主类型编译工具编译主类型
    void compileTransmitType(TransmitInfo *transmit_info) throw(ParameterError) override {
        //由主类型信息类解析
        master_type.onVerifyCompile(transmit_info, [&]() -> int { return isRelation(transmit_info); });
    }
    TransmitType* onTransmitType() override { return dynamic_cast<TransmitType*>(&master_type); }
private:
    short ignore_shared;    //忽略主类型的共享类型信息
    T master_type;          //主类型信息类
};

/**
 * 解析共享类型工具派生类
 * @tparam T    共享类型信息类
 */
template <typename T> class TransmitSharedUtil final : public TransmitUtil{
public:
    explicit TransmitSharedUtil(short type) : TransmitUtil(type), shared_type() {
        initSharedUtil();
    }
    explicit TransmitSharedUtil(short type, std::initializer_list<char> filter) : TransmitUtil(type), shared_type() {
        initSharedUtil();
        initSharedFilter(filter);
    }
    ~TransmitSharedUtil() override = default;

    //共享类型是否与主类型互斥
    int isRelation(TransmitInfo *transmit_info) const override{
        //是否互斥信息
        return isFilter(onMasterType(transmit_info->info_msg)) ? TRANSMIT_PARAMETER_LEN : 0;
    }
    bool isContain(short type) const override { return (transmit_type & type); }
    void setTransmitInfo(TransmitInfo *info) override { shared_type.setTypeInfo(info); }
    bool isFilter(char type) const { return master_filter[type]; }
    void setFilter(char filter) { master_filter[filter] = true; }

    //共享类型解析工具类解析共享类型
    void varietyTransmitType(TransmitInfo *transmit_info) throw(ParameterError) override {
        //由共享类型信息类解析
        shared_type.onVerifyAnalysis(transmit_info, [&]() -> int { return isRelation(transmit_info); });
    }
    //共享类型编译工具编译共享类型
    void compileTransmitType(TransmitInfo *transmit_info) throw(ParameterError) override {
        //由共享类型信息类解析
        shared_type.onVerifyCompile(transmit_info, [&]() -> int { return isRelation(transmit_info); });
    }
    TransmitType* onTransmitType() override { return dynamic_cast<TransmitType*>(&shared_type); }
private:
    //初始化共享解析工具
    void initSharedUtil(){
        std::uninitialized_fill(std::begin(master_filter), std::end(master_filter), false);
    }
    //初始化互斥主类型信息
    void initSharedFilter(std::initializer_list<char> &filter){
        for(const auto &value : filter){ setFilter(value); }
    }

    bool master_filter[LAYER_MATER_TYPE_SIZE];   //互斥的主类型信息
    T shared_type;  //共享类型信息类
};

/**
 * 解析响应类型工具派生类
 * @tparam T    响应类型信息类
 */
template <typename T> class TransmitResponseUtil : public TransmitUtil{
public:
    TransmitResponseUtil(short type, short rtype) : TransmitUtil(type), rtype(rtype), response_type() {}
    ~TransmitResponseUtil() override = default;

//    void setRequestType(short request) { rtype = request; }

    //置位的响应类型是否有对应的置位主类型或共享类型
    int isRelation(TransmitInfo *transmit_info) const override {
        //是否对应信息
        return (!isResponse(onMasterType(transmit_info->info_msg), onSharedType(transmit_info->info_msg))) ? TRANSMIT_PARAMETER_NORESPONSE : 0;
    }
    bool isContain(short type) const override { return (transmit_type & type); }
    void setTransmitInfo(TransmitInfo *info) override { response_type.setTypeInfo(info); }
    bool isResponse(short master, short shared) const { return ((rtype == master) || (rtype & shared)); }

    //响应类型解析工具类解析响应类型
    void varietyTransmitType(TransmitInfo *transmit_info) throw(ParameterError) override {
        //由响应类型信息类解析
        response_type.onVerifyAnalysis(transmit_info, [&]() -> int { return isRelation(transmit_info); });
    }
    //响应类型编译工具编译响应类型
    void compileTransmitType(TransmitInfo *transmit_info) throw(ParameterError) override {
        //由响应类型信息类解析
        response_type.onVerifyCompile(transmit_info, [&]() -> int { return isRelation(transmit_info); });
    }
    TransmitType* onTransmitType() override { return dynamic_cast<TransmitType*>(&response_type); }
private:
    short rtype; //响应的主类型或共享类型
    T response_type;    //响应类型信息类
};

class TransmitBasicUtil {
public:
    TransmitBasicUtil() = default;
    virtual ~TransmitBasicUtil() = default;

    TransmitBasicUtil(const TransmitBasicUtil&) = delete;
    TransmitBasicUtil& operator=(const TransmitBasicUtil&) = delete;
//    TransmitBasicUtil(TransmitBasicUtil&&) = delete;
//    TransmitBasicUtil& operator=(TransmitBasicUtil&&) = delete;

    virtual TransmitUtil* getMasterAnalysisUtil(char) = 0;
    virtual TransmitUtil* getSharedAnalysisUtil(short) = 0;
    virtual TransmitUtil* getResponseAnalysisUtil(short) = 0;

    static uint16_t onCodeNetWord(uint16_t value) { return htons(value); }
    static uint32_t onCodeNetWord(uint32_t value) { return htonl(value); }
    static uint16_t onDecodeNetWord(uint16_t value) { return ntohs(value); }
    static uint32_t onDecodeNetWord(uint32_t value) { return ntohl(value); }


    static void onCodeNetWordForNoteStatus(TransmitNoteStatus*);
    static void onDecodeNetWordForNoteStatus(TransmitNoteStatus*);

    static void onCodeTransmitMsgHdr(MsgHdr*);
    static void onDecodeTransmitMsgHdr(MsgHdr*);

    static void onCodeTransmitMedia(TransmitInfo*, NoteMediaInfo*, uint32_t);
    static void onDecodeTransmitMedia(NoteMediaInfo*);

    static void onCodeTransmitSynchro(SynchroData*);
    static void onDecodeTransmitSynchro(SynchroData*);

    static void onCodeTransmitSequence(SequenceData*);
    static void onDecodeTransmitSequence(SequenceData*);

    static void onCodeTransmitTransfer(sockaddr_in*);
    static void onDecodeTransmitTransfer(sockaddr_in*);
protected:
    template <typename T, typename... Args> static TransmitUtil* createAnalysisUtil(Args&&... args){
        return (new T(std::forward<Args>(args)...));
    }

    static void destroyTransmitUtil(TransmitUtil *transmit_util) {
        delete transmit_util;
    }

    template <typename V> static void createMasterUtil(std::map<char, TransmitUtil*> *master_map, short master_type, short ignore = 0){
        auto master_util = dynamic_cast<TransmitMasterUtil<V>*>(createAnalysisUtil<TransmitMasterUtil<V>>(master_type));
        master_util->setIgnore(ignore);
        master_map->insert({master_type, master_util});
    }
    template <typename V> static void createSharedUtil(TransmitUtil **shared_util, short shared_type){
        shared_util[convertTransmitTypeToPosition(shared_type)] = createAnalysisUtil<TransmitSharedUtil<V>>(shared_type);
    }
    template <typename V> static void createResponseUtil(TransmitUtil **response_util, short response_type, short rtype){
        response_util[convertTransmitTypeToPosition(response_type)] = createAnalysisUtil<TransmitResponseUtil<V>>(response_type, rtype);
    }
};

/**
 * 传输层解析工具集合（包含所有主类型、共享类型、响应类型的解析工具）
 */
class TransmitAnalysisUtil : public TransmitBasicUtil{
public:
    TransmitAnalysisUtil();
    ~TransmitAnalysisUtil() override;

    TransmitUtil* getMasterAnalysisUtil(char master_type) override {
        auto master_iterator = analysis_master_type.find(master_type);
        return (master_iterator != analysis_master_type.end()) ? master_iterator->second : nullptr;
    }
    TransmitUtil* getSharedAnalysisUtil(short shared_pos) override {
        return analysis_shared_type[shared_pos];
    }
    TransmitUtil* getResponseAnalysisUtil(short response_pos) override {
        return analysis_response_type[response_pos];
    }
private:
    TransmitUtil *analysis_shared_type[TRANSMIT_SHARED_TYPE_SIZE];      //共享类型工具集合
    TransmitUtil *analysis_response_type[TRANSMIT_RESPONSE_TYPE_SIZE];  //响应类型工具集合
    std::map<char, TransmitUtil*> analysis_master_type;                 //主类型工具集合
};

/**
 * 传输层编译工具（用于编译共享类型及其输出）
 */
class TransmitCompileUtil : public TransmitBasicUtil{
public:
    TransmitCompileUtil();
    ~TransmitCompileUtil() override;

    TransmitUtil* getMasterAnalysisUtil(char) override { return nullptr; }
    TransmitUtil* getSharedAnalysisUtil(short shared_pos) override { return analysis_shared_type[shared_pos]; }
    TransmitUtil* getResponseAnalysisUtil(short) override { return nullptr; }

    virtual short onCompileSharedType() const { return compile_shared; }
    virtual short onCompileSharedSize() const { return compile_shared_size; }
    virtual short onCompileResourceLen() const { return compile_resource_len; }

    virtual void compileSharedType(short shared, char *type_buffer){
        TransmitUtil *shared_util = getSharedAnalysisUtil(shared);
        if(!(compile_shared & shared)) {
            shared_util->onTransmitType()->onInitCompileType(type_buffer);
            compile_shared |= shared;
            compile_shared_size += sizeof(short);
            compile_resource_len += shared_util->onTransmitType()->onLen();
        }
    }
    virtual TransmitUtil* transmitSharedType(short shared){
        TransmitUtil *shared_util = nullptr;
        if(compile_shared & shared){
            shared_util = getSharedAnalysisUtil(shared);
            compile_shared &= (~shared);
            compile_shared_size -= sizeof(short);
            compile_resource_len -= shared_util->onTransmitType()->onLen();
        }
        return shared_util;
    }
    virtual void onClearRemoteCompile(){ compile_shared = 0; compile_shared_size = 0; compile_resource_len = 0; }
private:
    short compile_shared;                                               //输出共享类型集合
    short compile_shared_size;                                          //输出共享类型数量
    short compile_resource_len;                                         //输出共享类型资源长度
    TransmitUtil *analysis_shared_type[TRANSMIT_SHARED_TYPE_SIZE];      //共享类型工具集合
};

/*
 * 传输内存工具类（动态、固定）
 */
class TransmitMemoryUtil{
public:
    explicit TransmitMemoryUtil(int len) : memory_len(len) {}
    virtual ~TransmitMemoryUtil() = default;

    virtual bool onCreateMemory(int) = 0;
    virtual void onCopyMemory(int, const std::function<void(char*)>&) throw(std::logic_error) = 0;
    virtual MemoryReader onExtractMemory() = 0;
protected:
    void resetMemoryLen(int len) { memory_len = len; }

    int memory_len;     //内存长度
};

#endif //TEXTGDB_TRANSMITUTIL_H
