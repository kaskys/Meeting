//
// Created by abc on 20-12-30.
//

#ifndef TEXTGDB_BASICMANAGER_H
#define TEXTGDB_BASICMANAGER_H

#include <sstream>
#include <cstring>
#include <shared_mutex>

#define size_round_up(size, n)      (((size > 0) && (n > 0)) ? (((size) + (1 << n) - 1) & ~((1 << n) - 1)) : 0)
#define size_round_down(size, n)    (((size > 0) && (n > 0)) ? (size & ((1 << n) - 1)) : 0)
#define size_round_n                3

class TimeLayer;
struct MediaData;

class BasicManager {
public:
    BasicManager(TimeLayer*, uint32_t);
    virtual ~BasicManager() = default;

    void onUpdateSpace(uint32_t);
    virtual void onUpdateTime(uint32_t) = 0;
    virtual bool onSubmitBuffer(MediaData*, uint32_t) = 0;
    virtual void onReuseManager(uint32_t, uint32_t) = 0;
    virtual void onUnuseManager() = 0;
protected:
    void setRangeSize(uint32_t range);
    virtual void onClearBufferRange() = 0;
    virtual void onUpdateBufferRange(uint32_t) = 0;

    uint32_t range_factor;
    uint32_t range_size;
    TimeLayer *time_control;
};


#endif //TEXTGDB_BASICMANAGER_H
