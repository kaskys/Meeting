//
// Created by abc on 20-12-30.
//
#include "BasicManager.h"

BasicManager::BasicManager(TimeLayer *control, uint32_t buffer_size) : time_control(control), range_size(0) {
    onUpdateSpace(buffer_size);
}

/**
 * 更改储存空间（储存TimeBuffer）
 *  *根据帧超时时间更新空间
 * @param range 空间数量
 */
void BasicManager::onUpdateSpace(uint32_t range) {
    if((range > 0) && ((range = size_round_up(range, size_round_n)) > range_size)){
        onUpdateBufferRange(range);
    }else{
        //单线程调用,无需担心多线程
        onClearBufferRange();
    }
}

void BasicManager::setRangeSize(uint32_t range) {
    if((range_size = range) <= 0){
        range_factor = 0;
    }else{
        for(uint32_t bit = 1; ;bit++){
            if(!(range ^ (1 << bit))){
                range_factor = bit;
                break;
            }
        }
    }
}
