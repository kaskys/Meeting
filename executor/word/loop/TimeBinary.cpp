//
// Created by abc on 20-11-8.
//
#include "TimeBinary.h"

/**
 * 该函数说明在TimeInterval.cpp
 * @param info
 * @param new_time
 */
void TimeBinary::pushInfo(BinaryInfo *info, const interval_time_point &new_time, const comparator_func &cfunc) {
    if(!onPos(root_pos)){
        end_parent_pos = root_pos = info->getBinaryPosition();
    }else{
        onPushInfo(getBinaryInfoOnPosition(bucket_link, end_parent_pos), info);
    }

    onPercolateUp(info, new_time, cfunc);
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param parent
 * @param child
 */
void TimeBinary::onPushInfo(BinaryInfo *parent, BinaryInfo *child) {
    BinaryInfo *prev_info = nullptr;

    if(onPos(parent->left_pos)){
        parent->right_pos = child->getBinaryPosition();
        prev_info = getBinaryInfoOnPosition(bucket_link, parent->left_pos);

        end_parent_pos = parent->next_pos;
    }else{
        parent->left_pos = child->getBinaryPosition();
        prev_info = onPos(parent->prev_pos) ? getBinaryInfoOnPosition(bucket_link, getBinaryInfoOnPosition(bucket_link, parent->prev_pos)->right_pos)
                                            : parent;
    }
    child->parent_pos = parent->getBinaryPosition();

    if(prev_info){
        prev_info->next_pos = child->getBinaryPosition();
        child->parent_pos = prev_info->getBinaryPosition();
    }
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param new_time
 */
void TimeBinary::popRoot(const interval_time_point &new_time, const comparator_func &cfunc) {
    BinaryInfo *end_parent_info = getBinaryInfoOnPosition(bucket_link, end_parent_pos),
               *root_info = getBinaryInfoOnPosition(bucket_link, root_pos), *end_info = nullptr;

    if(end_parent_pos != root_pos){
        if(onPos(end_parent_info->left_pos)){
            end_info = getBinaryInfoOnPosition(bucket_link, end_parent_info->left_pos);
            end_parent_info->left_pos = -1;
        }else{
            end_info = getBinaryInfoOnPosition(bucket_link,
               (end_parent_info = getBinaryInfoOnPosition(bucket_link, end_parent_pos = end_parent_info->prev_pos))->right_pos);

            end_parent_info->right_pos = -1;
        }

        getBinaryInfoOnPosition(bucket_link, end_info->prev_pos)->next_pos = -1;
        end_info->prev_pos = -1;
        end_info->parent_pos = -1;

        if(end_parent_pos == root_pos){
            end_parent_pos = end_info->getBinaryPosition();
        }

        root_pos = end_info->getBinaryPosition();
        onSwapInfo(bucket_link, root_info, end_info);

        onPercolateDown(root_info, new_time, cfunc);
    }else{
        if(onPos(end_parent_info->left_pos)){
            root_pos = end_parent_info->left_pos;
            end_parent_pos = end_parent_info->left_pos;
//            bucket_link->erase(root_info->getInfoPosition()); 这里是否是回调
        }else{
            root_pos = -1;
        }
    }

    onReset(root_info);
}

/**
 * 该函数说明在TimeInterval.cpp
 */
void TimeBinary::clear(const remove_func &rfunc) {
    BinaryInfo *info = nullptr;

    for(int16_t pos = root_pos, next_pos = 0;;){
        if(!onPos(pos)){
            break;
        }
        info = getBinaryInfoOnPosition(bucket_link, pos);

        next_pos = info->next_pos;
        rfunc(dynamic_cast<TaskInfo*>(getBinaryInfoOnPosition(bucket_link, pos)));
        pos = next_pos;
    }
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param new_time
 * @param call_back
 */
void TimeBinary::ergodicTaskInfo(const interval_time_point &new_time, const std::function<void(TaskInfo *)> &call_back) {
    int value = 1, multiple = 1;
    BinaryInfo *info = nullptr;

    for(info = minTaskInfo(); info != nullptr; ){
        call_back(dynamic_cast<TaskInfo*>(info));
        std::cout << "(" << info->getBinaryPosition() << "," << info->bpos << "," << info->tpos << "),";

        if(onPos(info->next_pos)){
            info = getBinaryInfoOnPosition(bucket_link, info->next_pos);
        }else{
            break;
        }

        if((--value) <= 0){
            std::cout << std::endl;
            value = (multiple * 2);
            multiple *= 2;
        }
    }

    std::cout << std::endl;
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param search_info
 * @return
 */
bool TimeBinary::search(TaskInfo *search_info) {
    auto *binfo = dynamic_cast<BinaryInfo*>(search_info);

    if(!onPos(root_pos)){
        return false;
    }

    for(;;){
        if(onPos(search_info->parent_pos)){
            binfo = getBinaryInfoOnPosition(bucket_link, binfo->parent_pos);
        }else{
            return (search_info->getBinaryPosition() == root_pos);
        }
    }
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param new_time
 * @param call_back
 * @return
 */
BinaryInfo* TimeBinary::surmount(const interval_time_point &new_time, const std::function<bool(TaskInfo*)> &call_back, const comparator_func &cfunc) {
    BinaryInfo *surmount_info = nullptr;

    if(onPos(root_pos) && (surmount_info = getBinaryInfoOnPosition(bucket_link, root_pos))){
        if(call_back(dynamic_cast<TaskInfo*>(surmount_info))){
            surmount_info = nullptr;
        }else{
            popRoot(new_time, cfunc);
        }
    }

    return surmount_info;
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param info
 * @param new_time
 */
void TimeBinary::onPercolateUp(BinaryInfo *info, const interval_time_point &new_time, const comparator_func &cfunc) {
    BinaryInfo *parent_info = nullptr;

    while(info->getBinaryPosition() != root_pos){
        if(!onPos(info->parent_pos)){
            root_pos = info->getBinaryPosition();
            break;
        }

        parent_info = getBinaryInfoOnPosition(bucket_link, info->parent_pos);
        if(cfunc(dynamic_cast<TaskInfo*>(info), dynamic_cast<TaskInfo*>(parent_info), new_time)){
            onSwapInfo(bucket_link, info, parent_info);
        }else{
            break;
        }
    }

    if((root_pos != end_parent_pos) && !onPos(getBinaryInfoOnPosition(bucket_link, end_parent_pos)->next_pos)){
        end_parent_pos = getBinaryInfoOnPosition(bucket_link, end_parent_pos)->parent_pos;
    }
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param info
 * @param new_time
 */
void TimeBinary::onPercolateDown(BinaryInfo *info, const interval_time_point &new_time, const comparator_func &cfunc) {
    BinaryInfo *up_info = nullptr;

    while(onPos(info->left_pos)){
        if(onPos(info->right_pos) && cfunc(dynamic_cast<TaskInfo*>(getBinaryInfoOnPosition(bucket_link, info->right_pos)),
                                           dynamic_cast<TaskInfo*>(getBinaryInfoOnPosition(bucket_link, info->left_pos)), new_time)){
            up_info = getBinaryInfoOnPosition(bucket_link, info->right_pos);
        }else{
            up_info = getBinaryInfoOnPosition(bucket_link, info->left_pos);
        }

        if(cfunc(dynamic_cast<TaskInfo*>(info), dynamic_cast<TaskInfo*>(up_info), new_time)){
            if(info->getBinaryPosition() == root_pos){
                root_pos = up_info->getBinaryPosition();
            }

            if(up_info->getBinaryPosition() == end_parent_pos){
                end_parent_pos = info->getBinaryPosition();
            }else{
                end_parent_pos = up_info->getBinaryPosition();
            }
            onSwapInfo(bucket_link, info, up_info);
        }else{
            break;
        }
    }
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param link
 * @param swap1
 * @param swap2
 */
void TimeBinary::onSwapInfo(BucketLink *link, BinaryInfo *swap1, BinaryInfo *swap2) {
    if(swap1->parent_pos == swap2->getBinaryPosition()){
        onSwapParent(link, swap2, swap1);
    }else if(swap2->parent_pos == swap1->getBinaryPosition()){
        onSwapParent(link, swap1, swap2);
    }else{
        onSwapAncestor(link, swap1, swap2);
    }
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param link
 * @param parent_info
 * @param child_info
 */
void TimeBinary::onSwapParent(BucketLink *link, BinaryInfo *parent_info, BinaryInfo *child_info) {
    BinaryInfo swap = *child_info;

    if(onPos(child_info->parent_pos = parent_info->parent_pos)){
        if(getBinaryInfoOnPosition(link, child_info->parent_pos)->left_pos == parent_info->getBinaryPosition()){
            getBinaryInfoOnPosition(link, child_info->parent_pos)->left_pos = child_info->getBinaryPosition();
        }else{
            getBinaryInfoOnPosition(link, child_info->parent_pos)->right_pos = child_info->getBinaryPosition();
        }
    }

    if(child_info->prev_pos == parent_info->getBinaryPosition()){
        child_info->next_pos = parent_info->getBinaryPosition();
    }else{
        if(onPos(child_info->next_pos = parent_info->next_pos)){
            getBinaryInfoOnPosition(link, child_info->next_pos)->prev_pos = child_info->getBinaryPosition();
        }
    }

    if(onPos(child_info->prev_pos = parent_info->prev_pos)){
        getBinaryInfoOnPosition(link, child_info->prev_pos)->next_pos = child_info->getBinaryPosition();
    }

    if(parent_info->left_pos == child_info->getBinaryPosition()){
        child_info->left_pos = parent_info->getBinaryPosition();
        if(onPos(child_info->right_pos = parent_info->right_pos)){
            getBinaryInfoOnPosition(link, child_info->right_pos)->parent_pos = child_info->getBinaryPosition();
        }
    }else{
        child_info->right_pos = parent_info->getBinaryPosition();
        if(onPos(child_info->left_pos = parent_info->left_pos)){
            getBinaryInfoOnPosition(link, child_info->left_pos)->parent_pos = child_info->getBinaryPosition();
        }
    }

    parent_info->parent_pos = swap.getBinaryPosition();
    if(swap.prev_pos == parent_info->getBinaryPosition()){
        parent_info->prev_pos = swap.getBinaryPosition();
    }else{
        if(onPos(parent_info->prev_pos = swap.prev_pos)){
            getBinaryInfoOnPosition(link, parent_info->prev_pos)->next_pos = parent_info->getBinaryPosition();
        }
    }

    if(onPos(parent_info->next_pos = swap.next_pos)){
        getBinaryInfoOnPosition(link,parent_info->next_pos)->prev_pos = parent_info->getBinaryPosition();
    }

    if(onPos(parent_info->left_pos = swap.left_pos)){
        getBinaryInfoOnPosition(link, parent_info->left_pos)->parent_pos = parent_info->getBinaryPosition();
    }

    if(onPos(parent_info->right_pos = swap.right_pos)){
        getBinaryInfoOnPosition(link, parent_info->right_pos)->parent_pos = parent_info->getBinaryPosition();
    }
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param link
 * @param swap1
 * @param swap2
 */
void TimeBinary::onSwapAncestor(BucketLink *link, BinaryInfo *swap1, BinaryInfo *swap2) {
    BinaryInfo swap = *swap1;
    onSwapInfo0(link, swap2, swap1);
    onSwapInfo0(link, swap1, &swap);
}

/**
 * 该函数说明在TimeInterval.cpp
 * @param link
 * @param dsc
 * @param src
 */
void TimeBinary::onSwapInfo0(BucketLink *link, BinaryInfo *dsc, BinaryInfo *src) {
    if(onPos(dsc->parent_pos = src->parent_pos)){
        if(getBinaryInfoOnPosition(link, dsc->parent_pos)->left_pos == src->getBinaryPosition()){
            getBinaryInfoOnPosition(link, dsc->parent_pos)->left_pos = dsc->getBinaryPosition();
        }else{
            getBinaryInfoOnPosition(link, dsc->parent_pos)->right_pos = dsc->getBinaryPosition();
        }
    }

    if(onPos(dsc->prev_pos = src->prev_pos)){
        getBinaryInfoOnPosition(link, src->prev_pos)->next_pos = dsc->getBinaryPosition();
    }

    if(onPos(dsc->next_pos = src->next_pos)){
        getBinaryInfoOnPosition(link, src->next_pos)->prev_pos = dsc->getBinaryPosition();
    }

    if(onPos(dsc->left_pos = src->left_pos)){
        getBinaryInfoOnPosition(link, dsc->left_pos)->parent_pos = dsc->getBinaryPosition();
    }

    if(onPos(dsc->right_pos = src->right_pos)){
        getBinaryInfoOnPosition(link, dsc->right_pos)->parent_pos = dsc->getBinaryPosition();
    }
}



