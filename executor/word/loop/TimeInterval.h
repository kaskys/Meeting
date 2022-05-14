//
// Created by abc on 20-4-17.
//

#ifndef UNTITLED5_INTERVALMANAGER_H
#define UNTITLED5_INTERVALMANAGER_H

#include "TimeBinary.h"

#define comparator_func bool(*)(TimeInterval*, TaskInfo*, TaskInfo*, const interval_time_point&)
#define remove_func     void(*)(TimeInterval*, TaskInfo*)
class CorrelateBase;
class TimeInterval;
using ContainDynamicFunc = bool (*)(CorrelateBase*, TaskInfo*, const interval_time_point& , uint32_t , uint32_t);

/**
 * 时间间隔类,提供对时间间隔内任务信息的操作
 * 对任务信息排序（二叉堆）
 *  1.获取最小触发时间的任务信息
 *  2.插入任务信息
 *  3.删除最小触发时间的任务信息
 */
class TimeInterval final{
public:
    TimeInterval() = default;
    explicit TimeInterval(int start, int end, CorrelateBase *correlate, ContainDynamicFunc contain_dynamic, BucketLink *link)
                               : interval_start(static_cast<uint16_t>(start)), interval_end(static_cast<uint16_t>(end)),
                                  time_correlate(correlate), contain_dynamic_func(contain_dynamic), time_binary(link) {}
    ~TimeInterval() = default;

    TaskInfo* minTaskInfo();
    TaskInfo* onSurmount(const interval_time_point&);
    void forceReceive(TaskInfo*, const interval_time_point&);
    bool onReceive(TaskInfo*, const double&, const interval_time_point&);
    bool onSearch(TaskInfo*);
    void onClear();
    void ergodicTaskInfo(const interval_time_point&);
private:
    void onRemove(TaskInfo*);
    bool onComparator(TaskInfo*, TaskInfo*, const interval_time_point&);
    bool onContain(TaskInfo*, const double&);
    bool onContain(TaskInfo*, const interval_time_point&);
    /*
     * [start, end)
     */
    int16_t interval_start;         //间隔开始信息(闭区间)
    int16_t interval_end;           //间隔结束信息(开区间)
    CorrelateBase *time_correlate;  //时间关联器
    ContainDynamicFunc contain_dynamic_func;
    TimeBinary time_binary;
};

#endif //UNTITLED5_INTERVALMANAGER_H
