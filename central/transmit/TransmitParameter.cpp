//
// Created by abc on 21-2-13.
//

#include "TransmitParameter.h"

void LayerParameter::onMediaParameter(uint32_t byte) {
    total_byte += byte;
    media_byte += byte;
}

void LayerParameter::onErrorParameter(uint32_t byte) {
    total_byte += byte;
    error_byte += byte;
}

void LayerParameter::onOtherParameter(uint32_t byte) {
    total_byte += byte;
    other_byte += byte;
}

void LayerParameter::onTextParameter(uint32_t byte) {
    total_byte += byte;
    text_byte += byte;
}

void LayerParameter::onClear() {
    this->total_byte = 0; this->media_byte = 0; this->error_byte = 0;
    this->other_byte = 0; this->text_byte = 0;
}

void LayerParameter::onFuseData(LayerParameter &parameter) {
    this->total_byte += parameter.total_byte;
    this->media_byte += parameter.media_byte;
    this->error_byte += parameter.error_byte;
    this->other_byte += parameter.other_byte;
    this->text_byte += parameter.text_byte;
}

void LayerParameter::onCopyData(uint32_t &tbyte, uint32_t &mbyte, uint32_t &ebyte, uint32_t &obyte, uint32_t &xbyte) {
    tbyte = this->total_byte;
    mbyte = this->media_byte;
    ebyte = this->error_byte;
    obyte = this->other_byte;
    xbyte = this->text_byte;
}

//--------------------------------------------------------------------------------------------------------------------//

void InputParameter::onUnSocket(uint32_t size) {
    input_null += size;
}

void InputParameter::onParameterError(uint32_t size) {
    parameter_error += size;
}

void InputParameter::onTypeError(uint32_t size) {
    type_error += size;
}

void InputParameter::onInputMedia(uint32_t byte) {
    LayerParameter::onMediaParameter(byte);
}

void InputParameter::onInputError(uint32_t byte) {
    LayerParameter::onErrorParameter(byte);
}

void InputParameter::onInputOther(uint32_t byte) {
    LayerParameter::onOtherParameter(byte);
}

void InputParameter::onInputText(uint32_t byte) {
    LayerParameter::onTextParameter(byte);
}

void InputParameter::onClear() {
    input_null = 0; type_error = 0; parameter_error = 0;
    LayerParameter::onClear();
}

void InputParameter::onFuseData(InputParameter &parameter) {
    this->input_null += parameter.input_null;
    this->type_error += parameter.type_error;
    this->parameter_error += parameter.parameter_error;
    LayerParameter::onFuseData(parameter);
}

void InputParameter::onCopyData(ParameterTransmit *parameter) {
    parameter->input_null = this->input_null;
    parameter->type_error = this->type_error;
    parameter->parameter_error = this->parameter_error;
    LayerParameter::onCopyData(parameter->input_total_byte,
                               parameter->input_media_byte,
                               parameter->input_error_byte,
                               parameter->input_other_byte,
                               parameter->input_text_byte);
}

//--------------------------------------------------------------------------------------------------------------------//

void OutputParameter::onOutputMedia(uint32_t byte) {
    (byte > 0) ? output_success++ : output_fail++;
    LayerParameter::onMediaParameter(byte);
}

void OutputParameter::onOutputError(uint32_t byte) {
    (byte > 0) ? output_success++ : output_fail++;
    LayerParameter::onErrorParameter(byte);
}

void OutputParameter::onOutputOther(uint32_t byte) {
    (byte > 0) ? output_success++ : output_fail++;
    LayerParameter::onOtherParameter(byte);
}

void OutputParameter::onOutputText(uint32_t byte) {
    (byte > 0) ? output_success++ : output_fail++;
    LayerParameter::onTextParameter(byte);
}

void OutputParameter::onClear() {
    output_success = 0; output_fail = 0;
    LayerParameter::onClear();
}

void OutputParameter::onFuseData(OutputParameter &parameter) {
    this->output_fail += parameter.output_fail;
    this->output_success += parameter.output_success;
    LayerParameter::onFuseData(parameter);
}

void OutputParameter::onCopyData(ParameterTransmit *parameter) {
    parameter->output_fail = this->output_fail;
    parameter->output_success = this->output_success;
    LayerParameter::onCopyData(parameter->output_total_byte,
                               parameter->output_media_byte,
                               parameter->output_error_byte,
                               parameter->output_other_byte,
                               parameter->output_text_byte);
}

//--------------------------------------------------------------------------------------------------------------------//


void SpecificParameter::onSpecificMeaid(uint32_t byte) {
    output_number++;
    LayerParameter::onMediaParameter(byte);
}

void SpecificParameter::onSpecificError(uint32_t byte) {
    output_number++;
    LayerParameter::onErrorParameter(byte);
}

void SpecificParameter::onSpecificOther(uint32_t byte) {
    output_number++;
    LayerParameter::onOtherParameter(byte);
}

void SpecificParameter::onSpecificText(uint32_t byte) {
    output_number++;
    LayerParameter::onTextParameter(byte);
}

void SpecificParameter::onClear() {
    output_number = 0;
    LayerParameter::onClear();
}

void SpecificParameter::onCopyData(ParameterTransmit *parameter) {
    parameter->output_success = this->output_number;
    LayerParameter::onCopyData(parameter->output_total_byte,
                               parameter->output_media_byte,
                               parameter->output_error_byte,
                               parameter->output_other_byte,
                               parameter->output_text_byte);
}


//--------------------------------------------------------------------------------------------------------------------//

void TransmitParameter::onInputLink() {
    link_number++;
}

void TransmitParameter::onInputSequence() {
    seq_number++;
}

void TransmitParameter::onInputSynchro(uint32_t synchro, uint32_t response) {
    syn_number.first += synchro;
    syn_number.second += response;
}

void TransmitParameter::onInputJoin(uint32_t join, uint32_t response) {
    join_number.first += join;
    join_number.second += response;
}

void TransmitParameter::onInputExit(uint32_t exit, uint32_t response) {
    exit_number.first += exit;
    exit_number.second += response;
}

void TransmitParameter::onInputQuery(uint32_t query, uint32_t response) {
    query_number.first += query;
    query_number.second += response;
}

void TransmitParameter::onInputMatch(bool is_match) {
    is_match ? match_number.first++ : match_number.second++;
}

void TransmitParameter::onUnObtain() {
    obtain_null++;
}

void TransmitParameter::onClear() {
    link_number = 0; seq_number = 0; obtain_null = 0;
    syn_number = {0, 0}; join_number = {0, 0};
    exit_number = {0, 0}; query_number = {0, 0}; match_number = {0, 0};
    InputParameter::onClear();
    OutputParameter::onClear();
}

void TransmitParameter::onFuseData(TransmitParameter &parameter) {
    this->link_number += parameter.link_number;
    this->seq_number += parameter.seq_number;
    this->obtain_null += parameter.obtain_null;

    this->syn_number.first += parameter.syn_number.first;
    this->syn_number.second += parameter.syn_number.second;

    this->join_number.first += parameter.join_number.first;
    this->join_number.second += parameter.join_number.second;

    this->exit_number.first += parameter.exit_number.first;
    this->exit_number.second += parameter.exit_number.second;

    this->query_number.first += parameter.query_number.first;
    this->query_number.second += parameter.query_number.second;

    this->match_number.first += parameter.match_number.first;
    this->match_number.second += parameter.match_number.second;

    InputParameter::onFuseData(parameter);
    OutputParameter::onFuseData(parameter);
}

void TransmitParameter::onCopyData(ParameterTransmit *parameter) {
    parameter->link_number = this->link_number;
    parameter->seq_number = this->seq_number;
    parameter->obtain_null = this->obtain_null;

    parameter->syn_number = this->syn_number.first;
    parameter->syn_response_number = this->syn_number.second;

    parameter->join_number = this->join_number.first;
    parameter->join_response_number = this->join_number.second;

    parameter->exit_number = this->exit_number.first;
    parameter->exit_response_number = this->exit_number.second;

    parameter->query_number = this->query_number.first;
    parameter->query_response_number = this->query_number.second;

    parameter->match_number = this->match_number.first;
    parameter->unmatch_number = this->match_number.second;

    InputParameter::onCopyData(parameter);
    OutputParameter::onCopyData(parameter);
}