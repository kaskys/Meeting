//
// Created by abc on 20-11-3.
//

#ifndef TEXTGDB_TIMELINK_H
#define TEXTGDB_TIMELINK_H

#include "LinkBase.h"
#include "../../task/LoopWordTask.h"

class BucketLink;

//--------------------------------------------------------------------------------------------------------------------//

struct BinaryInfo : public LinkInfo {
    friend class TimeBinary;
    BinaryInfo() : LinkInfo(), bpos(-1), tpos(-1), parent_pos(-1), prev_pos(-1), next_pos(-1), left_pos(-1), right_pos(-1){}
    ~BinaryInfo() override = default;

    BinaryInfo(const BinaryInfo &info) noexcept = default;
    BinaryInfo(BinaryInfo &&info) noexcept : LinkInfo(std::move(info)), bpos(info.bpos), tpos(info.tpos), parent_pos(info.parent_pos),
                                    prev_pos(info.prev_pos), next_pos(info.next_pos), left_pos(info.left_pos), right_pos(info.right_pos){
        resetPosition(&info);
    }
    BinaryInfo& operator=(const BinaryInfo &info) noexcept = default;
    BinaryInfo& operator=(BinaryInfo &&info) noexcept {
        LinkInfo::operator=(std::move(info));
        this->bpos = info.bpos; this->tpos = info.tpos; this->parent_pos = info.parent_pos;
        this->prev_pos = info.prev_pos; this->next_pos = info.next_pos;
        this->left_pos = info.left_pos; this->right_pos = info.right_pos;
        resetPosition(&info);
        return *this;
    }

    int16_t getBinaryPosition() const{
        return static_cast<int16_t>((bpos * DEFAULT_TASK_ARRAY_SIZE) + tpos);
    }
    void setInfoPosition(int bpos, int tpos) {
        this->bpos = bpos; this->tpos = tpos;
    }
    void setInfoPosition(BinaryInfo *info) {
        this->bpos = info->bpos; this->tpos = info->tpos;
    }
    std::pair<int, int> getInfoPosition() const{
        return {bpos, tpos};
    };
private:
    static void resetPosition(BinaryInfo *info){
        info->bpos = -1; info->tpos = -1; info->parent_pos = -1;
        info->prev_pos = -1; info->next_pos = -1; info->left_pos = -1; info->right_pos = -1;
    }

    int bpos;               //桶数组位置
    int tpos;               //任务数组位置

    int16_t parent_pos;     //二叉堆父类的数组实际位置
    int16_t prev_pos;       //二叉堆的前一个节点的数组实际位置
    int16_t next_pos;       //二叉堆的下一个节点的数组实际位置
    int16_t left_pos;       //二叉堆的左子节点的的数组实际位置
    int16_t right_pos;      //二叉堆的右子节点的的数组实际位置
};

struct TaskInfo : public BinaryInfo {
    TaskInfo() = default;
    explicit TaskInfo(std::shared_ptr<LoopExecutorTask> &&task) : BinaryInfo(), interval_pos(0), interval_type(0),
                                                                                           time_task(std::move(task)){ }
    ~TaskInfo() override = default;

    TaskInfo(const TaskInfo &info) noexcept = default;
    TaskInfo(TaskInfo &&info) noexcept = default;
    TaskInfo& operator=(const TaskInfo &info) noexcept = default;
    TaskInfo& operator=(TaskInfo &&info) noexcept = default;

    uint16_t interval_pos;
    uint16_t interval_type;
    std::shared_ptr<LoopExecutorTask> time_task;
};

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 特化版本<TaskInfo>
 */
template <> class LinkBase<TaskInfo> {
public:
    LinkBase() throw(std::bad_alloc) : array_size(0), next_use_pos(0), recovery_root(nullptr), link_array(nullptr) {
        initArrayNotCopy(0);
    }
    explicit LinkBase(int size) throw(std::bad_alloc) : array_size(0), next_use_pos(0), recovery_root(nullptr), link_array(nullptr) {
        initArrayNotCopy(size);
    }
    virtual ~LinkBase() {
        if(link_array){ free(link_array); }
    }

    LinkBase(const LinkBase &link) throw(std::bad_alloc) : array_size(0), next_use_pos(0), recovery_root(nullptr), link_array(nullptr){
        copyLinkBase(link);
    }
    LinkBase(LinkBase &&link) noexcept : array_size(link.array_size), next_use_pos(link.next_use_pos),
                                         recovery_root(link.recovery_root), link_array(link.link_array){
        resetLinkBase(link);
    }

    LinkBase& operator=(const LinkBase &link) throw(std::bad_alloc) {
        copyLinkBase(link); return *this;
    }
    LinkBase& operator=(LinkBase &&link) noexcept {
        array_size = link.array_size; next_use_pos = link.next_use_pos;
        recovery_root = link.recovery_root; link_array = link.link_array;
        resetLinkBase(link); return *this;
    }

    TaskInfo& operator[](int pos){
        return link_array[pos];
    }

    const TaskInfo& operator[](int pos) const{
        return link_array[pos];
    }

    virtual int push() = 0;
    virtual void erase(int) = 0;
protected:
    virtual void onConstruction(TaskInfo*, int) = 0;
    virtual void onInit(TaskInfo*, TaskInfo*) = 0;
    virtual bool onExpand() = 0;
    virtual TaskInfo* onUsable(TaskInfo*, int) = 0;
    virtual TaskInfo* onNext(TaskInfo*, int) = 0;
    virtual TaskInfo* onLink(TaskInfo*, int) = 0;

    bool onPositionOver(int pos) const {
        return ((pos >= 0) && (pos < next_use_pos));
    }

    int arraySize() const { return array_size; }

    int insert(TaskInfo &&data) throw(std::bad_alloc) {
        return ((next_use_pos >= array_size)
                && !recovery_root
                && (!onExpand()
                    || !initArray(next_size(array_size))))
               ? throw std::bad_alloc()
               : insert0(std::move(data));
    }

    void remove(int pos){ recovery_root = onLink(recovery_root, pos); }
private:
    static void resetLinkBase(LinkBase &link) {
        link.array_size = 0; link.next_use_pos = 0;
        link.recovery_root = nullptr; link.link_array = nullptr;
    }

    void copyLinkBase(const LinkBase &link) throw(std::bad_alloc) {
        if(array_size < link.array_size){
            initArrayNotCopy(link.array_size);
        }
        std::uninitialized_copy(link.link_array, link.link_array + (next_use_pos = link.next_use_pos), link_array);
    }

    bool initArray(int init_size) {
        int old_size = array_size;
        TaskInfo *new_array = initArray0(init_size);

        if(new_array){
            int pos = 0;
            if(link_array){
                std::uninitialized_copy(link_array, link_array + old_size, new_array);
                free(link_array);
                pos = old_size;
            }
            for(; pos < init_size; pos++){
                onConstruction(new_array + pos, pos);
            }

            link_array = new_array;
            array_size = init_size;
        }

        return static_cast<bool>(new_array);
    }

    void initArrayNotCopy(int init_size) throw(std::bad_alloc) {
        auto new_array = initArray0(init_size);
        if(!new_array){ throw std::bad_alloc(); }
        if(link_array){ free(link_array); }

        for(int pos = 0; pos < init_size; pos++){
            onConstruction(new_array + pos, pos);
        }
        link_array = new_array;
        array_size = init_size;
    }

    TaskInfo* initArray0(int init_size) {
        if(init_size < DEFAULT_ARRAY_INIT_SIZE){
            init_size = DEFAULT_ARRAY_INIT_SIZE;
        }

        if(array_size >= init_size) {
            return nullptr;
        }

        return reinterpret_cast<TaskInfo*>(malloc(sizeof(TaskInfo) * init_size));
    }

    int insert0(TaskInfo &&data){
        TaskInfo *insert_ = nullptr;

        if((insert_ = onUsable(recovery_root, 0))){
            recovery_root = onNext(insert_, 0);
        }else{
            insert_ = &link_array[next_use_pos++];
        }
        onInit(insert_, &data);
        new (reinterpret_cast<void*>(insert_)) TaskInfo(std::move(data));

        return static_cast<int>(link_array - insert_);
    }

private:
    int array_size;
    int next_use_pos;
    TaskInfo *recovery_root;
    TaskInfo *link_array;
};

//--------------------------------------------------------------------------------------------------------------------//

class TaskLink : public LinkBase<TaskInfo> {
    friend class BucketLink;
public:
    explicit TaskLink(BucketLink *blink) : LinkBase(DEFAULT_TASK_ARRAY_SIZE), bpos(-1), bucket_link(blink){}
    ~TaskLink() override = default;

    TaskLink(const TaskLink&);
    TaskLink(TaskLink &&) noexcept;
    TaskLink& operator=(const TaskLink&);
    TaskLink& operator=(TaskLink&&) noexcept;

    int push() override { return -1; }
    void erase(int) override;
protected:
    void onConstruction(TaskInfo *info, int pos) override { info->setInfoPosition(bpos, pos); }
    void onInit(TaskInfo *src, TaskInfo *dsc) override { dsc->setInfoPosition(src); }
    bool onExpand() override { return false;}
    TaskInfo* onUsable(TaskInfo *info, int) override { return info; }
    TaskInfo* onNext(TaskInfo *info, int) override { return dynamic_cast<TaskInfo*>(info->next); }
    TaskInfo* onLink(TaskInfo *info, int pos) override;
private:
    int pushTask(std::shared_ptr<LoopExecutorTask>&&) throw(std::overflow_error);
    void setBucketPos(int pos) { bpos = pos; }
    int getBucketPos() const { return bpos; }

    int bpos;                //桶数组位置
    BucketLink *bucket_link;
};

//--------------------------------------------------------------------------------------------------------------------//

struct BucketInfo : public LinkInfo {
    BucketInfo() = default;
    explicit BucketInfo(BucketLink *link) : LinkInfo(), task_size(0), task_link(link) {}
    ~BucketInfo() override = default;

    BucketInfo(const BucketInfo&) = default;
    BucketInfo(BucketInfo&&) noexcept = default;
    BucketInfo& operator=(const BucketInfo&) noexcept = default;
    BucketInfo& operator=(BucketInfo&&) noexcept = default;

    int task_size;      //存储任务数量
    TaskLink task_link;
};

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 特化版本<BucketInfo>
 */
template <> class LinkBase<BucketInfo> {
public:
    LinkBase() throw(std::bad_alloc) : array_size(0), next_use_pos(0), recovery_root(nullptr), link_array(nullptr) {
        initArrayNotCopy(0);
    }
    explicit LinkBase(int size) throw(std::bad_alloc) : array_size(0), next_use_pos(0), recovery_root(nullptr), link_array(nullptr) {
        initArrayNotCopy(size);
    }
    virtual ~LinkBase() {
        if(link_array){ free(link_array); }
    }

    LinkBase(const LinkBase &link) throw(std::bad_alloc) : array_size(0), next_use_pos(0), recovery_root(nullptr), link_array(nullptr){
        copyLinkBase(link);
    }
    LinkBase(LinkBase &&link) noexcept : array_size(link.array_size), next_use_pos(link.next_use_pos),
                                         recovery_root(link.recovery_root), link_array(link.link_array){
        resetLinkBase(link);
    }

    LinkBase& operator=(const LinkBase &link) throw(std::bad_alloc) {
        copyLinkBase(link); return *this;
    }
    LinkBase& operator=(LinkBase &&link) noexcept {
        array_size = link.array_size; next_use_pos = link.next_use_pos;
        recovery_root = link.recovery_root; link_array = link.link_array;
        resetLinkBase(link); return *this;
    }

    BucketInfo& operator[](int pos){
        return link_array[pos];
    }

    const BucketInfo& operator[](int pos) const{
        return link_array[pos];
    }

    virtual int push() = 0;
    virtual void erase(int) = 0;
protected:
    virtual void onConstruction(BucketInfo*, int) = 0;
    virtual void onInit(BucketInfo*, BucketInfo*) = 0;
    virtual bool onExpand() = 0;
    virtual BucketInfo* onUsable(BucketInfo*, int) = 0;
    virtual BucketInfo* onNext(BucketInfo*, int) = 0;
    virtual BucketInfo* onLink(BucketInfo*, int) = 0;

    bool onPositionOver(int pos) const {
        return ((pos >= 0) && (pos < next_use_pos));
    }

    int arraySize() const { return array_size; }

    int insert(BucketInfo &&data) throw(std::bad_alloc) {
        return ((next_use_pos >= array_size)
                && !recovery_root
                && (!onExpand()
                    || !initArray(next_size(array_size))))
               ? throw std::bad_alloc()
               : insert0(std::move(data));
    }

    void remove(int pos){ recovery_root = onLink(recovery_root, pos); }
private:
    static void resetLinkBase(LinkBase &link) {
        link.array_size = 0; link.next_use_pos = 0;
        link.recovery_root = nullptr; link.link_array = nullptr;
    }

    void copyLinkBase(const LinkBase &link) throw(std::bad_alloc) {
        if(array_size < link.array_size){
            initArrayNotCopy(link.array_size);
        }
        std::uninitialized_copy(link.link_array, link.link_array + (next_use_pos = link.next_use_pos), link_array);
    }

    bool initArray(int init_size) {
        int old_size = array_size;
        BucketInfo *new_array = initArray0(init_size);

        if(new_array){
            int pos = 0;
            if(link_array){
                std::uninitialized_copy(link_array, link_array + old_size, new_array);
                free(link_array);
                pos = old_size;
            }
            for(; pos < init_size; pos++){
                onConstruction(new_array + pos, pos);
            }

            link_array = new_array;
            array_size = init_size;
        }

        return static_cast<bool>(new_array);
    }

    void initArrayNotCopy(int init_size) throw(std::bad_alloc) {
        auto new_array = initArray0(init_size);
        if(!new_array){ throw std::bad_alloc(); }
        if(link_array){ free(link_array); }

        for(int pos = 0; pos < init_size; pos++){
            onConstruction(new_array + pos, pos);
        }
        link_array = new_array;
        array_size = init_size;
    }

    BucketInfo* initArray0(int init_size) {
        if(init_size < DEFAULT_ARRAY_INIT_SIZE){
            init_size = DEFAULT_ARRAY_INIT_SIZE;
        }

        if(array_size >= init_size) {
            return nullptr;
        }

        return reinterpret_cast<BucketInfo*>(malloc(sizeof(BucketInfo) * init_size));
    }

    int insert0(BucketInfo &&data){
        BucketInfo *insert_ = nullptr;

        if((insert_ = onUsable(recovery_root, 0))){
            recovery_root = onNext(insert_, 0);
        }else{
            insert_ = &link_array[next_use_pos++];
        }
        onInit(insert_, &data);
        new (reinterpret_cast<void*>(insert_)) BucketInfo(std::move(data));

        return static_cast<int>(link_array - insert_);
    }

private:
    int array_size;
    int next_use_pos;
    BucketInfo *recovery_root;
    BucketInfo *link_array;
};

//--------------------------------------------------------------------------------------------------------------------//


class BucketLink : public LinkBase<BucketInfo> {
    friend class TaskLink;
public:
    BucketLink() throw(std::bad_alloc);
    ~BucketLink() override = default;

    BucketLink(const BucketLink&);
    BucketLink(BucketLink&&) noexcept;

    BucketLink& operator=(const BucketLink&);
    BucketLink& operator=(BucketLink&&) noexcept;

    std::shared_ptr<LoopExecutorTask> get(int ,int );
    std::shared_ptr<LoopExecutorTask> get(std::pair<int, int>);
    BinaryInfo* getBinaryInfo(int ,int);
    BinaryInfo* getBinaryInfo(std::pair<int, int>);

    std::pair<int, int> push(std::shared_ptr<LoopExecutorTask>&&) throw(std::bad_alloc);
    void erase(std::pair<int, int>);

    int push() override;
    void erase(int) override { /* 不需要处理 */ }
protected:
    void onConstruction(BucketInfo *info, int pos) override { info->task_link.setBucketPos(pos); }
    void onInit(BucketInfo *src, BucketInfo *dsc) override { dsc->task_link.setBucketPos(src->task_link.getBucketPos()); }
    bool onExpand() override { return true; }
    BucketInfo* onUsable(BucketInfo*, int) override;
    BucketInfo* onNext(BucketInfo *info, int) override { return dynamic_cast<BucketInfo*>(info->next); }
    BucketInfo* onLink(BucketInfo*, int) override;
private:
    void onTaskLoad(int);
    void onTaskIdle(int);

    int current_bucket_pos;     //当前能存储任务的桶数组位置
};


#endif //TEXTGDB_TIMELINK_H
