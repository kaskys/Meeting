//
// Created by abc on 21-2-13.
//

#ifndef TEXTGDB_TRANSMITPARAMETER_H
#define TEXTGDB_TRANSMITPARAMETER_H

#include <tuple>
#include <atomic>
#include <mutex>
#include <exception>
#include <cstdint>
#include <utility>

using ParameterType = std::pair<uint32_t, uint32_t>;

struct ParameterTransmit{
    uint32_t input_total_byte;
    uint32_t input_media_byte;
    uint32_t input_error_byte;
    uint32_t input_other_byte;
    uint32_t input_text_byte;
    uint32_t input_null;
    uint32_t type_error;
    uint32_t parameter_error;

    uint32_t output_total_byte;
    uint32_t output_media_byte;
    uint32_t output_error_byte;
    uint32_t output_other_byte;
    uint32_t output_text_byte;
    uint32_t output_success;
    uint32_t output_fail;

    uint32_t link_number;
    uint32_t seq_number;
    uint32_t obtain_null;

    uint32_t syn_number;
    uint32_t syn_response_number;
    uint32_t join_number;
    uint32_t join_response_number;
    uint32_t exit_number;
    uint32_t exit_response_number;
    uint32_t query_number;
    uint32_t query_response_number;
    uint32_t match_number;
    uint32_t unmatch_number;
};

class LayerParameter {
public:
    LayerParameter() : total_byte(0), media_byte(0), error_byte(0), other_byte(0), text_byte(0){}
    virtual ~LayerParameter() = default;
protected:
    void onMediaParameter(uint32_t);
    void onErrorParameter(uint32_t);
    void onOtherParameter(uint32_t);
    void onTextParameter(uint32_t);

    void onClear();
    void onFuseData(LayerParameter&);
    void onCopyData(uint32_t&, uint32_t&, uint32_t&, uint32_t&, uint32_t&);
private:
    uint32_t total_byte;    //（输入\输出）或（输入和输出）的总字节
    uint32_t media_byte;    //媒体字节
    uint32_t error_byte;    //错误字节
    uint32_t other_byte;    //其他字节（非媒体、错误、文本以外的字节）
    uint32_t text_byte;     //文本字节
};

class InputParameter : public LayerParameter{
public:
    InputParameter() : LayerParameter(), input_null(0), type_error(0), parameter_error(0) {}
    ~InputParameter() override = default;

    void onUnSocket(uint32_t size = 1);
    void onTypeError(uint32_t size = 1);
    void onParameterError(uint32_t size = 1);

    void onInputMedia(uint32_t byte = 0);
    void onInputError(uint32_t byte = 0);
    void onInputOther(uint32_t byte = 0);
    void onInputText(uint32_t byte = 0);
protected:
    void onClear();
    void onCopyData(ParameterTransmit*);
    void onFuseData(InputParameter&);
private:
    uint32_t input_null;        //输入socket的缓存为空的次数
    uint32_t type_error;        //输入类型错误的次数
    uint32_t parameter_error;   //输入参数错误的次数
};

class OutputParameter : public LayerParameter{
public:
    OutputParameter() : LayerParameter(), output_success(0), output_fail(0) {}
    ~OutputParameter() override = default;

    void onOutputMedia(uint32_t byte = 0);
    void onOutputError(uint32_t byte = 0);
    void onOutputOther(uint32_t byte = 0);
    void onOutputText(uint32_t byte = 0);
protected:
    void onClear();
    void onCopyData(ParameterTransmit*);
    void onFuseData(OutputParameter&);
private:
    uint32_t output_success;    //输出成功次数
    uint32_t output_fail;       //输出失败次数
};

class SpecificParameter : public LayerParameter {
public:
    SpecificParameter() : LayerParameter(), output_number(0) {};
    ~SpecificParameter() override = default;

    void onSpecificMeaid(uint32_t);
    void onSpecificError(uint32_t);
    void onSpecificOther(uint32_t);
    void onSpecificText(uint32_t);

    void onClear();
    void onCopyData(ParameterTransmit*);
private:
    uint32_t output_number;     //特化地址输出次数
};

class TransmitParameter : public OutputParameter, public InputParameter{
public:
    TransmitParameter() : OutputParameter(), InputParameter(), link_number(0), seq_number(0), obtain_null(0),
                          syn_number(0, 0), join_number(0, 0), exit_number(0, 0), query_number(0, 0), match_number(0, 0){}
    ~TransmitParameter() override = default;

    void onInputLink();
    void onInputSequence();
    void onInputSynchro(uint32_t, uint32_t);
    void onInputJoin(uint32_t, uint32_t);
    void onInputExit(uint32_t, uint32_t);
    void onInputQuery(uint32_t, uint32_t);
    void onInputMatch(bool);

    void onUnObtain();
    void onClear();
    void onFuseData(TransmitParameter&);
    void onCopyData(ParameterTransmit*);
private:
    uint32_t link_number;       //链接次数
    uint32_t seq_number;        //序号次数
    uint32_t obtain_null;       //获取内存失败次数

    ParameterType syn_number;   //同步和响应的次数
    ParameterType join_number;  //加入和响应的次数
    ParameterType exit_number;  //退出和响应的次数
    ParameterType query_number; //查询和响应的次数
    ParameterType match_number; //地址匹配和不匹配的次数
};


#endif //TEXTGDB_TRANSMITPARAMETER_H
