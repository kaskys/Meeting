//
// Created by abc on 21-3-8.
//

#ifndef TEXTGDB_ADDRESSTIMERUTIL_H
#define TEXTGDB_ADDRESSTIMERUTIL_H

#include "../../../executor/task/ExecutorTask.h"
#include "../../BasicLayer.h"

#define NOTE_TIMER_TYPE_INIT        0x80000000      //初始化
#define NOTE_TIMER_TYPE_INFO        0x40000000      //创建数据
#define NOTE_TIMER_TYPE_JOIN        1               //加入定时器
#define NOTE_TIMER_TYPE_LINK        2               //链接定时器
#define NOTE_TIMER_TYPE_SEQUENCE    3               //序号定时器
#define NOTE_TIMER_TYPE_PASSIVE     4               //被动退出定时器

/**
 * 地址层定时器信息类
 */
class NoteTimerInfo {
public:
    explicit NoteTimerInfo(int type);
    explicit NoteTimerInfo(int type, ExecutorTimerData&&);
    virtual ~NoteTimerInfo();

//    NoteTimerInfo(const NoteTimerInfo&) = delete;
//    NoteTimerInfo& operator=(const NoteTimerInfo&) = delete;
    NoteTimerInfo(NoteTimerInfo&&) noexcept;
    NoteTimerInfo& operator=(NoteTimerInfo&& noteTimerInfo) noexcept;

    void setTimerInfo(NoteTimerInfo*) noexcept;
    void setTimerHolder(TaskHolder<void>&&) &;
    ExecutorTimerData getTimerData() &;
    ExecutorTimerData getTimerData() &&;

    static NoteTimerInfo initNoteTimerInfo(int , uint64_t, uint64_t, uint64_t, const std::function<void()>&, const std::function<void()>&);
protected:
    void clearNoteTimerInfo();

    int timer_type;                 //定时器类型（数据、定时器）
    ExecutorTimerData timer_data;   //定时器信息（未创建定时器）
    TaskHolder<void> timer_holder;  //定时器持有器（已创建定时器）
};

#endif //TEXTGDB_ADDRESSTIMERUTIL_H
