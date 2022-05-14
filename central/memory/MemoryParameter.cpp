//
// Created by abc on 20-12-15.
//

#include "MemoryParameter.h"

void BasicParameter::onLayerParameter(MemoryParameter *parameter) {
    parameter->amount_application_variety += getAmountApplicationVariety();
    parameter->amount_application_fixed += getAmountApplicationFixed();
    parameter->amount_release_variety += getAmountReleaseVariety();
    parameter->amount_release_fixed += getAmountReleaseFixed();

    parameter->number_success_application_variety += getNumberSuccessVariety();
    parameter->number_success_application_fixed += getNumberSuccessFixed();
    parameter->number_fail_application_variety += getNumberFailVariety();
    parameter->number_fail_application_fixed += getNumberFailFixed();

    onLayerParameter0(parameter);
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

void VarietyParameter::onVarietyComplete(uint32_t len) {
    if(len > 0) {
        vamount_application += len; vnumber_success++;
    }else{
        vnumber_fail++;
    }
}

void VarietyParameter::onVarietyRelease(uint32_t len) {
    vamount_release += len;
    onReleaseMemory();
}

uint32_t VarietyParameter::getAmountApplicationVariety() const {
    return vamount_application;
}

uint32_t VarietyParameter::getAmountReleaseVariety() const {
    return vamount_release;
}

uint32_t VarietyParameter::getNumberSuccessVariety() const {
    return vnumber_success;
}

uint32_t VarietyParameter::getNumberFailVariety() const {
    return vnumber_fail;
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

void FixedParameter::onFixedComplete(uint32_t len) {
    if(len > 0) {
        famount_application += len; fnumber_success++;
    }else{
        fnumber_fail++;
    }
}

void FixedParameter::onFixedRelease(uint32_t len) {
    famount_release += len;
    onReleaseMemory();
}

uint32_t FixedParameter::getAmountApplicationFixed() const {
    return famount_application;
}

uint32_t FixedParameter::getAmountReleaseFixed() const {
    return famount_release;
}

uint32_t FixedParameter::getNumberSuccessFixed() const {
    return fnumber_success;
}
uint32_t FixedParameter::getNumberFailFixed() const {
    return fnumber_fail;
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

void AddressMemoryParameter::onLayerParameter0(MemoryParameter *parameter) {
    parameter->amount_address_application = getAmountApplicationVariety() + getAmountApplicationFixed();
    parameter->amount_address_release = getAmountReleaseVariety() + getAmountReleaseFixed();
    parameter->number_address_application = getNumberSuccessVariety() + getNumberSuccessFixed()
                                            + getNumberFailVariety() + getNumberFailFixed();
    parameter->number_address_release = getNumberRelease();
}

void TransmitMemoryParameter::onLayerParameter0(MemoryParameter *parameter) {
    parameter->amount_transmission_application = getAmountApplicationVariety() + getAmountApplicationFixed();
    parameter->amount_transmission_release = getAmountReleaseVariety() + getAmountReleaseFixed();
    parameter->number_transmission_application = getNumberSuccessVariety() + getNumberSuccessFixed()
                                            + getNumberFailVariety() + getNumberFailFixed();
    parameter->number_transmission_release = getNumberRelease();
}

void ControlMemoryParameter::onLayerParameter0(MemoryParameter *parameter) {
    parameter->amount_control_application = getAmountApplicationVariety() + getAmountApplicationFixed();
    parameter->amount_control_release = getAmountReleaseVariety() + getAmountReleaseFixed();
    parameter->number_control_application = getNumberSuccessVariety() + getNumberSuccessFixed()
                                            + getNumberFailVariety() + getNumberFailFixed();
    parameter->number_control_release = getNumberRelease();
}

void TimeMemoryParameter::onLayerParameter0(MemoryParameter *parameter) {
    parameter->amount_time_application = getAmountApplicationVariety() + getAmountApplicationFixed();
    parameter->amount_time_release = getAmountReleaseVariety() + getAmountReleaseFixed();
    parameter->number_time_application = getNumberSuccessVariety() + getNumberSuccessFixed()
                                            + getNumberFailVariety() + getNumberFailFixed();
    parameter->number_time_release = getNumberRelease();
}