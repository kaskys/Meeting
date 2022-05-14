//
// Created by abc on 20-4-17.
//
#include "TimeCorrelate.h"

/**
 * 获取时间间隔内的最小触发时间的任务信息
 * @return 任务信息
 */
TaskInfo* TimeInterval::minTaskInfo() {
    return dynamic_cast<TaskInfo*>(time_binary.minTaskInfo());
}


bool TimeInterval::onComparator(TaskInfo *cinfo1, TaskInfo *cinfo2, const interval_time_point &new_time) {
    return (time_correlate->triggerTime(cinfo1, new_time) < time_correlate->triggerTime(cinfo2, new_time));
}

void TimeInterval::onRemove(TaskInfo *info) {
    time_correlate->onClear(info);
}

/**
 * 任务触发时间是否在时间间隔内
 * @param info          任务信息
 * @param contain_time  触发时间
 * @return  true:在,false:不在
 */
bool TimeInterval::onContain(TaskInfo*, const double &contain_time) {
    return time_correlate->onContainStatic(contain_time, static_cast<uint32_t>(interval_start), static_cast<uint32_t>(interval_end));
}

/**
 * 任务触发时间是否在时间间隔内
 * @param info      任务信息
 * @param new_time  当前时间点
 * @return
 */
bool TimeInterval::onContain(TaskInfo *info, const interval_time_point &new_time) {
    return contain_dynamic_func(time_correlate, info, new_time, static_cast<uint32_t>(interval_start), static_cast<uint32_t>(interval_end));
}

/**
 * 不在时间间隔内的任务信息
 * 如果有,则返回一个任务信息
 * 没有则返回nullptr,调用者需要调用多次该函数来返回任务信息,直到返回nullptr为止
 * @param new_time  当前时间点
 * @return  任务信息 or nullptr
 */
TaskInfo* TimeInterval::onSurmount(const interval_time_point &new_time) {
    return dynamic_cast<TaskInfo*>(time_binary.surmount(new_time,
                                                        [&](TaskInfo *info) -> bool{
                                                            return onContain(info, new_time);
                                                        }, std::bind((comparator_func)(&TimeInterval::onComparator), this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
}

/**
 * 时间间隔强行接收任务信息
 * @param info      任务信息
 * @param new_time  当前时间点
 */
void TimeInterval::forceReceive(TaskInfo *info, const interval_time_point &new_time) {
    time_binary.pushInfo(info, new_time, std::bind((comparator_func)(&TimeInterval::onComparator), this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

/**
 * 时间间隔接收任务信息
 *  接收：任务触发时间在时间间隔内
 *  拒绝：不在时间间隔内
 * @param info      任务信息
 * @param new_time  当时时间点
 * @return true：接收,false：拒绝
 */
bool TimeInterval::onReceive(TaskInfo *info, const double &contain_time, const interval_time_point &new_time) {
    if(onContain(info, contain_time)){
        time_binary.pushInfo(info, new_time, std::bind((comparator_func)(&TimeInterval::onComparator), this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        return true;
    }else{
        return false;
    }
}

/**
 * 时间间隔接收任务信息的实现函数
 * @param info      任务信息
 * @param new_time  当前时间点
 */
//void TimeInterval::onReceive0(SectionInfo *info, const interval_time_point &new_time) {
//    //时间间隔里是否有任务信息
//    if(!onPos(root_pos)){
//        //没有任务信息,设置根节点和尾节点信息
//        end_parent_pos = root_pos = info->current_pos;
//    }else{
//        //设置info的信息、end_parent_pos的信息
//        onPushChild(time_correlate->getSectionInfo(end_parent_pos), info);
//    }
//
//    //二叉堆上滤(二叉堆的非顺序数组的上滤)
//    onPercolateUp(info, new_time);
//}

/**
 * 搜索任务信息函数
 * @param search_info
 * @return
 */
bool TimeInterval::onSearch(TaskInfo *info) {
    return time_binary.search(info);
}

/**
 * 清空时间间隔内的任务信息
 */
void TimeInterval::onClear() {
    time_binary.clear(std::bind((remove_func)(&TimeInterval::onRemove), this, std::placeholders::_1));
}

/**
 * 遍历时间间隔内的任务信息
 * @param new_time  当前时间点
 */
void TimeInterval::ergodicTaskInfo(const interval_time_point &new_time) {
    time_binary.ergodicTaskInfo(new_time,
                                [&](TaskInfo *info) -> void{
                                    std::cout << time_correlate->getTaskTime(info)->triggerTime(new_time) << "-" << time_correlate->getTaskTime(info).get();
                                });
}