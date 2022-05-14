//
// Created by abc on 20-12-9.
//

#ifndef TEXTGDB_MEMORYLAYER_H
#define TEXTGDB_MEMORYLAYER_H

#include "../BasicLayer.h"
#include "MemoryParameter.h"

#define application_len(msg)    (msg->serial_number)
#define application_layer(msg)  (msg->shared_type)

#define MEMORY_LAYER_VARIETY_MEMORY         1
#define MEMORY_LAYER_FIXED_MEMORY           2
#define MEMORY_LAYER_RELEASE_MEMORY         3
#define MEMORY_LAYER_BAN_VARIETY_MEMORY     4

#define MEMORY_LAYER_CONTROL_VARIETY        3
#define MEMORY_LAYER_CONTROL_FIXED          4

#define MEMORY_LAYER_PARAMETER_SIZE         4

static thread_local TimeMemoryParameter        time_parameter{};
static thread_local AddressMemoryParameter     address_parameter{};
static thread_local ControlMemoryParameter     control_parameter{};
static thread_local TransmitMemoryParameter    transmission_parameter{};

enum ModelType{
    MEMORY_MODEL_INFINITE,      //无限制
    MOMORY_MODEL_LIMIT,         //限制(最大值、最小值)
    MOMORY_MODEL_PAGE,          //对齐页(包含最大值、最小值)
};

//固定内存模式
struct MemoryFixedModelData{
    int max_len;        //最大长度
    int min_len;        //最小长度
    int align_multiple; //对齐倍数
    ModelType type;     //模式类型
};

class MemoryLayerUtil : public LayerUtil{
public:
    BasicLayer* createLayer(BasicControl*) noexcept override;
    void destroyLayer(BasicControl*, BasicLayer*) noexcept override;
};

class MemoryLayer : public BasicLayer{
public:
    explicit MemoryLayer(bool ban, BasicControl*);
    ~MemoryLayer() override = default;

    void initLayer() override;
    void onInput(MsgHdr*) override;
    void onOutput() override;
    bool isDrive() const override { return false; }
    void onDrive(MsgHdr*) override;
    void onParameter(MsgHdr*) override;
    void onControl(MsgHdr*) override;
    uint32_t onStartLayerLen() const override { return 0; }
    LayerType onLayerType() const override { return LAYER_MEMORY_TYPE; }
private:
    bool onLaunchLayer();
    void onStopLayer();

    void onControlFixed(MsgHdr*);
    void onControlVariety(MsgHdr*);

    void onUpdateFixedModel(MemoryFixedModelData*);

    bool ban_variety_memory;            //是否禁止动态内存
    SpaceControl *space_control;        //内存控制器
    BasicParameter *memory_parameter[MEMORY_LAYER_PARAMETER_SIZE];  //内存层参数类
};


#endif //TEXTGDB_MEMORYLAYER_H
