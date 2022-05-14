//
// Created by abc on 20-11-3.
//

#ifndef TEXTGDB_LINKBASE_H
#define TEXTGDB_LINKBASE_H

#include <malloc.h>
#include <stdint.h>
#include <stdexcept>
#include <typeinfo>
#include <cstring>
#include <memory>

#define DEFAULT_ARRAY_INIT_SIZE     2
#define DEFAULT_TASK_ARRAY_SIZE     8
#define next_size(size)             ((size >= DEFAULT_TASK_ARRAY_SIZE) ? (size += DEFAULT_TASK_ARRAY_SIZE) : (size * 2))

struct LinkInfo {
    LinkInfo() = default;
    virtual ~LinkInfo() = default;
    LinkInfo(const LinkInfo&) = default;
    LinkInfo(LinkInfo&&) = default;
    LinkInfo& operator=(const LinkInfo&) = default;
    LinkInfo& operator=(LinkInfo&&) = default;

    LinkInfo *next;
};

/**
 * 通用版本
 * @tparam T
 */
template <typename T> class LinkBase {
public:
    LinkBase() throw(std::bad_alloc): array_size(0), recovery_root(-1), next_use_pos(0), recovery_array(nullptr), link_array(nullptr) {
        initArrayNotInit(DEFAULT_ARRAY_INIT_SIZE);
    }
    explicit LinkBase(int size) throw(std::bad_alloc) : array_size(0), recovery_root(-1), next_use_pos(0), recovery_array(nullptr), link_array(nullptr) {
        initArrayNotInit(0);
    }
    virtual ~LinkBase() {
        if(link_array){ free(link_array); }
    }

    LinkBase(const LinkBase &link) throw(std::bad_alloc): array_size(0), recovery_root(-1), next_use_pos(0), recovery_array(nullptr), link_array(nullptr){
        copyLinkBase(link);
    }
    LinkBase(LinkBase &&link) noexcept : array_size(link.array_size), recovery_root(link.recovery_root), next_use_pos(link.next_use_pos),
                                         recovery_array(link.recovery_array), link_array(link.link_array){
        resetLinkBase(link);
    }

    LinkBase& operator=(const LinkBase &link) throw(std::bad_alloc){
        copyLinkBase(link); return *this;
    }
    LinkBase& operator=(LinkBase &&link) noexcept {
        array_size = link.array_size; recovery_root = link.recovery_root; next_use_pos = link.next_use_pos;
        recovery_array = link.recovery_array; link_array = link.link_array;
        resetLinkBase(link); return *this;
    }

    T& operator[](int pos){
        return link_array[pos];
    }

    const T& operator[](int pos) const{
        return link_array[pos];
    }

    virtual int push() = 0;
    virtual void erase(int) = 0;
protected:
    virtual void onConstruction(T*, int) {} //不需要实现
    virtual void onInit(T*, T*) {} //不需要实现
    virtual bool onExpand() { return true; }
    virtual T* onUsable(T*, int) {
        return (recovery_root < 0 ? nullptr : link_array + recovery_root);
    }
    virtual T* onNext(T*, int next_pos) {
        recovery_root = recovery_array[next_pos]; recovery_array[next_pos] = -1;
        return link_array + recovery_root;
    }
    virtual T* onLink(T*, int link_pos) {
        recovery_array[link_pos] = recovery_root; return link_array + link_pos;
    }

    bool onPositionOver(int pos) const {
        return ((pos >= 0) && (pos < next_use_pos));
    }

    int arraySize() const { return array_size; }

    int insert(T &&data) throw(std::bad_alloc) {
        return ((next_use_pos >= array_size)
                && (recovery_root < 0)
                && (!onExpand() ||
                !initArray(next_size(array_size))))
                ? throw std::bad_alloc()
                : insert0(std::move(data));
    }

    void remove(int pos){ recovery_root = onLink(recovery_array, pos); }
private:
    static void resetLinkBase(LinkBase &link) {
        link.array_size = 0; link.recovery_root = -1; link.next_use_pos = 0;
        link.recovery_array = nullptr; link.link_array = nullptr;
    }

    void copyLinkBase(const LinkBase &link) {
        if(array_size < link.array_size){
            initArrayNotInit(link.array_size);
        }
        recovery_root = link.recovery_root; next_use_pos = link.next_use_pos;
        std::uninitialized_copy(link_array, link_array + array_size, link.link_array);
        std::uninitialized_copy(recovery_array, recovery_array + array_size, link.recovery_array);
    }

    bool initArray(int init_size) {
        int old_size = array_size;
        auto new_array = initArray0(init_size);

        if(new_array.second){
            if(link_array){
                for(int i = 0; i < old_size; i++){
                    if(recovery_array[i] < 0){
                        new (reinterpret_cast<void*>(new_array.second + i)) T();
                    }else{
                        if(std::is_nothrow_move_constructible<T>::value){
                            new (reinterpret_cast<void*>(new_array.second + i)) T(std::move(link_array[i]));
                        }else{
                            new (reinterpret_cast<void*>(new_array.second + i)) T(link_array[i]);
                        }
                    }
                }
                free(link_array);
                std::uninitialized_fill(new_array.second + old_size, new_array.second + init_size, T());
            } else {
                std::uninitialized_fill(new_array.second, new_array.second + init_size, T());
            }

            recovery_array = new_array.first;
            link_array = new_array.second;
            array_size = init_size;
            return true;
        }else{
            return false;
        }
    }

    bool initArrayGeneral(int init_size) {
        int old_size = array_size;
        auto new_array = initArray0(init_size);

        if(new_array.second){
            if(link_array){
                for(int i = 0; i < old_size; i++){
                    if(recovery_array[i] < 0){
                        new_array.second[i] = 0;
                    }else{
                        memcpy(new_array.second + i, link_array + i, sizeof(T));
                    }
                }
                free(link_array);
                std::uninitialized_fill(new_array.second + old_size, new_array.second + init_size, 0);
            } else {
                std::uninitialized_fill(new_array.second, new_array.second + init_size, 0);
            }

            recovery_array = new_array.first;
            link_array = new_array.second;
            array_size = init_size;
            return true;
        }else{
            return false;
        }
    }

    void initArrayNotInit(int init_size) throw(std::bad_alloc) {
        auto new_array = initArray0(init_size);
        if(!new_array.second){ throw std::bad_alloc(); }
        if(link_array){ free(link_array); }

        recovery_array = new_array.first;
        link_array = new_array.second;
        array_size = init_size;
    }

    std::pair<int*, T*> initArray0(int init_size) {
        int create_size = 0;
        int *new_recovery = nullptr;
        T *new_array = nullptr;

        if(init_size < DEFAULT_ARRAY_INIT_SIZE){
            init_size = DEFAULT_ARRAY_INIT_SIZE;
        }

        if(array_size >= init_size){
            return {nullptr, nullptr};
        }
        create_size = (sizeof(T) + sizeof(int) * init_size);

        if(!(new_array = reinterpret_cast<T*>(malloc(create_size)))){
            return {nullptr, nullptr};
        }else{
            new_recovery = reinterpret_cast<int*>(new_array + init_size);
            std::uninitialized_fill(new_recovery, new_recovery + init_size, -1);
            return {new_recovery, new_array};
        }
    }

    int insert0(T &&data){
        T *insert_ = nullptr;

        if(insert_ = onUsable(recovery_array, recovery_root)){
            recovery_root = onNext(insert_, recovery_root);
        }else{
            insert_ = &link_array[next_use_pos++];
        }

        onInit(insert_, &data);
        new (reinterpret_cast<void*>(insert_)) T(std::move(data));

        return (link_array - insert_);
    }

    int array_size;
    int recovery_root;
    int next_use_pos;
    int *recovery_array;
    T *link_array;
};

template <> inline bool LinkBase<bool>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<char>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<signed char>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<unsigned char>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<wchar_t>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<char16_t>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<char32_t>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<short>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<unsigned short>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<int>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<unsigned int>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<long>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<unsigned long>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<long long>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<unsigned long long >::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<float>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<double>::initArray(int init_size){ return initArrayGeneral(init_size); };
template <> inline bool LinkBase<long double>::initArray(int init_size){ return initArrayGeneral(init_size); };

#endif //TEXTGDB_LINKBASE_H
