//
// Created by abc on 19-9-20.
//

#ifndef UNTITLED8_UNLOCKLIST_H
#define UNTITLED8_UNLOCKLIST_H

#include "../UtilTool.h"

/**
 * 无锁双向链表
 *   1.UnLockList<T>::Note的线程唯一性问题,因为线程A占据Note,且修改或删除时,另线程B同时占据Note,当读取、修改或删除,会发生未定义行为
 *      (1)该问题交给使用者来处理
 *      (2)使用std::atomic<bool>来标记Note的占据情况,且只有一个线程拥有Note
 *      (3)使用std::atomic<int>来标记Note的线程使用数,当 <= 0 时,需要删除Note,可以提供n个线程操作读取操作
 *         UnLockIterator使用std::shared_ptr来标记同一个线程内使用Note的情况
 */
struct UnLockBaseNote{
    UnLockBaseNote() : erase_flag(), root_note(nullptr), prev(this), next(this) {}
    virtual ~UnLockBaseNote() = default;

    UnLockBaseNote(const UnLockBaseNote&) = delete;
    UnLockBaseNote(UnLockBaseNote&&) = delete;

    UnLockBaseNote& operator=(const UnLockBaseNote&) = delete;
    UnLockBaseNote& operator=(UnLockBaseNote&&) = delete;

    void onFormatNote(){
        new (&erase_flag) std::once_flag();
        root_note = nullptr; prev.store(this); next.store(this);
    }

    std::once_flag erase_flag;          //删除(销毁)同步标志（多线程竞争）
    union {
        UnLockBaseNote *root_note;
        UnLockBaseNote *holder_next;
    };
    std::atomic<UnLockBaseNote*> prev;
    std::atomic<UnLockBaseNote*> next;
};

template <typename T> struct UnLockDefaultNote : public UnLockBaseNote {
    explicit UnLockDefaultNote(const T &d) : UnLockBaseNote(), data(d) {}
    explicit UnLockDefaultNote(T &&d) : UnLockBaseNote(), data(std::move(d)) {}
    ~UnLockDefaultNote() = default;

    static UnLockDefaultNote<T>* createNote(const T &data) throw(std::bad_alloc) { return new UnLockDefaultNote(data); }
    static UnLockDefaultNote<T>* createNote(T &&data) throw(std::bad_alloc) { return new UnLockDefaultNote(std::move(data)); }
    static void destroyNote(UnLockBaseNote *base_note) { delete base_note; }

    T data;
};

template <typename T, typename Note = UnLockDefaultNote<T>> class UnLockList final{
    bool operator_concurrent;
    UnLockBaseNote head;
public:
    class UnLockConcurrentException final : public std::exception {
    public:
        using exception::exception;
        ~UnLockConcurrentException() override  = default;
        const char* what() const noexcept override { return nullptr; }
    };

    class UnLockEraseException final : public std::exception {
    public:
        using exception::exception;
        ~UnLockEraseException() override  = default;
        const char* what() const noexcept override { return nullptr; }
    };

    class UnLockIterator final {
        friend class UnLockList<T, Note>;
        friend class UnLockListHolder;
    public:
        UnLockIterator() : root_list(nullptr), iterator_note(nullptr) {}
        explicit UnLockIterator(UnLockBaseNote *note) : root_list(nullptr), iterator_note(note) {}
        explicit UnLockIterator(UnLockList<T, Note> *list, UnLockBaseNote *note) : root_list(list), iterator_note(note) {}
        ~UnLockIterator() = default;

        T& operator*() { return (dynamic_cast<Note*>(iterator_note))->data; }
        T* operator->() { return operator*(); }
        const T& operator*() const { return  operator*(); }
        const T* operator->() const { return operator->(); }


        bool operator==(const UnLockIterator &dsc) { return (iterator_note == dsc.iterator_note); }
        bool operator!=(const UnLockIterator &dsc) { return !(*this == dsc); }

        UnLockIterator& operator++() throw(UnLockConcurrentException) {
            UIterator cur_iterator(iterator_note), next_iterator = UIterator();
            for(;;){
                //获取next(next还没有被删除或没有完成删除成功,iterator_number的值是有效的)
                next_iterator = UnLockIterator(cur_iterator.iterator_note->next.load(std::memory_order_acquire));

                //判断number值是否正在执行删除或完成删除
                if(root_list->operator_concurrent && ((cur_iterator.iterator_note->root_note != &root_list->head) || (cur_iterator == next_iterator))){
                    //令该iterator_note指向head,结束外循环(令该UnLockIterator指向end(), 使外面循环结束)
                    //next_iterator = &root_list->head;
                    throw UnLockConcurrentException();
                }

                //没有执行删除操作(1)或正在执行删除操作前部分,没有完成设置next(2)
                /*
                 * (1)没有执行删除：可以对该UnLockIterator进行操作
                 * (2)正在执行删除：该UnLockIterator不对同时操作和删除的线程进行同步,由调用者执行同步操作(如MemoryBuffer,对UnLockIterator进行挂载或卸载)
                 */
                iterator_note = next_iterator.iterator_note;
                break;

                //iterator_note没有删除操作(1)、正在删除操作(2)、完成删除操作(3)
                // (1)当iterator_note没有执行删除操作时,next_iterator指向iterator_note的next,第一个compare_iterator比较成功
                /*
                 * (2)当iterator_note正在执行删除操作时
                 *      1.当next_iterator指向iterator_note的next,步骤(1)
                 *      2.当next_iterator指向iterator_note的prev,判断iterator_note.prev是否指向next
                 *          是:第二个compare_iterator比较成功
                 *          否:重新循环
                 */
                // (3)当iterator_note完成执行删除操作时,iterator_note.prev指向实际的next,第二个compare_iterator比较成功
//                if(!compare_iterator(cur_iterator, next_iterator) || !compare_iterator(cur_iterator, (next_iterator = cur_iterator.iterator_note->prev.load(std::memory_order_consume)))){
//                    break;
//                }

            }
            return *this;
        }

        UnLockIterator  operator++(int) {
            UIterator tmp_iterator = *this;
            try {
                operator++();
            }catch(const UnLockConcurrentException &e){
                this = root_list->end();
            }
            return tmp_iterator;
        }

//        int getId() const { return iterator_note->id; }
    private:
        Note* onNote() { return dynamic_cast<Note*>(iterator_note); }
        std::once_flag& onOnceFlag() { return iterator_note->erase_flag; }

        UnLockList<T, Note> *root_list;
        UnLockBaseNote *iterator_note;
    };

    /**
     * 单线程持有
     */
    class UnLockListHolder final {
    public:
        UnLockListHolder() : holder_size(0), first(nullptr), last(nullptr) {}
        ~UnLockListHolder() = default;

        UnLockListHolder(const UnLockListHolder&) = default;
        UnLockListHolder(UnLockListHolder&&) = default;

        UnLockListHolder& operator=(const UnLockListHolder&) = default;
        UnLockListHolder& operator=(UnLockListHolder&&) = default;

        UnLockListHolder& operator<<(UnLockBaseNote*);
        UnLockListHolder& operator<<(UnLockIterator&);

        UnLockListHolder& operator+=(const UnLockListHolder&);
        UnLockListHolder& operator+(const UnLockListHolder&);
        void onHolderNote(const std::function<bool(UnLockIterator)>&);
        int onHolderSize() const { return holder_size; }
    private:
        static void onFormatHolder(UnLockListHolder &holder){
            holder.holder_size = 0; holder.first = nullptr; holder.last = nullptr;
        }

        int holder_size;
        UnLockBaseNote *first;
        UnLockBaseNote *last;
    };


    typedef UnLockList<T, Note>::UnLockIterator     UIterator;
    typedef UnLockList<T, Note>::UnLockListHolder   UHolder;

    UnLockList(bool concurrent = true) : operator_concurrent(concurrent), head() {}
    ~UnLockList() { clear(); }

    /**
     * 移动构造和拷贝不提供多线程服务
     * 必须有使用者使用锁或其他
     */
    UnLockList(const UnLockList&) = delete;
    UnLockList(UnLockList&&) = delete;

    UnLockList& operator=(const UnLockList&) = delete;
    UnLockList& operator=(UnLockList&&) = delete;

    bool empty() const { return (head.next.load(std::memory_order_consume) == &head); }
    void clear() { clear(this); }
//    void merge(UnLockList &dsc) {
//        clear(&dsc, [&](UIterator begin_iterator) -> void {
//                        this->push_front(begin_iterator.onNote()->data);
//                    });
//    }
    /**
     * 清空UnLockList（假设调用该函数的线程独占或唯一操作该List的线程）
     * @param dsc   目标UnLockList
     * @param cfunc 回调函数
     */
    static void clear(UnLockList<T, Note> *dsc, std::function<void(UIterator)> cfunc = nullptr){
        UnLockBaseNote *next_note = dsc->head.next.load(), *erase_note = nullptr;
        for(;;){
            if(next_note == &dsc->head){
                break;
            }

            if(cfunc){
                cfunc(UIterator(dsc, next_note));
            }

            next_note = (erase_note = next_note)->next.load();
            Note::destroyNote(dynamic_cast<UnLockDefaultNote<T>*>(erase_note));
        }
    }

    void push_front(const T &data) throw (std::bad_alloc) {
        push_front0(Note::createNote(data));
    }
    void push_front(T &&data) throw(std::bad_alloc) {
        push_front0(Note::createNote(std::move(data)));
    }

    void push_last(const T &data) throw(std::bad_alloc) {} //暂不实现
    void push_last(T &&data) throw(std::bad_alloc) {} //暂不实现
    void push(UIterator &iterator, const T &&data) throw(std::bad_alloc) {}//暂不实现
    void push(UIterator &iterator, T &&data) throw(std::bad_alloc) {}//暂不实现

    void erase(UnLockIterator &iterator) throw(UnLockEraseException) { erase0(iterator, std::bind(Note::destroyNote, std::placeholders::_1)); }

    UIterator begin() { return UIterator(this, head.next.load(std::memory_order_consume)); }
    UIterator end() { return UIterator(&head); }

    UHolder thread_holder() {
        UHolder holder_util;
        UIterator iterator;
        for(;;){
            if((iterator = begin()) == end()){ break; }
            erase0(iterator,
                   [&](UnLockDefaultNote<T> *note) -> void {
                       holder_util << note;
                   });
        }
        return holder_util;
    }
private:
    void push_front0(UnLockBaseNote *push_note){
        UnLockBaseNote *next_note = nullptr, *head_note = nullptr;

        //获取id
//        push_note->id = id_generator.fetch_add(ID_GENERATOR_INCREASE_VALUE, std::memory_order_relaxed);
        do{
            //加载head
            head_note = &head;
            //加载next
            next_note = head_note->next.load(std::memory_order_consume);
            //设置root_note
            push_note->root_note = head_note;
            //尝试令next->prev 指向 push_note
            //失败,说明其他线程已经成功删除了head(令next->prev 指向 head->prev),循环直到其他线程删除完成即可
        }while(!next_note->prev.compare_exchange_weak(head_note, push_note, std::memory_order_release, std::memory_order_relaxed));
        //令push_note->next指向新的next
        //  成功:head->next(另一个节点)
        //  失败:head->next(head自己)
        //判断是否需要重新获取id
        if((next_note != &head)){ // && (push_note->id <= next_note->id)){
//            push_note->id = id_generator.fetch_add(ID_GENERATOR_INCREASE_VALUE, std::memory_order_relaxed);
        }

        push_note->next.store(next_note, std::memory_order_release);
        head_note = &head;

        //尝试令head->next 指向 push_note
hn_cew:
        if(!head_note->next.compare_exchange_weak(next_note, push_note, std::memory_order_release, std::memory_order_acquire)){
            //更新失败,重新加载了next_note,判断next_note是否指向head_note
            //是->即其他线程调用了erase_函数,令head_note->next指向了head_note,所以跳出循环即可
            //否->假失败
            if(next_note != head_note){
                goto hn_cew;
            }
        }
        //更新成功,head_note->next指向了push_note,令push_note->prev指向head_note
        //更新失败,head_note->next指向了head_note,令push_note->prev指向head_note,由erase_函数完成即可
        push_note->prev.store(head_note, std::memory_order_release);
    }

//    void swap(UnLockList *dsc){
//        UnLockBaseNote *next_note;
//
//        auto erase_func = [&](int id_value, int erase_number){
//            note_size.fetch_add(erase_number, std::memory_order_relaxed);
////            id_generator.fetch_add(id_value, std::memory_order_relaxed);
//
//            dsc->note_size.fetch_sub(erase_number, std::memory_order_release);
//            //无法令其原子改变
////            dsc->id_generator.store(ID_GENERATOR_INIT_VALUE, std::memory_order_release);
//        };
//
//        if((next_note = erase0(dsc, erase_func)) != dsc->end().iterator_note){
//            push_(next_note);
//        }
//    }

    void push_(UnLockBaseNote *base_note){
        head.next.store(base_note, std::memory_order_relaxed);
        head.prev.store(base_note->prev, std::memory_order_relaxed);

        base_note->prev.load(std::memory_order_relaxed)->next.store(&head, std::memory_order_relaxed);
        base_note->prev.store(&head, std::memory_order_relaxed);
    }

    void erase0(UIterator &iterator, const std::function<void(UnLockDefaultNote<T>*)> &rfunc) throw(UnLockEraseException) {
        using EraseFunc = void (*)(UnLockList<T,Note>*, UnLockBaseNote*, const std::function<void(UnLockDefaultNote<T>*)>&);
        if(iterator.iterator_note == &head) { return; }
        std::call_once(iterator.onOnceFlag(), (EraseFunc)&UnLockList::erase_, this, iterator.iterator_note, rfunc);
    }

    void erase_(UnLockBaseNote *base_note, const std::function<void(UnLockDefaultNote<T>*)> &rfunc) throw(UnLockEraseException) {
        UnLockBaseNote *prev_iterator = nullptr,*next_iterator = nullptr, *cur_iterator = base_note;

        base_note->root_note = nullptr;
        do{
            next_iterator = base_note->next.load(std::memory_order_consume);
        }while(!base_note->next.compare_exchange_weak(next_iterator, base_note, std::memory_order_release, std::memory_order_relaxed));

        do{
            prev_iterator = base_note->prev.load(std::memory_order_consume);
        }while(!base_note->prev.compare_exchange_weak(prev_iterator, base_note, std::memory_order_release, std::memory_order_relaxed));


        while(!next_iterator->prev.compare_exchange_weak(cur_iterator, prev_iterator, std::memory_order_release, std::memory_order_acquire)){
            if(cur_iterator != base_note){
                do{
                    do{
                        next_iterator = base_note->next.load(std::memory_order_consume);
                    }while(next_iterator == base_note);
                }while(!base_note->next.compare_exchange_weak(next_iterator, base_note, std::memory_order_release, std::memory_order_relaxed));
            }
        }


        while(!prev_iterator->next.compare_exchange_weak(cur_iterator, next_iterator, std::memory_order_release, std::memory_order_relaxed)){
            if(cur_iterator != base_note){
                prev_iterator->next.store(next_iterator, std::memory_order_release);
                break;
            }
        }

        rfunc(dynamic_cast<UnLockDefaultNote<T>*>(base_note));
        throw UnLockEraseException();
    }


    /**
     * 多个线程删除同一个节点,需要决定其他线程push_front节点（无视其他线程删除不同节点）
     * 调用该函数后的链表只能由单个线程持有(单个线程遍历),如果想多个线程持有,需要重新设置每个UnLockBaseNote.root_note
     * @param iterator
     * @return
     */
//    UnLockBaseNote* erase0(UnLockList *dsc, std::function<void(int, int)> erase_func){
//        int id_value = 0, erase_number = 0;
//        UnLockBaseNote *next_iterator = nullptr, *root_iterator = &dsc->head;
//
//        do{
//            if((next_iterator = root_iterator->next.load(std::memory_order_consume)) == root_iterator){
//                //说明另一个线程已经更新了iterator.next,所以需要退出,该线程已经不需要处理了
//                break;
//            }
//        }while(!root_iterator->next.compare_exchange_weak(next_iterator, root_iterator, std::memory_order_release, std::memory_order_relaxed));
//
//        //说明这一个线程已经成功更新了iterator.next(同步进行,没有实现多线程处理)
//        if(next_iterator != root_iterator){
//            //加载prev
//            UnLockBaseNote *prev_iterator = root_iterator->prev.load(std::memory_order_consume);
//
//restart:
//
//            id_value = dsc->id_generator.load(std::memory_order_consume);
//            erase_number = dsc->note_size.load(std::memory_order_consume);
//
//            //尝试令next_iterator->prev 从 iterator 换成 prev_iterator
//            while(!next_iterator->prev.compare_exchange_weak(root_iterator, prev_iterator, std::memory_order_release, std::memory_order_acquire)){
//                //失败,即有其他线程push_front,next_iterator->prev指向了新插入的节点
//                //判断重新加载的root_iterator,指向root_iterator,重新设置
//                if(root_iterator == &dsc->head){
//                    continue;
//                }
//                //令next_iterator指向root_iterator(root_iterator已经重新加载并指向了next_iterator->prve)
//                next_iterator = root_iterator;
//                //重载设置root_iterator
//                root_iterator = &dsc->head;
//                goto restart;
//            }
//
//            //完成令next_iterator->prev指向prev_iterator,所以需要令prev_iterator->next指向next_iterator
//            prev_iterator->next.store(next_iterator, std::memory_order_release);
//
//            //令iterator.prev 指向 iterator,因为iterator.next已经成功更新了iterator
//            root_iterator->prev.store(root_iterator, std::memory_order_release);
//
//            if(erase_func != nullptr){
//                erase_func(id_value, erase_number);
//            }
//        }
//
//        //返回成功更新后的next_iterator 或 更新失败后的实际的iterator
//        return next_iterator;
//    }

//    static bool compare_iterator(UIterator &src, UIterator &dsc){
//#define compare_id(sid, did) (static_cast<int>(did - sid) < 0)
//
//        if((src.iterator_note->id == ID_GENERATOR_DEFAULT_VALUE) || (dsc.iterator_note->id == ID_GENERATOR_DEFAULT_VALUE)
//           || compare_id(src.iterator_note->id, dsc.iterator_note->id)){
//            return false;
//        }else{
//            return true;
//        }
//    }
};

template <typename T, typename Note> typename UnLockList<T, Note>::UnLockListHolder& UnLockList<T, Note>::UnLockListHolder::operator<<(UnLockBaseNote *note) {
    if(first){ last->holder_next = note; }
    else{ first = note; }
    last = note; holder_size++; return *this;
}
template <typename T, typename Note> typename UnLockList<T, Note>::UnLockListHolder& UnLockList<T, Note>::UnLockListHolder::operator<<(typename UnLockList<T, Note>::UnLockIterator &iterator) {
    return operator<<(iterator.iterator_note);
}

template <typename T, typename Note> typename UnLockList<T, Note>::UnLockListHolder& UnLockList<T, Note>::UnLockListHolder::operator+=(const typename UnLockList<T, Note>::UnLockListHolder &holder_){
    if(holder_.holder_size <= 0) { return *this; }
    if(first) { last->holder_next = holder_.first; }
    else { first = holder_.first; }
    last = holder_.last; holder_size += holder_.holder_size;
    return *this;
}
//没有禁止右值调用(如MemoryManager::lookupAvailableBufferOnLoad函数)
template <typename T, typename Note> typename UnLockList<T, Note>::UnLockListHolder& UnLockList<T, Note>::UnLockListHolder::operator+(const typename UnLockList<T, Note>::UnLockListHolder &holder_){
    typename UnLockList<T, Note>::UnLockListHolderUtil holder_util = this;
    holder_util += holder_;
    return holder_util;
}

template <typename T, typename Note> void UnLockList<T, Note>::UnLockListHolder::onHolderNote(const std::function<bool(typename UnLockList<T, Note>::UnLockIterator)> &func){
    for(UnLockBaseNote *cur_note = first, *prev_note = nullptr;;){
        if(!cur_note) { break; }
        cur_note->onFormatNote();

        if(func(UnLockIterator(cur_note))){
            holder_size--;
            if(prev_note){
                if(last == cur_note){
                    last = prev_note; break;
                }else{
                    prev_note->holder_next = (cur_note = cur_note->holder_next); continue;
                }
            }else{
                if(last == cur_note){
                    first = nullptr; last = nullptr; break;
                }else{
                    first = (cur_note = cur_note->holder_next); continue;
                }
            }
        }else {
            prev_note = cur_note;
            cur_note = cur_note->holder_next;
        }
    }
}

#endif //UNTITLED8_UNLOCKLIST_H
