//
// Created by abc on 19-6-29.
//

#ifndef UNTITLED8_BINARYHEAP_H
#define UNTITLED8_BINARYHEAP_H

#include "../UtilTool.h"

#define PERCOLATE_POSITION              0
#define PERCOLATE_HOLE                  1
#define BINARY_HEAP_ARRAY_DEFAULT_SIZE  8

template <typename> class BinaryHeap;
template <typename T> inline void BinaryHeapExtendFunc(BinaryHeap<T>*, int);
template <typename T> inline void onMoveHeapValue(BinaryHeap<T>*, int, int) throw(std::runtime_error);

template <typename T> class BinaryHeap final{
    friend void BinaryHeapExtendFunc<T>(BinaryHeap<T>*, int);
    friend void onMoveHeapValue<T>(BinaryHeap<T>*, int, int);
public:
    BinaryHeap(const std::function<bool(const T&, const T&)> &compare = std::less<T>(), int size = BINARY_HEAP_ARRAY_DEFAULT_SIZE)
            : current_size(0), total_size(0), array(nullptr), percolate_func(nullptr),compare_func(compare){
        extendHeap(size);
    }
    ~BinaryHeap(){
        current_size = 0;
        total_size = 0;
        if(array) free(array);
    }

    BinaryHeap(const BinaryHeap&) = default;
    BinaryHeap(BinaryHeap&&) = delete;
    BinaryHeap& operator=(const BinaryHeap&) = delete;
    BinaryHeap& operator=(BinaryHeap&&) = delete;

    T& operator[](size_t n){
        return array[n];
    }
    const T& operator[](size_t n) const {
        return array[n];
    }

    T& get() {
        return array[PERCOLATE_HOLE];
    }

    T& extreme(){
        int start = (current_size / 2) + 1, end = current_size, extreme_pos = 0;

        if(start >= end){
            extreme_pos = start;
        }else{
            for(extreme_pos = start, start++; start < end; start++){
                if(!compare_func(array[extreme_pos], array[start])){
                    extreme_pos = start;
                }
            }
        }

        return array[extreme_pos];
    }

    void insert(const T &data) throw(std::bad_alloc, std::runtime_error) {
        if(++current_size >= total_size){
            extendHeap(total_size * 2);
        }
        new (reinterpret_cast<void*>(array + current_size)) T(data);
        percolateUp(current_size);
    }
    void insert(T &&data) throw(std::bad_alloc, std::runtime_error) {
        if(++current_size >= total_size){
            extendHeap(total_size * 2);
        }
        new (reinterpret_cast<void*>(array + current_size)) T(std::move(data));
        percolateUp(current_size);
    }
    void remove() throw(std::runtime_error){
        if(current_size <= 0){
            return;
        }

        onMoveHeapValue(this, PERCOLATE_HOLE, current_size--);
        percolateDown(PERCOLATE_HOLE);
    }
    bool empty() const { return (current_size <= 0); }

    void increase(int pos) throw(std::runtime_error) {
        percolateUp(pos);
    }
    void reduce(int pos) throw(std::runtime_error) {
        percolateDown(pos);
    }
    void setPercolateFunc(void (*percolate)(T&,int)) { percolate_func = percolate;}

    void ergodicHeap(){
        for(int i = 1; i <= current_size; i++){
            std::cout << "i->" << i << " : " << array[i] << std::endl;
        }
    }
private:
    void extendHeap(int extend_size) throw(std::bad_alloc) {
        {
            T *old_array = array;
            if(!(array = reinterpret_cast<T*>(malloc(sizeof(T) * extend_size)))){
                throw std::bad_alloc();
            }

            if(current_size) {
                if(std::is_nothrow_move_constructible<T>::value){
                    uninitialized_copy(std::make_move_iterator(old_array), std::make_move_iterator(old_array + total_size), array);
                } else {
                    try {
                        uninitialized_copy(old_array, old_array + total_size, array);
                    }catch (...){
                        free(array);
                        array = old_array;
                        throw std::bad_alloc();
                    }
                }

                free(old_array);
            }

            total_size = extend_size;
        }
    }
    void percolateUp(int up_pos) throw(std::runtime_error) {
        int parent_pos = up_pos / 2;

        onMoveHeapValue(this, PERCOLATE_POSITION, up_pos);
        for(;;){
            if(parent_pos <= 0){
                break;
            }

            if(compare_func(array[parent_pos], array[PERCOLATE_POSITION])){
                onMoveHeapValue(this, up_pos, parent_pos);

                if(percolate_func != nullptr){
                    percolate_func(array[parent_pos], up_pos);
                }
            }else{
                break;
            }

            up_pos = parent_pos;
            parent_pos /= 2;
        }

        onMoveHeapValue(this, up_pos, PERCOLATE_POSITION);
        if(percolate_func != nullptr){
            percolate_func(array[PERCOLATE_POSITION], up_pos);
        }
    }
    void percolateDown(int down_pos) throw(std::runtime_error){
        int cleft = 0, cright = 0, compare_pos = 0;

        onMoveHeapValue(this, PERCOLATE_POSITION, down_pos);
        for(;;){
            cleft = down_pos * 2;
            cright = (down_pos * 2) + 1;

            if(cleft > current_size){
                break;
            }

            if((cright > current_size) || compare_func(array[cright], array[cleft])){
                compare_pos = cleft;
            }else{
                compare_pos = cright;
            }

            if(compare_func(array[PERCOLATE_POSITION], array[compare_pos])){
                onMoveHeapValue(this, down_pos, compare_pos);
                if(percolate_func != nullptr){
                    percolate_func(array[compare_pos], down_pos);
                }
            }else{
                break;
            }

            down_pos = compare_pos;
        }

        onMoveHeapValue(this, down_pos, PERCOLATE_POSITION);
        if(percolate_func != nullptr){
            percolate_func(array[PERCOLATE_POSITION], down_pos);
        }
    }

    int current_size;
    int total_size;
    T *array;
    void (*percolate_func)(T&,int);
    std::function<bool(const T&, const T&)> compare_func;
};

template <typename T> inline void BinaryHeapExtendFunc(BinaryHeap<T> *binary_heap, int extend_size){
    {
        T *old_array = binary_heap->array;
        if(!(binary_heap->array = reinterpret_cast<T*>(malloc(sizeof(T) * extend_size)))){
            throw std::bad_alloc();
        }

        if(binary_heap->current_size) {
            try {
                std::uninitialized_copy(old_array, old_array + binary_heap->total_size, binary_heap->array);
            }catch (...){
                free(binary_heap->array);
                binary_heap->array = old_array;
                throw std::bad_alloc();
            }

            free(old_array);
        }

        binary_heap->total_size = extend_size;
    }
}

template <typename T> inline void onMoveHeapValue(BinaryHeap<T> *binary_heap, int up, int down) throw(std::runtime_error){
    try {
        new (reinterpret_cast<void*>(binary_heap->array + up)) T(std::move(binary_heap->array[down]));
    }catch (...){
        throw std::runtime_error(onRuntimeErrorValue("BinaryHeap", "PercolateUp", up, down));
    }
}

template <> inline void BinaryHeap<bool>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<char>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<signed char>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<unsigned char>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<wchar_t>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<char16_t>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<char32_t>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<short>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<unsigned short>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<int>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<unsigned int>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<long>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<unsigned long>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<long long>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<unsigned long long >::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<float>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<double>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }
template <> inline void BinaryHeap<long double>::extendHeap(int extend_size) throw(std::bad_alloc) { BinaryHeapExtendFunc(this, extend_size); }

//template <typename T> inline bool onCompareFunc(const T &first, const T &second){
//    return std::less<T>(first, second);
//}

template <typename T> class BinaryHeap<T*> final{
public:
    BinaryHeap(const std::function<bool(const T&, const T&)> compare = std::less<T>(), int size = BINARY_HEAP_ARRAY_DEFAULT_SIZE)
            : current_size(0), total_size(0), array(nullptr), percolate_func(nullptr), compare_func(compare){
        extendHeap(size);
    }
    ~BinaryHeap(){
        current_size = 0;
        total_size = 0;
        if(array) free(array);
    }

    BinaryHeap(const BinaryHeap&) = default;
    BinaryHeap(BinaryHeap&&) = delete;
    BinaryHeap& operator=(const BinaryHeap&) = delete;
    BinaryHeap& operator=(BinaryHeap&&) = delete;

    T* operator[](size_t n){
        return array[n];
    }
    const T* operator[](size_t n) const{
        return array[n];
    }

    T* get() { return array[1]; }

    T* extreme(){
        int start = (current_size / 2) + 1, end = current_size,
                extreme_pos;

        if(start >= end){
            extreme_pos = start;
        }else{
            for(extreme_pos = start, start++; start < end; start++){
                if(!compare_func(*array[extreme_pos], *array[start])){
                    extreme_pos = start;
                }
            }
        }

        return array[extreme_pos];
    }

    void insert(T* data) throw(std::bad_alloc) {
        if(++current_size >= total_size){
            extendHeap(total_size * 2);
        }

        memcpy(array + current_size, data, sizeof(T*));
        percolateUp(current_size);
    }

    void remove() {
        if(current_size <= 0){
            return;
        }

        memcpy(array + PERCOLATE_HOLE, array + current_size--, sizeof(T*));
        percolateDown(PERCOLATE_HOLE);
    }

    bool empty() const { return (current_size <= 0); }

    void increase(int pos){ percolateUp(pos);}
    void reduce(int pos) { percolateDown(pos); }
    void setPercolateFunc(void (*percolate)(T*,int)) { percolate_func = percolate;}

    void ergodicHeap(){
        for(int i = 1; i <= current_size; i++){
            std::cout << "i->" << i << " : " << *(array[i]) << std::endl;
        }
    }
private:
    void extendHeap(int extend_size) throw(std::bad_alloc) {
        T **old_array = array;
        if(!(array = (T**)(malloc(sizeof(T*) * extend_size)))){
            throw std::bad_alloc();
        }

        if(current_size){
            memcpy(array, old_array, sizeof(T*) * current_size);
            free(old_array);
        }

        total_size = extend_size;
    }

    void percolateUp(int up_pos){
        int parent_pos = current_size / 2;

        memcpy(array + PERCOLATE_HOLE, array + current_size, sizeof(T*));
        for(;;){
            if(parent_pos <= 0){
                break;
            }

            if(compare_func(*array[parent_pos], *array[PERCOLATE_POSITION])){
                memcpy(array + up_pos, array + parent_pos, sizeof(T*));
                if(percolate_func != nullptr){
                    percolate_func(array[parent_pos], up_pos);
                }
            }else{
                break;
            }

            up_pos = parent_pos;
            parent_pos /= 2;
        }

        memcpy(array + up_pos, array + PERCOLATE_POSITION, sizeof(T*));
        if(percolate_func != nullptr){
            percolate_func(array[PERCOLATE_POSITION], up_pos);
        }
    }

    void percolateDown(int down_pos) {
        int cleft = 0, cright = 0,compare_pos = 0;

        memcpy(array + PERCOLATE_POSITION, array + down_pos, sizeof(T*));
        for(;;){
            cleft = down_pos * 2;
            cright = (down_pos * 2) + 1;


            if(cleft > current_size){
                break;
            }

            if((cright > current_size) || compare_func(*array[cright], *array[cleft])){
                compare_pos = cleft;
            }else{
                compare_pos = cright;
            }

            if(compare_func(*array[PERCOLATE_POSITION], *array[compare_pos])){
                memcpy(array + down_pos, array + compare_pos, sizeof(T*));
                if(percolate_func != nullptr){
                    percolate_func(array[compare_pos], down_pos);
                }
            }else{
                break;
            }

            down_pos = compare_pos;
        }

        memcpy(array + down_pos, array + PERCOLATE_POSITION, sizeof(T*));
        if(percolate_func != nullptr){
            percolate_func(array[PERCOLATE_POSITION], down_pos);
        }
    }

    int current_size;
    int total_size;
    T **array;
    void (*percolate_func)(T*,int);
    std::function<bool(const T&, const T&)> compare_func;
};


#endif //UNTITLED8_BINARYHEAP_H
