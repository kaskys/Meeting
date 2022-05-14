//
// Created by abc on 20-11-8.
//

#ifndef TEXTGDB_TIMEBINARY_H
#define TEXTGDB_TIMEBINARY_H

#include "TimeLink.h"

using interval_time_point = std::chrono::steady_clock::time_point;
using remove_func = std::function<void(TaskInfo*)>;
using comparator_func = std::function<bool(TaskInfo*, TaskInfo*, const interval_time_point&)>;
struct BinaryInfo;

class TimeBinary final {
public:
    TimeBinary() : root_pos(-1), end_parent_pos(-1), bucket_link(nullptr) {}
    explicit TimeBinary(BucketLink *link) : root_pos(-1), end_parent_pos(-1), bucket_link(link) {}
    ~TimeBinary() = default;

    void pushInfo(BinaryInfo*, const interval_time_point&, const comparator_func&);
    void popRoot(const interval_time_point&, const comparator_func&);
    void clear(const remove_func&);
    void ergodicTaskInfo(const interval_time_point&, const std::function<void(TaskInfo*)>&);
    bool search(TaskInfo*);
    BinaryInfo* minTaskInfo() const {
        return (onPos(root_pos) ? getBinaryInfoOnPosition(bucket_link, root_pos) : nullptr);
    }
    BinaryInfo* surmount(const interval_time_point&, const std::function<bool(TaskInfo*)>&, const comparator_func&);
private:
    void onPushInfo(BinaryInfo*, BinaryInfo*);
    void onPercolateUp(BinaryInfo*, const interval_time_point&, const comparator_func&);
    void onPercolateDown(BinaryInfo*, const interval_time_point&, const comparator_func&);

    static bool onPos(int16_t pos) { return (pos >= 0); }
    static void onReset(BinaryInfo *reset_info) {
        reset_info->next_pos = -1; reset_info->prev_pos = -1; reset_info->parent_pos = -1;
        reset_info->right_pos = -1; reset_info->left_pos = -1;
    }
    static int getBucketPosition(int16_t binary_pos) {
        return (binary_pos / DEFAULT_TASK_ARRAY_SIZE);
    }
    static int getTaskPosition(int16_t binary_pos) {
        return (binary_pos & (DEFAULT_TASK_ARRAY_SIZE - 1));
    }
    static std::pair<int, int> getPositionPair(int16_t binary_pos) {
        return {getBucketPosition(binary_pos), getTaskPosition(binary_pos)};
    };
    static BinaryInfo* getBinaryInfoOnPosition(BucketLink *link, int16_t pos) {
        return link->getBinaryInfo(getPositionPair(pos));
    }

    static void onSwapInfo(BucketLink*, BinaryInfo*, BinaryInfo*);
    static void onSwapParent(BucketLink*, BinaryInfo*, BinaryInfo*);
    static void onSwapAncestor(BucketLink*, BinaryInfo*, BinaryInfo*);
    static void onSwapInfo0(BucketLink*, BinaryInfo*, BinaryInfo*);

    int16_t root_pos;       //二叉堆非顺序数组信息的根节点位置
    int16_t end_parent_pos; //二叉堆非顺序数组信息的下一个插入位置的父节点位置;
    BucketLink *bucket_link;
};

#endif //TEXTGDB_TIMEBINARY_H
