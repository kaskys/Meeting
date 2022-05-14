//
// Created by abc on 20-12-8.
//

#include "BasicControl.h"

void LayerFilter::clear() {
    shared_.store(0, std::memory_order_release);
    response_.store(0, std::memory_order_release);
    for(short pos = 0; pos < LAYER_MATER_TYPE_SIZE; pos++){
        clrMasterFilter(pos);
    }
}

void LayerFilter::setFilter(const FilterInfo &filter) {
    setMasterFilter(filter.master_type);
    setSharedFilter(filter.shared_type);
    setResponseFilter(filter.response_type);
}

void LayerFilter::clrFilter(const FilterInfo &filter) {
    clrMasterFilter(filter.master_type);
    clrSharedFilter(filter.shared_type);
    clrResponseFilter(filter.response_type);
}

bool LayerFilter::onFilterMaster(short filter) {
    return (master_[filter].load(std::memory_order_consume));
}

bool LayerFilter::onFilterShared(short filter) {
    return static_cast<bool>((shared_.load(std::memory_order_consume) & filter));
}

bool LayerFilter::onFilterResponse(short filter) {
    return static_cast<bool>((response_.load(std::memory_order_consume) & filter));
}

void LayerFilter::setMasterFilter(short filter) {
    master_[filter].store(true, std::memory_order_release);
}

void LayerFilter::setSharedFilter(short filter) {
    short value = (shared_.load(std::memory_order_consume) | filter);
    shared_.store(value, std::memory_order_release);
}

void LayerFilter::setResponseFilter(short filter) {
    short value = (response_.load(std::memory_order_consume) | filter);
    response_.store(value, std::memory_order_release);
}

void LayerFilter::clrMasterFilter(short filter) {
    master_[filter].store(false, std::memory_order_release);
}

void LayerFilter::clrSharedFilter(short filter) {
    short value = (shared_.load(std::memory_order_consume) & (~filter));
    shared_.store(value, std::memory_order_release);
}

void LayerFilter::clrResponseFilter(short filter) {
    short value = (response_.load(std::memory_order_consume) & (~filter));
    response_.store(value, std::memory_order_release);
}

//--------------------------------------------------------------------------------------------------------------------//

LayerUtil* BasicLayer::layer_util[LAYER_TYPE_SIZE] = {&executor_util, &memory_util, &address_util, &timer_util, &transmit_util, &display_util, &user_util};

BasicLayer::BasicLayer(BasicControl *control) : basic_control(control) {
    
}

BasicLayer* BasicLayer::createLayer(BasicControl *control, LayerType type) {
    return layer_util[type]->createLayer(control);
}

void BasicLayer::destroyLayer(BasicControl *control, BasicLayer *layer) {
    layer_util[layer->onLayerType()]->destroyLayer(control, layer);
}

/**
 * 创建固定内存
 * @param create_len
 * @return
 */
char* BasicLayer::onCreateBuffer(uint32_t create_len) {
    if((create_len <= 0)){
        return nullptr;
    }

    MsgHdr *create_msg = nullptr;
    char msg_buffer[sizeof(MsgHdr) + sizeof(char*)];

    basic_control->onCommonInput(
            (create_msg = MsgHdrUtil<char*>::initMsgHdr(reinterpret_cast<MsgHdr*>(msg_buffer), create_len, LAYER_CONTROL_REQUEST_ALLOC_FIXED, nullptr)),
            CONTROL_INPUT_FLAG_CONTROL);

    return (create_msg->serial_number >= create_len) ? create_msg->buffer : nullptr;
}

/**
 * 创建动态内存
 * @param callback_func
 */
void BasicLayer::onCreateBuffer(const std::function<void(MsgHdr*)> &cinit_func, const std::function<void(MsgHdr*)> &callback_func) {
    if(!cinit_func || !callback_func){
        return;
    }

    MsgHdr *create_msg = nullptr;
    char msg_buffer[sizeof(MsgHdr) + sizeof(InitLocator)];

    {
        cinit_func((create_msg = reinterpret_cast<MsgHdr*>(msg_buffer)));
        create_msg->shared_type = onLayerType();
        basic_control->onCommonInput(create_msg, CONTROL_INPUT_FLAG_CONTROL);
    }

    callback_func(create_msg);
}

void BasicLayer::onDestroyBuffer(uint32_t len, const std::function<void(MsgHdr*)> &callback_func) {
    if((len <= 0) || !callback_func){
        return;
    }

    MsgHdr *destroy_msg = nullptr;
    char msg_buffer[sizeof(MsgHdr) + len];

    callback_func((destroy_msg = reinterpret_cast<MsgHdr*>(msg_buffer)));
    destroy_msg->shared_type = onLayerType();
    basic_control->onCommonInput(destroy_msg, CONTROL_INPUT_FLAG_CONTROL);
}