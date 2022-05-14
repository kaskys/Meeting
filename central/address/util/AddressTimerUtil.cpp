//
// Created by abc on 21-3-8.
//

#include "AddressTimerUtil.h"

NoteTimerInfo::NoteTimerInfo(int type) : timer_type(type & (~(NOTE_TIMER_TYPE_INIT | NOTE_TIMER_TYPE_INFO))), timer_data(), timer_holder(){}

NoteTimerInfo::NoteTimerInfo(int type, ExecutorTimerData &&data) : timer_type((type | NOTE_TIMER_TYPE_INIT) & (~NOTE_TIMER_TYPE_INFO)),
                                                                        timer_data(std::move(data)), timer_holder(){}
NoteTimerInfo::~NoteTimerInfo() {
    if(timer_type | NOTE_TIMER_TYPE_INFO){ timer_holder.cancel(); }
}

NoteTimerInfo::NoteTimerInfo(NoteTimerInfo &&timer_info) noexcept {
    if(timer_info.timer_type | NOTE_TIMER_TYPE_INFO){
        timer_data = ExecutorTimerData();
        timer_holder = std::move(timer_info.timer_holder);
    }
    if(timer_info.timer_type | NOTE_TIMER_TYPE_INIT){
        timer_data = std::move(timer_info.timer_data);
        timer_holder = TaskHolder<void>();
    }
    timer_type = timer_info.timer_type; timer_info.timer_type &= (~(NOTE_TIMER_TYPE_INIT | NOTE_TIMER_TYPE_INFO));
}

NoteTimerInfo& NoteTimerInfo::operator=(NoteTimerInfo &&timer_info) noexcept {
    if(timer_info.timer_type | NOTE_TIMER_TYPE_INFO){
        timer_data = ExecutorTimerData();
        timer_holder = std::move(timer_info.timer_holder);
    }
    if(timer_info.timer_type | NOTE_TIMER_TYPE_INIT){
        timer_data = std::move(timer_info.timer_data);
        timer_holder = TaskHolder<void>();
    }
    timer_type = timer_info.timer_type; timer_info.timer_type &= (~(NOTE_TIMER_TYPE_INIT | NOTE_TIMER_TYPE_INFO));
    return *this;
}

NoteTimerInfo NoteTimerInfo::initNoteTimerInfo(int type, uint64_t amount, uint64_t microsecond, uint64_t increase_microsecond,
                                               const std::function<void()> &efunc, const std::function<void()> &tfunc) {
    return NoteTimerInfo(type, ExecutorTimerData(amount, microsecond, increase_microsecond, efunc, tfunc));
}

void NoteTimerInfo::setTimerInfo(NoteTimerInfo *timer_info) noexcept {
    *this = std::move(*timer_info);
}

void NoteTimerInfo::setTimerHolder(TaskHolder<void> &&holder) & {
    this->timer_holder = std::move(holder);
    timer_type &= NOTE_TIMER_TYPE_INFO;
}

ExecutorTimerData NoteTimerInfo::getTimerData() & {
    if(timer_type | NOTE_TIMER_TYPE_INIT){
        return timer_data;
    }else{
        return ExecutorTimerData();
    }
}

ExecutorTimerData NoteTimerInfo::getTimerData() && {
    if(timer_type | NOTE_TIMER_TYPE_INIT){
        return std::move(timer_data);
    }else{
        return ExecutorTimerData();
    }
}

void NoteTimerInfo::clearNoteTimerInfo() {
    if(timer_type | NOTE_TIMER_TYPE_INFO){
        timer_holder.cancel();
        timer_type &= ~NOTE_TIMER_TYPE_INFO;
    }
    if(timer_type | NOTE_TIMER_TYPE_INIT){
        timer_data = ExecutorTimerData();
        timer_type &= ~NOTE_TIMER_TYPE_INIT;
    }
}