//
// Created by abc on 20-12-9.
//
#include "../BasicControl.h"

BasicLayer* MemoryLayerUtil::createLayer(BasicControl *control) noexcept {
    return (new (std::nothrow) MemoryLayer(!control->onCoreControl(), control));
}

void MemoryLayerUtil::destroyLayer(BasicControl *, BasicLayer *layer) noexcept {
    delete layer;
}

//--------------------------------------------------------------------------------------------------------------------//

MemoryLayer::MemoryLayer(bool ban, BasicControl *control) : BasicLayer(control), ban_variety_memory(ban), space_control(nullptr) {
    initLayer();
}

void MemoryLayer::initLayer() {
    memory_parameter[0] = dynamic_cast<BasicParameter*>(&address_parameter);
    memory_parameter[1] = dynamic_cast<BasicParameter*>(&transmission_parameter);
    memory_parameter[2] = dynamic_cast<BasicParameter*>(&control_parameter);
    memory_parameter[3] = dynamic_cast<BasicParameter*>(&time_parameter);
}

void MemoryLayer::onInput(MsgHdr *msg) {
    int memory_len = 0, memory_layer = 0;
    BasicParameter *parameter = nullptr;

    if(msg){
        memory_len = application_len(msg);
        if((memory_layer = application_layer(msg)) < MEMORY_LAYER_PARAMETER_SIZE){
            parameter = memory_parameter[memory_layer];
        }else{
            msg->serial_number = 0; return;
        }

        switch (msg->master_type){
            case MEMORY_LAYER_VARIETY_MEMORY:
                if(!ban_variety_memory){
                    try{
                        InitLocator memory_locator = space_control->malloc(static_cast<uint32_t>(memory_len));

                        memory_locator.setOnReleaseMemory(dynamic_cast<VarietyParameter*>(parameter)->getCallBack());

                        MsgHdrUtil<InitLocator>::initMsgHdr(msg, sizeof(InitLocator),
                                                            static_cast<uint32_t>(msg->master_type), memory_locator);

                        parameter->onVarietyComplete(static_cast<uint32_t>(memory_len));
                    }catch (std::bad_alloc &e){
                        msg->serial_number = 0;
                        parameter->onVarietyComplete(0);
                    }
                }else{
                    msg->serial_number = 0;
                    msg->response_type = MEMORY_LAYER_BAN_VARIETY_MEMORY;
                    parameter->onVarietyComplete(0);
                }
                break;
            case MEMORY_LAYER_FIXED_MEMORY:
                try {
                    MsgHdrUtil<void*>::initMsgHdr(msg, sizeof(void*), static_cast<uint32_t>(msg->master_type),
                                               space_control->malloc(static_cast<uint32_t>(memory_len), MemorySpace()));
                    parameter->onFixedComplete(static_cast<uint32_t>(memory_len));
                }catch (std::bad_alloc &e){
                    msg->serial_number = 0;
                    parameter->onFixedComplete(0);
                }
                break;
            case MEMORY_LAYER_RELEASE_MEMORY:
                {
                    auto rbuffer = reinterpret_cast<void*>(msg->buffer);
                    if (rbuffer) {
                        space_control->free(rbuffer);
                        parameter->onFixedRelease(static_cast<uint32_t>(memory_len));
                    }
                }
                break;
            default:
                break;
        }

    }
}

void MemoryLayer::onOutput() {}

void MemoryLayer::onDrive(MsgHdr*) {}

void MemoryLayer::onParameter(MsgHdr *msg) {
    MemoryParameter *msg_parameter = nullptr;

    if((application_len(msg) < (sizeof(MemoryParameter))) || !(msg_parameter = reinterpret_cast<MemoryParameter*>(msg->buffer))){
        application_len(msg) = 0;
    }else {
        BasicParameter *parameter = nullptr;

        for(int i = 0; i < MEMORY_LAYER_PARAMETER_SIZE; i++){
            parameter = memory_parameter[i];
            parameter->onLayerParameter(msg_parameter);
        }
        msg_parameter->amount_runtime_variety = msg_parameter->amount_application_variety - msg_parameter->amount_release_variety;
        msg_parameter->amount_runtime_fixed = msg_parameter->amount_application_fixed - msg_parameter->amount_release_fixed;

        application_len(msg) = sizeof(MemoryParameter);
    }
}

void MemoryLayer::onControl(MsgHdr *msg) {
    switch (msg->master_type){
        case LAYER_CONTROL_STATUS_START:
            if(!onLaunchLayer()) { msg->master_type = LAYER_CONTROL_STATUS_THROW; }
            break;
        case LAYER_CONTROL_STATUS_STOP:
            onStopLayer();
            break;
        case MEMORY_LAYER_CONTROL_VARIETY:
            onControlVariety(msg);
            break;
        case MEMORY_LAYER_CONTROL_FIXED:
            onControlFixed(msg);
            break;
        default:
            break;
    }
}

bool MemoryLayer::onLaunchLayer() {
    if((space_control = memory_alloc.allocator<SpaceControl>(sizeof(SpaceControl)))){
        memory_alloc.construct(space_control);
    }
    return static_cast<bool>(space_control);
}

void MemoryLayer::onStopLayer() {
    if(space_control){
        memory_alloc.destroy(space_control);
        memory_alloc.deallocator(space_control);
        space_control = nullptr;
    }
    time_parameter.onClear();
    address_parameter.onClear();
    control_parameter.onClear();
    transmission_parameter.onClear();
}

void MemoryLayer::onControlFixed(MsgHdr *model_msg) {
    MemoryFixedModelData *model_data = nullptr;
    if((application_len(model_msg) < sizeof(MemoryFixedModelData)) || !(model_data = reinterpret_cast<MemoryFixedModelData*>(model_msg->buffer))){
        return;
    }
    onUpdateFixedModel(model_data);
}

void MemoryLayer::onControlVariety(MsgHdr *variety_msg) {
    if(application_len(variety_msg) < sizeof(int)){
        return;
    }
#ifdef centralizetion_uncore
    return;
#elif  centralizetion_core
    ban_variety_memory = !(*(reinterpret_cast<int*>(variety_msg->buffer)));
#endif
}

/**
 * 更新固定内存缓存模式 /XX暂不实现,默认无限模式XX/
 * @param model_data
 */
void MemoryLayer::onUpdateFixedModel(MemoryFixedModelData*) {}