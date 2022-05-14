//
// Created by abc on 19-6-25.
//

#ifndef UNTITLED8_UNLOCKQUEUE_H
#define UNTITLED8_UNLOCKQUEUE_H

#include "../UtilTool.h"

#define DEFAULT_ALLOCATOR_SIZE      1
#define UNLOCK_QUEUE_NORMAL_STATUS  true
#define UNLOCK_QUEUE_BLOCK_STATUS   false

enum BlockFlag{
    NORMAL_STATUS,
    BLOCK_STATUS
};

template <typename T> class UnLockQueue final{
    friend class UnLockQueueUtil;
private:
    struct Note {
        explicit Note(const T &data) : data(data), next(nullptr) {}
        explicit Note(T &&data) : data(std::move(data)), next(nullptr) {}
        ~Note() = default;

        T data;
        std::atomic<Note*> next;
    };
public:
    class UnLockQueueUtil final{
        friend class UnLockQueue<T>;
        using QueueNote = UnLockQueue::Note;
    public:
        UnLockQueueUtil() = default;
        explicit UnLockQueueUtil(QueueNote *first, QueueNote *last) : note_first(first), note_last(last) {}
        ~UnLockQueueUtil() = default;

        bool isEmpty() const { return !note_first; }

        UnLockQueueUtil& operator=(const UnLockQueueUtil &queue_){
            if(queue_.isEmpty()) { return *this; }
            note_last->next.store(queue_.note_first);
            note_last = queue_.note_last;
            return *this;
        }
    private:
        UnLockQueueUtil& operator<<(Note *note){
            if(isEmpty()){
                note_first = note;
            }else{
                note_last->next.store(note);
            }
            note_last = note;
            return *this;
        }
        std::pair<Note*, Note*> onQueue() const { return {note_first, note_last}; }

        QueueNote *note_first;
        QueueNote *note_last;
    };

    UnLockQueue() : head(nullptr), tail(nullptr), block_flag(UNLOCK_QUEUE_NORMAL_STATUS), notify_func(nullptr), wait_func(nullptr) {}
    explicit UnLockQueue(const std::function<void()> &nfunc, const std::function<void()> &wfunc)
            : head(nullptr), tail(nullptr), block_flag(UNLOCK_QUEUE_NORMAL_STATUS), notify_func(nfunc), wait_func(wfunc) {}
    explicit UnLockQueue(const T &data) : UnLockQueue() { push(data); }
    explicit UnLockQueue(T &&data) : UnLockQueue() { push(std::move(data)); }
    explicit UnLockQueue(const UnLockQueueUtil &queue_) : head(nullptr), tail(nullptr), block_flag(UNLOCK_QUEUE_NORMAL_STATUS), notify_func(nullptr), wait_func(nullptr) { merge(queue_); }
    ~UnLockQueue() = default;

    UnLockQueue(const UnLockQueue&) = delete;
    UnLockQueue(UnLockQueue<T> &&queue) = delete;

    UnLockQueue<T>& operator=(const UnLockQueue&) = delete;
    UnLockQueue<T>& operator=(UnLockQueue &&queue) = delete;

    bool empty() const { return !tail.load(std::memory_order_consume); }
    void clear() { while(!empty()) destroyNote(pop0()); }

    void merge(const UnLockQueueUtil &queue_){
        auto mqueue = queue_.onQueue();
        if(!mqueue.first || !mqueue.second) { return; }
        push0(mqueue.first, mqueue.second);
    }

    /**
     * 分割部分Note（符合cfunc函数）（假设调用函数线程是现阶段唯一操作或独占该UnLockQueue的线程）
     * @param cfunc 匹配函数
     * @param bfunc 结束函数
     * @return
     */
    UnLockQueueUtil splice(const std::function<bool(const T&)> &cfunc, const std::function<bool()> &bfunc){
        Note *splice_note = nullptr, *splice_prev = nullptr,
             *head_note = nullptr, *tail_note = nullptr;
        UnLockQueueUtil splice_queue = UnLockQueueUtil();

        if((tail_note = tail.load(std::memory_order_consume))){
            if((splice_note = head_note = head.load(std::memory_order_acquire))){
                for(;;){
                    if(bfunc() || (splice_note == tail_note)){
                        break;
                    }

                    if(cfunc(splice_note->data)){
                        splice_queue << splice_note;

                        if(splice_note == head_note){
                            splice_note = head_note = splice_note->next.load(std::memory_order_relaxed);
                            head.store(head_note, std::memory_order_relaxed);
                        }else{
                            splice_prev->next.store((splice_note = splice_note->next.load(std::memory_order_relaxed)),
                                                    std::memory_order_relaxed);
                        }
                    }else {
                        splice_note = (splice_prev = splice_note)->next.load(std::memory_order_relaxed);
                    }
                }
            }
        }
        return splice_queue;
    }

    /**
     * 转移全部Note（假设调用函数线程是现阶段唯一操作或独占该UnLockQueue的线程）
     * @return
     */
    UnLockQueueUtil transfer() {
        return UnLockQueueUtil(head.load(std::memory_order_acquire), tail.load(std::memory_order_consume));

    }

    T pop() {
        return popGeneral(pop0());
    }

    void push(const T &pdata) throw(std::bad_alloc) {
        push0(createNote(pdata));
    }
    void push(T &&pdata) throw(std::bad_alloc) {
        push0(createNote(std::move(pdata)));
    }
private:
    static Note* createNote(const T &data) throw(std::bad_alloc) { return new Note(data); }
    static Note* createNote(T &&data) throw(std::bad_alloc) { return new Note(std::move(data)); }
    static void destroyNote(Note *note){ delete(note); }

    void push0(Note *pnote){ push0(pnote, pnote); }

    T popGeneral(Note *pop_note) throw(std::runtime_error) {
        T pop_value;

        try {
            pop_value = std::move(pop_note->data);
            destroyNote(pop_note);
        }catch (...){
            push0(pop_note); throw std::runtime_error(onRuntimeErrorValue("UnLockQueue", "pop", 0, 0));
        }

        return pop_value;
    }

    T popSpecialization(Note *pop_note) {
        T value;

        memcpy(&value, &pop_note->data, sizeof(T));
        destroyNote(pop_note);

        return value;
    }

    /**
     * 多线程执行push(多线程push与pop0存在竞态条件)
     * @param phead
     * @param ptail
     */
    void push0(Note *phead, Note *ptail){
        //先加载tail,判断是否有元素
        Note *tail_note = nullptr;
reload:
        if((tail_note = tail.load(std::memory_order_acquire))){
            //有元素,只会与其他push线程和pop0线程发生竟态条件
            for(Note *next_note = nullptr; ; ){
                //push线程设置tail_note->next(push线程发生竟态条件,与pop0线程不发生竞态条件,pop0不会设置next变量,只会加载next变量)
                if(tail_note->next.compare_exchange_weak(next_note, phead, std::memory_order_release, std::memory_order_relaxed)){
                    for(;;){
                        //成功的push设置线程,与pop0线程发生竟态条件
                        //pop线程 --> 加载tail      --> 最后一个元素  --> 设置tail(nullptr)  --> 设置head(nullptr)
                        //push线程    --> 加载tail            --> 设置 tail(ptail)  --> push(tail非空)
                        if(tail.compare_exchange_weak(tail_note, ptail, std::memory_order_release, std::memory_order_relaxed)){
                            break;
                        }
                        //失败,重新加载tail(pop0设置tail = nullptr成功)
                        goto reload;
                    }
                    //与其他push线程和pop0线程的竞态成功的push线程,返回
                    break;
                }
                //失败的push设置,重新加载tail
                goto reload;
            }
        }else{
            //无元素,会与其他push线程发生竞态条件(不会与pop0线程发生竟态条件,因为pop0线程已经将tail设置为nullptr)
            if(tail.compare_exchange_weak(tail_note, ptail, std::memory_order_release, std::memory_order_relaxed)){
                //成功的push设置线程,等待pop0将head设置为nullptr(pop0函数步骤(#))
                while(!head.load(std::memory_order_consume)){
                    head.store(phead, std::memory_order_release);
                    break;
                }
            }else {
                //push线程竟态失败,重新加载tail(成功的push线程已经设置了tail)
                goto reload;
            }
        }

        //(*)判断是否需要唤醒pop0的阻塞状态
        if((notify_func != nullptr) && (block_flag.load(std::memory_order_consume) == UNLOCK_QUEUE_BLOCK_STATUS)){
            notify_func();
        }

    }

    /**
     * 单线程执行pop0(与push是多线程竞态)
     * @return
     */
    Note* pop0(){
        for(Note *pop_note = nullptr; ;){
            Note *tail_note = nullptr;
            //先加载tail,判断是否有元素
            if((tail_note = tail.load(std::memory_order_acquire))){
                //有元素,但获取不到head,轮询直到获取head为止
                if(!(pop_note = head.load(std::memory_order_consume))){
                    continue;
                }
                //已经获取到head,取消阻塞状态
                block_flag.store(UNLOCK_QUEUE_NORMAL_STATUS, std::memory_order_release);
                //判断是否最后一个元素
                if(pop_note == tail_note){
                    for(;;){
                        //pop0线程设置UnLockQueue无元素
                        if(tail.compare_exchange_weak(tail_note, nullptr, std::memory_order_release, std::memory_order_acquire)){
                            //成功设置,结束循环
                            //(#)设置head为空
                            //pop线程 --> 设置tail(nullptr)                           --> 设置head(nullptr)
                            //push线程                     -->设置tail(非nullptr)                          --> 设置head(非nullptr)
                            head.store(nullptr, std::memory_order_release);
                            break;
                        }
                        //判断是否为假失败
                        if(tail_note != pop_note){
                            //push线程成功push元素
                            //push线程 --> 设置next --> 设置tail
                            //pop线程                           --> 获取tail --> 获取next
                            //所以head不会为空(是push线程的元素)
                            head.store(pop_note->next.load(std::memory_order_consume), std::memory_order_relaxed);
                            break;
                        }
                    }
                }else{
                    //原理同上
                    //因为加载了tail并存在二个元素以上,head->next先行发生于tail的赋值
                    //所以head->next != nullptr,路径最短(head->next == tail)
                    head.store(pop_note->next.load(std::memory_order_consume), std::memory_order_relaxed);
                }
            }else{
                //轮询or阻塞
                if(wait_func != nullptr){
                    //UnLockQueue没有元素,设置阻塞状态
                    block_flag.store(UNLOCK_QUEUE_BLOCK_STATUS, std::memory_order_release);
                    wait_func();
                }
            }

            if(pop_note){
                return pop_note;
            }
        }
    }

    std::atomic<Note*> head;
    std::atomic<Note*> tail;
    std::atomic_bool block_flag;
    std::function<void()> notify_func;
    std::function<void()> wait_func;
};

template <> inline bool UnLockQueue<bool>::pop() { return popSpecialization(pop0()); }
template <> inline char UnLockQueue<char>::pop(){ return popSpecialization(pop0()); }
template <> inline signed char UnLockQueue<signed char>::pop(){ return popSpecialization(pop0()); }
template <> inline unsigned char UnLockQueue<unsigned char>::pop(){ return popSpecialization(pop0()); }
template <> inline wchar_t UnLockQueue<wchar_t>::pop(){ return popSpecialization(pop0()); }
template <> inline char16_t UnLockQueue<char16_t>::pop(){ return popSpecialization(pop0()); }
template <> inline char32_t UnLockQueue<char32_t>::pop(){ return popSpecialization(pop0()); }
template <> inline short UnLockQueue<short>::pop(){ return popSpecialization(pop0()); }
template <> inline unsigned short UnLockQueue<unsigned short>::pop(){  return popSpecialization(pop0());}
template <> inline int UnLockQueue<int>::pop(){ return popSpecialization(pop0()); }
template <> inline unsigned int UnLockQueue<unsigned int>::pop(){ return popSpecialization(pop0()); }
template <> inline long UnLockQueue<long>::pop(){ return popSpecialization(pop0()); }
template <> inline unsigned long UnLockQueue<unsigned long>::pop(){ return popSpecialization(pop0()); }
template <> inline long long UnLockQueue<long long>::pop(){ return popSpecialization(pop0()); }
template <> inline unsigned long long UnLockQueue<unsigned long long>::pop(){ return popSpecialization(pop0()); }
template <> inline float UnLockQueue<float>::pop(){ return popSpecialization(pop0()); }
template <> inline double UnLockQueue<double>::pop(){ return popSpecialization(pop0()); }
template <> inline long double UnLockQueue<long double>::pop(){ return popSpecialization(pop0()); }

#endif //UNTITLED8_UNLOCKQUEUE_H
