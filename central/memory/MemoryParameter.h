//
// Created by abc on 20-12-15.
//

#ifndef TEXTGDB_MEMORYPARAMETER_H
#define TEXTGDB_MEMORYPARAMETER_H

#include <cstdint>
#include <functional>

struct MemoryParameter {
    struct MemoryAmount {
        uint32_t application_variety;
        uint32_t application_fixed;
        uint32_t runtime_variety;
        uint32_t runtime_fixed;
        uint32_t release_variety;
        uint32_t release_fixed;

        uint32_t address_application;
        uint32_t address_release;

        uint32_t transmission_application;
        uint32_t transmission_release;

        uint32_t control_application;
        uint32_t control_release;

        uint32_t time_application;
        uint32_t time_release;
    };

    struct MemoryNumber{
        uint32_t success_application_variety;
        uint32_t success_application_fixed;
        uint32_t fail_application_variety;
        uint32_t fail_application_fixed;

        uint32_t address_application;
        uint32_t address_release;

        uint32_t transmission_application;
        uint32_t transmission_release;

        uint32_t control_application;
        uint32_t control_release;

        uint32_t time_application;
        uint32_t time_release;
    };

    MemoryAmount memory_amount;
    MemoryNumber memory_number;
};

#define amount_application_variety         memory_amount.application_variety
#define amount_application_fixed           memory_amount.application_fixed
#define amount_runtime_variety             memory_amount.runtime_variety
#define amount_runtime_fixed               memory_amount.runtime_fixed
#define amount_release_variety             memory_amount.release_variety
#define amount_release_fixed               memory_amount.release_fixed

#define amount_address_application         memory_amount.address_application
#define amount_address_release             memory_amount.address_release
#define amount_transmission_application    memory_amount.transmission_application
#define amount_transmission_release        memory_amount.transmission_release
#define amount_control_application         memory_amount.control_application
#define amount_control_release             memory_amount.control_release
#define amount_time_application            memory_amount.time_application
#define amount_time_release                memory_amount.time_release

#define number_success_application_variety memory_number.success_application_variety
#define number_success_application_fixed   memory_number.success_application_fixed
#define number_fail_application_variety    memory_number.fail_application_variety
#define number_fail_application_fixed      memory_number.fail_application_fixed

#define number_address_application         memory_number.address_application
#define number_address_release             memory_number.address_release
#define number_transmission_application    memory_number.transmission_application
#define number_transmission_release        memory_number.transmission_release
#define number_control_application         memory_number.control_application
#define number_control_release             memory_number.control_release
#define number_time_application            memory_number.time_application
#define number_time_release                memory_number.time_release


struct BasicParameter{
public:
    BasicParameter() : number_release(0) {}
    virtual ~BasicParameter() = default;

    virtual void onClear() = 0;

    virtual void onVarietyComplete(uint32_t) = 0;
    virtual void onFixedComplete(uint32_t) = 0;

    virtual void onVarietyRelease(uint32_t) = 0;
    virtual void onFixedRelease(uint32_t) = 0;

    virtual uint32_t getAmountApplicationVariety() const = 0;
    virtual uint32_t getAmountApplicationFixed() const = 0;
    virtual uint32_t getAmountReleaseVariety() const = 0;
    virtual uint32_t getAmountReleaseFixed() const = 0;

    virtual uint32_t getNumberSuccessVariety() const = 0;
    virtual uint32_t getNumberSuccessFixed() const = 0;
    virtual uint32_t getNumberFailVariety() const = 0;
    virtual uint32_t getNumberFailFixed() const = 0;

    uint32_t getNumberRelease() const { return number_release; }
    void onLayerParameter(MemoryParameter*);
protected:
    virtual void onLayerParameter0(MemoryParameter*) = 0;
    void onReleaseMemory() {
        number_release++;
    }
    void clearParameter(){ number_release = 0; }

    uint32_t number_release;
};

struct VarietyParameter : virtual public BasicParameter{
public:
    VarietyParameter() : vamount_application(0), vamount_release(0), vnumber_success(0), vnumber_fail(0), call_back(nullptr) {
        initCallBack();
    }
    ~VarietyParameter() override = default;

    virtual std::function<void(int)>* getCallBack() { return &call_back; }

    void onVarietyComplete(uint32_t) override;
    void onVarietyRelease(uint32_t) override;

    uint32_t getAmountApplicationVariety() const override;
    uint32_t getAmountReleaseVariety() const override;

    uint32_t getNumberSuccessVariety() const override;
    uint32_t getNumberFailVariety() const override;
protected:
    void initCallBack(){
        call_back = [=](uint32_t len) -> void { return onVarietyRelease(len);  };
    }
    void clearVarierty(){
        clearParameter();
        vamount_application = 0; vamount_release = 0;
        vnumber_success = 0; vnumber_fail = 0;
    }

    uint32_t vamount_application;
    uint32_t vamount_release;
    uint32_t vnumber_success;
    uint32_t vnumber_fail;

    std::function<void(int)> call_back;
};

struct FixedParameter : virtual public BasicParameter{
public:
    FixedParameter() : famount_application(0), famount_release(0), fnumber_success(0), fnumber_fail(0) {}
    ~FixedParameter() override = default;

    void onFixedComplete(uint32_t) override;
    void onFixedRelease(uint32_t) override;

    uint32_t getAmountApplicationFixed() const override;
    uint32_t getAmountReleaseFixed() const override;

    uint32_t getNumberSuccessFixed() const override;
    uint32_t getNumberFailFixed() const override;
protected:
    void clearFixed(){
        clearParameter();
        famount_application = 0; famount_release = 0;
        fnumber_success = 0; fnumber_fail = 0;
    }

    uint32_t famount_application;
    uint32_t famount_release;
    uint32_t fnumber_success;
    uint32_t fnumber_fail;
};

struct AddressMemoryParameter : public VarietyParameter, public FixedParameter{
public:
    AddressMemoryParameter() : VarietyParameter(), FixedParameter(), BasicParameter() {}
    ~AddressMemoryParameter() override = default;

    void onClear() override { clearVarierty(); clearFixed();}
protected:
    void onLayerParameter0(MemoryParameter*) override;
};

struct TransmitMemoryParameter : public VarietyParameter, public FixedParameter{
public:
    TransmitMemoryParameter() : VarietyParameter(), FixedParameter(), BasicParameter() {}
    ~TransmitMemoryParameter() override = default;

    void onClear() override { clearVarierty(); clearFixed();}
protected:
    void onLayerParameter0(MemoryParameter*) override;
};

struct ControlMemoryParameter : public VarietyParameter, public FixedParameter{
public:
    ControlMemoryParameter() : VarietyParameter(), FixedParameter(), BasicParameter() {}
    ~ControlMemoryParameter() override = default;

    void onClear() override { clearVarierty(); clearFixed();}
protected:
    void onLayerParameter0(MemoryParameter*) override;
};

struct TimeMemoryParameter : public VarietyParameter, public FixedParameter{
public:
    TimeMemoryParameter() : VarietyParameter(), FixedParameter(), BasicParameter() {}
    ~TimeMemoryParameter() override = default;

    void onClear() override { clearVarierty(); clearFixed();}
protected:
    void onLayerParameter0(MemoryParameter*) override;
};

#endif //TEXTGDB_MEMORYPARAMETER_H
