//
// Created by abc on 20-11-3.
//
#include "TimeLink.h"

TaskLink::TaskLink(const TaskLink &link) : LinkBase(link), bpos(link.bpos), bucket_link(link.bucket_link) {}

TaskLink::TaskLink(TaskLink &&link) noexcept : LinkBase(std::move(link)), bpos(link.bpos), bucket_link(link.bucket_link){
    link.bpos = -1; link.bucket_link = nullptr;
}

TaskLink& TaskLink::operator=(const TaskLink &link) {
    LinkBase<TaskInfo>::operator=(link);
    bpos = link.bpos; bucket_link = link.bucket_link;
    return *this;
}

TaskLink& TaskLink::operator=(TaskLink &&link) noexcept {
    LinkBase<TaskInfo>::operator=(std::move(link));
    bpos = link.bpos; bucket_link = link.bucket_link;
    link.bpos = -1; link.bucket_link = nullptr;
    return *this;
}

void TaskLink::erase(int pos) {
    LinkBase<TaskInfo>::operator[](pos).time_task.reset();
    remove(pos);

    bucket_link->onTaskIdle(bpos);
}

TaskInfo* TaskLink::onLink(TaskInfo *info, int pos) {
    TaskInfo *link_info = &LinkBase<TaskInfo>::operator[](pos);
    link_info->next = info;
    return link_info;
}

int TaskLink::pushTask(std::shared_ptr<LoopExecutorTask> &&time_task) throw(std::overflow_error){
    int tpos = 0;

    try {
        tpos = insert(TaskInfo(std::move(time_task)));
    }catch (std::bad_alloc &e){
        throw std::overflow_error("TaskLink->push:数组越界！");
    }

    bucket_link->onTaskLoad(bpos);

    return tpos;
}

//--------------------------------------------------------------------------------------------------------------------//

BucketLink::BucketLink() throw(std::bad_alloc) : LinkBase() {
    current_bucket_pos = BucketLink::push();
}

BucketLink::BucketLink(const BucketLink &link) : LinkBase(link), current_bucket_pos(link.current_bucket_pos)  {}

BucketLink::BucketLink(BucketLink &&link) noexcept : LinkBase(std::move(link)), current_bucket_pos(link.current_bucket_pos){
    link.current_bucket_pos = 0;
}

BucketLink& BucketLink::operator=(const BucketLink &link) {
    LinkBase<BucketInfo>::operator=(link);
    current_bucket_pos = link.current_bucket_pos;
    return *this;
}

BucketLink& BucketLink::operator=(BucketLink &&link) noexcept {
    LinkBase<BucketInfo>::operator=(std::move(link));
    current_bucket_pos = link.current_bucket_pos; link.current_bucket_pos = 0;
    return *this;
}

std::shared_ptr<LoopExecutorTask> BucketLink::get(int bpos, int tpos) {
    if(onPositionOver(bpos) && LinkBase<BucketInfo>::operator[](bpos).task_link.onPositionOver(tpos)) {
        return LinkBase<BucketInfo>::operator[](bpos).task_link[tpos].time_task;
    }else{
        return std::shared_ptr<LoopExecutorTask>(nullptr);
    }
}

std::shared_ptr<LoopExecutorTask> BucketLink::get(std::pair<int, int> pos) {
    if(onPositionOver(pos.first) && LinkBase<BucketInfo>::operator[](pos.first).task_link.onPositionOver(pos.second)) {
        return LinkBase<BucketInfo>::operator[](pos.first).task_link[pos.second].time_task;
    }else{
        return std::shared_ptr<LoopExecutorTask>(nullptr);
    }
}

BinaryInfo* BucketLink::getBinaryInfo(int bpos, int tpos) {
    if(onPositionOver(bpos) && LinkBase<BucketInfo>::operator[](bpos).task_link.onPositionOver(tpos)) {
        return static_cast<BinaryInfo*>(&LinkBase<BucketInfo>::operator[](bpos).task_link[tpos]);
    }else{
        return nullptr;
    }
}

BinaryInfo* BucketLink::getBinaryInfo(std::pair<int, int> pos) {
    if(onPositionOver(pos.first) && LinkBase<BucketInfo>::operator[](pos.first).task_link.onPositionOver(pos.second)) {
        return static_cast<BinaryInfo*>(&LinkBase<BucketInfo>::operator[](pos.first).task_link[pos.second]);
    }else{
        return nullptr;
    }
}

int BucketLink::push() {
    return insert(BucketInfo(this));
}

std::pair<int, int> BucketLink::push(std::shared_ptr<LoopExecutorTask> &&time_task) throw(std::bad_alloc){
    int tpos = 0;

    for(;;){
        try {
            tpos = LinkBase<BucketInfo>::operator[](current_bucket_pos).task_link.pushTask(std::move(time_task));
            break;
        }catch (std::bad_alloc &e){
            try {
                current_bucket_pos = BucketLink::push();
            }catch (std::bad_alloc &e){
                throw;
            }
        }

    }

    return {current_bucket_pos, tpos};
}

void BucketLink::erase(std::pair<int, int> pos) {
    if(onPositionOver(pos.first) && LinkBase<BucketInfo>::operator[](pos.first).task_link.onPositionOver(pos.second)){
        LinkBase<BucketInfo>::operator[](pos.first).task_link.erase(pos.second);
    }
}


BucketInfo* BucketLink::onUsable(BucketInfo *info, int) {
    BucketInfo *binfo = nullptr;

    while((binfo = info)){
        if(binfo->task_size >= binfo->task_link.arraySize()){
            info = dynamic_cast<BucketInfo*>(info->next);
        }else{
            break;
        }
    }

    return info;
}

BucketInfo* BucketLink::onLink(BucketInfo *info, int pos) {
    BucketInfo *link_info = &LinkBase<BucketInfo>::operator[](pos);
    link_info->next = info;
    return link_info;
}

void BucketLink::onTaskIdle(int pos) {
    BucketInfo &info = LinkBase::operator[](pos);

    if((info.task_link.getBucketPos() != current_bucket_pos) && (info.task_size--) >= info.task_link.arraySize()){
        remove(pos);
    }
}

void BucketLink::onTaskLoad(int pos) {
    BucketInfo &info = LinkBase::operator[](pos);

    if((++info.task_size) > info.task_link.arraySize()){
        info.task_size = info.task_link.arraySize();
    }
}
