//
// Created by abc on 19-9-2.
//

#ifndef UNTITLED8_SPACEMANAGER_H
#define UNTITLED8_SPACEMANAGER_H


#include "util/MemoryManagerUtil.h"
#include "util/MemoryManagerRecordUtil.h"

class MemoryManager;
//static template <typename T, typename... Args> MemoryCorrelate<T>* CreateCorrelateMemory(Args&&...);
//static template <typename T> void DestroyCorrelateMemory(MemoryCorrelate<T>*);
/**
 *                                                              <-----  MemoryCorrelate<MemoryCorrelate>
 * 只有两层高度 manager  <----- MemoryCorrelate<MemoryManager>    <-----  MemoryCorrelate<MemoryCorrelate>
 *                                                              <-----  MemoryCorrelate<MemoryCorrelate>
 * @tparam T
 */
//template <typename T> class MemoryCorrelate{
//public:
//    typedef T CorrelateType;
//    typedef MemoryCorrelate<MemoryManager>      CorrelateParent;
//    typedef MemoryCorrelate<MemoryCorrelate>    CorrelateChild;
//
//    MemoryCorrelate(CommonHead*, MemoryManager*);
//    MemoryCorrelate(MemoryCorrelate*);
//    virtual ~MemoryCorrelate();
//
//    MemoryCorrelate(const MemoryCorrelate&) = delete;
//    MemoryCorrelate(MemoryCorrelate&&) = delete;
//    MemoryCorrelate& operator=(const MemoryCorrelate&) = delete;
//    MemoryCorrelate& operator=(MemoryCorrelate&&) = delete;
//
////    template <typename U, typename... Args> MemoryCorrelate<U>& swap(const MemoryCorrelate<T>&, Args&&...);
////    template <typename U = MemoryManager> void setTemplate(CorrelateParent&, MemoryManager*);
////    template <typename U = MemoryCorrelate> void setTemplate(CorrelateChild&, CorrelateParent*, std::list<CorrelateChild*>::iterator&&);
//
//    virtual CommonHead* getCorrelate();
//    virtual void setCorrelate(CommonHead*);
//    virtual void releaseCorrelate();
//    virtual void blastCorrelate();
//
//    template <typename U = MemoryCorrelate> void mountMemoryCorrelate(CorrelateChild*);
//    template <typename U = MemoryCorrelate> void unmountMemoryCorrelate(CorrelateChild*);
//private:
////    static template <typename U = MemoryManager> void setTemplate(CorrelateParent&, const CorrelateParent&);
////    static template <typename U = MemoryCorrelate> void setTemplate(CorrelateChild&, const CorrelateChild&);
//
//    template <typename U> inline void sameCorrelateTemplate() const;
//    template <typename U = MemoryManager> void adsorbMemoryCorrelate(CorrelateParent*);
//    template <typename U = MemoryManager> void unadsorbMemoryCorrelate();
//
//    std::atomic_bool memory_done;
//    union {
//        struct{
//            std::atomic_int  child_size;
//            CommonHead       *buf_head;
//            MemoryManager    *manager;
//            std::shared_timed_mutex correlate_lock;
//            UnLockQueue<MemoryCorrelate*> child;
//        } manager_info;
//        MemoryCorrelate<MemoryManager> *parent;
//    };
//};
#define MEMORY_BUFFER_SIGN_IDLE_INIT_SIZE           2
#define MEMORY_BUFFER_SIGN_DEFAULT_INIT_SIZE        1

#define MEMORY_BUFFER_FLAG_NONE                     0
#define MEMORY_BUFFER_FLAG_BIG_BUFFER               0x01
#define MEMORY_BUFFER_FLAG_MAKE_SPACE_HEAD          0x02
#define MEMORY_BUFFER_FLAG_INIT_CORRELATE           0x04
#define MEMORY_BUFFER_FLAG_FIND_UNBREAK             0x08
#define MEMORY_BUFFER_FLAG_NOSIZE_UNBREAK           0x10

#define MEMORY_BUFFER_ALLOC_FIXED_SIZE              (1024 * 64)
#define MEMORY_BUFFER_ALLOC_IDLE_SIZE               (1024 * 128)
#define MEMORY_BUFFER_LOAD_FACTOR                   0.7
#define MEMORY_BUFFER_BIG_MEMORY_FACTOR             0.3

#define MEMORY_BUFFER_THREAD_PERMIT_INIT_VALUE      0
#define MEMORY_BUFFER_THREAD_PERMIT_CHANGE_VALUE    1
#define MEMORY_BUFFER_THREAD_PERMIT_DONE_VALUE      1

#define MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT     0
#define MEMORY_BUFFER_THREAD_PERMIT_STATUS_REQUEST  1
#define MEMORY_BUFFER_THREAD_PERMIT_STATUS_DONE     2

#define MEMORY_BUFFER_THREAD_PERMIT_CREATE_FROCE    true
#define MEMORY_BUFFER_THREAD_PERMIT_CREATE_APPLY    false

#define MEMORY_BUFFER_MERGE_IDLE_INFO_UNLINK        true
#define MEMORY_BUFFER_MERGE_IDLE_INFO_NOTUNLINK     false


/**
 * 当UnLockDefaultNote从A里插入到B里,需要先从A里删除掉,然后再插入到B里,因为当A和B里都存在同一个UnLockDefaultNote时,
 * 此时会共享B设置的UnLockBaseNote的id,会导致A里的链表的id乱序问题(一个UnLockDefaultNote只能存在一个UnLockList里)
 */
template <> struct UnLockDefaultNote<MemoryBuffer*> : public UnLockBaseNote{
    UnLockDefaultNote() : UnLockBaseNote(), data(nullptr) {}
    explicit UnLockDefaultNote(MemoryBuffer *d) : UnLockBaseNote(), data(d) {}
    ~UnLockDefaultNote() override = default;

    static UnLockDefaultNote<MemoryBuffer*>* createNote(MemoryBuffer *data) throw(std::bad_alloc) { return data ? (UnLockDefaultNote<MemoryBuffer*>*)(++data) : throw std::bad_alloc(); }
    static void destroyNote(UnLockDefaultNote<MemoryBuffer*> *note) { note->onFormatNote(); } //不需要实现

    MemoryBuffer *data;
};

typedef UnLockDefaultNote<MemoryBuffer*>                                MemoryBufferNote;
typedef UnLockList<MemoryBuffer*, MemoryBufferNote>                     MemoryBufferList;
typedef MemoryBufferList::UnLockConcurrentException                     MemoryBufferException;
typedef MemoryBufferList ::UnLockEraseException                         MemoryBufferEraseFinal;
typedef MemoryBufferList::UnLockListHolder                              MemoryBufferHolder;
typedef MemoryBufferList::UnLockIterator                                MemoryBufferIterator;
class SpaceControl;
struct SearchInfo;


/**
 * 内存生产者
 */
class MemoryProducer{
public:
    typedef struct PermitInfo{
        PermitInfo() : permit(MEMORY_BUFFER_THREAD_PERMIT_INIT_VALUE), status(MEMORY_BUFFER_THREAD_PERMIT_STATUS_WAIT) {}
        ~PermitInfo() = default;

        std::atomic_int permit;
        std::atomic_int status;
    };
    struct InitMemoryBuffer{
        int len;
        CommonHead *common;
    };

    MemoryProducer() = default;
    virtual ~MemoryProducer() = default;

    void makeMemoryBuffer(bool, int, PermitInfo*, std::function<void(int)>);
    void doneMemoryBuffer(PermitInfo*);

    int initMemoryBuffer(int, int, InitMemoryBuffer*) throw(std::bad_alloc);
    void uinitMemoryBuffer(const InitMemoryBuffer&);
    void uinitMemoryBuffer(CommonHead*);

    void onMakeMemoryBuffer(MemoryBuffer*, PermitInfo*);
    void onConsumeMemoryBuffer(MemoryBuffer*, PermitInfo*);
    virtual void onInitMemoryBuffer(std::function<void(int)>&, int) = 0;

    PermitInfo fixed_info;
    PermitInfo idle_info;
private:
    int createForce(std::atomic_int&);
    int createApply(std::atomic_int&);
    void waitCreate(std::atomic_int&);
    char* createMemoryBuffer(int);
    void destroyMemoryBuffer(char*, int);
};

/**
 * 内存审判者
 */
class MemoryAdjudicator{
private:
#define MEMORY_ADJUDICATOR_BUFFER_INFO_INCREASE_VALUE  1
    friend int insertMemoryBufferUseToIdle(char*,char*,int);
    friend char* nextMemoryBufferIdle(char*);

    struct SearchInfo{
        SearchInfo() = default;
        SearchInfo(MemoryBuffer *m, std::function<MemoryBufferIdleInfo*(SearchInfo*)> *ufunc, std::function<int(SearchInfo*)> *ifunc,
                   std::function<void(MemoryAdjudicator*, SearchInfo*)> *sfunc)
                : is_last(false), memory_buffer(m), use_info(nullptr), pu_info(nullptr), upi_info(nullptr), idle_info(nullptr), pi_info(nullptr),
                  use_func(ufunc), idle_func(ifunc), shrink_func(sfunc){}
        ~SearchInfo() = default;
        SearchInfo(const SearchInfo&) = default;
        SearchInfo(SearchInfo&&) = default;
        SearchInfo& operator=(const SearchInfo&) = default;
        SearchInfo& operator=(SearchInfo&&) = default;

        bool is_last;                                                       //是否最后一个info
        MemoryBuffer *memory_buffer;                                        //对应的MemoryBuffer
        MemoryBufferUseInfo *use_info, *pu_info, *upi_info;
        MemoryBufferIdleInfo *idle_info, *pi_info;
        std::function<int(SearchInfo*)> *idle_func;                         //idle_info回调函数
        std::function<MemoryBufferIdleInfo*(SearchInfo*)> *use_func;        //use_info回调函数
        std::function<void(MemoryAdjudicator*, SearchInfo*)> *shrink_func;  //收缩函数
    };
public:
    MemoryAdjudicator() = default;
    virtual ~MemoryAdjudicator() = default;

    MemoryBufferUseInfo* initMemoryBufferUseInfo(MemoryBufferUseInfo*, MemoryCorrelate*, MemoryBuffer*, int, int, int);
    MemoryBufferIdleInfo* initMemoryBufferIdleInfo(MemoryBuffer*, MemoryBufferUseInfo*, int, int);
    MemoryBufferSubstituteInfo* initMemoryBufferSubstituteInfo(MemoryBufferUseInfo*);
    MemoryBufferSign memoryBufferUseStatus(MemoryBuffer*);

    MemoryBufferIdleInfo* recoverMemoryBufferUseInfo(MemoryBufferUseInfo*, MemoryBufferIdleInfo*, bool);
    CommonHead* searchAvailableBuffer(MemoryBuffer*, std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)>*, std::function<void(MemoryBufferUseInfo*)>*,
                                      std::function<void(MemoryBufferIdleInfo*)>*, int, int);
    /**
     * 修理一个满载的MemoryBuffer
     */
    void repairLoadMemoryBuffer(MemoryBuffer*, std::function<void(MemoryBufferUseInfo*)>*, std::function<void(MemoryBufferIdleInfo*)>*);

    /**
     * 整理满载的MemoryBuffer
     */
    void treatmentLoadMemoryBuffer(MemoryBufferHolder&, int, int, int, int, int);

    /**
     * 强制整理满载的MemoryBuffer
     */
    void forceLoadMemoryBuffer(MemoryBufferHolder&, int, int, int);

    virtual void ergodicMemoryBufferInfo(MemoryBuffer*) = 0;

    void linkMemoryBufferUseInfo(MemoryBufferUseInfo*, MemoryBufferUseInfo*);
    void unlinkMemoryBufferUseInfo(MemoryBufferUseInfo*);
    MemoryBufferUseInfo* substituteMemoryBufferUseInfo(MemoryBufferUseInfo*, MemoryBufferIdleInfo*);

    MemoryBufferUseInfo* memmoveMemoryBufferUseInfo(MemoryBuffer*, MemoryBufferUseInfo*, MemoryBufferIdleInfo*, MemoryBufferUseInfo*, MemoryBufferIdleInfo*, std::function<void(int)>);
    MemoryBufferIdleInfo* memmoveMemoryBufferIdleInfo(MemoryBuffer*, MemoryBufferIdleInfo*, MemoryBufferIdleInfo*, char*);

    int shrinkMemoryBufferIdleInfo(MemoryBufferIdleInfo*, MemoryBufferUseInfo*, MemoryBuffer*);
    void shrinkMemoryBufferIdleInfo0(MemoryBufferIdleInfo*, MemoryBufferUseInfo*, MemoryBuffer*);

    MemoryBufferIdleInfo* linkMemoryBufferIdleInfo(MemoryBufferIdleInfo*, MemoryBufferIdleInfo*, MemoryBuffer*);
    MemoryBufferIdleInfo* unlinkMemoryBufferIdleInfo(MemoryBufferIdleInfo*, MemoryBuffer*);

    int isMergeMemoryBufferIdleInfo(MemoryBufferIdleInfo *idle_info, MemoryBufferIdleInfo *pi_info) const { return (idle_info && pi_info && (pi_info->end >= idle_info->start)); }
    /**
     * 合并两个相邻的可用内存
     */
    void mergeMemoryBufferIdleInfo(MemoryBuffer *buffer, MemoryBufferIdleInfo *idle_info, MemoryBufferIdleInfo *pi_info, bool un_link) {
        idle_info->start = pi_info->start; idle_info->ipu_info = pi_info->ipu_info;
        if(un_link) { unlinkMemoryBufferIdleInfo(pi_info, buffer); }
    }

    MemoryBufferIdleInfo* expandMemoryBufferIdleInfo(MemoryBufferIdleInfo*, MemoryBuffer*, int);
protected:
    virtual void onConstructMemoryBuffer(CommonHead*, int) = 0;
    virtual void onMemoryBufferRecorderDebris(uint32_t) = 0;
    virtual void onReleaseMemoryCorrelate(MemoryCorrelate*) = 0;

    virtual void onResponseMemoryBuffer(MemoryBufferHolder&) = 0;
    virtual MemoryBufferHolder onRequestLoadMemoryBuffer() = 0;
private:
    static MemoryBufferSign memoryBufferUseStatus0(int, int, MemoryBufferSign);
    void searchMemoryBuffer(SearchInfo*);
};

/**
 * 内存记录者
 */
class MemoryRecorder : public MemoryAdjudicator{
    friend struct RecordFixed;
    friend struct RecordIdle;
    friend struct RecordUse;
    friend struct RecordLoad;
    friend struct RecordExternal;
    friend struct RecordDebris;

    typedef struct RecordInfo{
        RecordInfo() : external_number(0), debris_size(0),
                       fixed_number(0), fixed_call_size(0), fixed_success_size(0), fixed_occupy_size(0), fixed_unoccupy_size(0),
                       idle_number(0), idle_call_size(0), idle_success_size(0),
                       use_number(0), use_call_size(0), use_success_size(0), use_occupy_size(0), use_unoccupy_size(0),
                       load_number(0), load_call_size(0) {}
        ~RecordInfo() = default;

        uint32_t external_number;
        std::atomic_uint debris_size;
        std::atomic_uint fixed_number, fixed_call_size, fixed_success_size, fixed_occupy_size,fixed_unoccupy_size;
        std::atomic_uint idle_number, idle_call_size, idle_success_size;
        std::atomic_uint use_number, use_call_size, use_success_size, use_occupy_size, use_unoccupy_size;
        std::atomic_uint load_number, load_call_size;
    };
public:
    typedef struct LookupInfo{
        LookupInfo(int bf, int ll, int abl, int uv, MemoryBufferSign s, std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> *lf)
                : search_load_buffer(true), search_flag(bf), lookup_len(ll), alloc_buffer_len(abl), unmount_value(uv),
                  load_use_len(0), load_use_size(0), load_buffer_len(0), load_buffer_size(0), sign(s), lookup_func(lf){}
        ~LookupInfo() = default;

        bool search_load_buffer;    //是否已经搜索了MEMORY_BUFFER_SIGN_LOAD
        int search_flag;            //搜索标记
        int lookup_len;             //搜索长度
        int alloc_buffer_len;       //MemoryBuffer的长度
        int unmount_value;          //MemoryBuffer的卸载次数

        int load_use_len;           //MEMORY_BUFFER_SIGN_LOAD的MemoryBuffer的所有use_info的长度
        int load_use_size;          //MEMORY_BUFFER_SIGN_LOAD的MemoryBuffer的use_info数量
        int load_buffer_len;        //MEMORY_BUFFER_SIGN_LOAD的MemoryBuffer的所有的长度
        int load_buffer_size;       //MEMORY_BUFFER_SIGN_LOAD的MemoryBuffer的数量

        MemoryBufferSign sign;      //搜索的sign
        std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)> *lookup_func; //搜索回调函数
    };

    MemoryRecorder();
    ~MemoryRecorder() override = default;

    MemoryBufferHolder extractLoadMemoryBuffer();
//    void popMemoryBufferToSign(MemoryBufferSign);
    void initMemoryBufferArrayOperatorConcurrent(bool);

    void mountMemoryBuffer(MemoryBuffer*, bool);
    bool unmountMemoryBuffer(MemoryBuffer*, int);

    CommonHead* lookupAvailableBuffer(MemoryBufferSign, int, int, int, std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)>*);
    CommonHead* lookupAvailableBuffer0(LookupInfo*, MemoryBufferHolder*);
    CommonHead* lookupAvailableBufferOnLoad(LookupInfo*, MemoryBufferHolder*);

    void ergodicMemoryBufferInfo(MemoryBuffer*) override;
    void ergodicMemoryBufferRecorder();

    uint32_t getRecordInfo(MemoryBufferSign sign, const RecordNumber &r) const { return records[sign]->getRecord(this, r); }
    uint32_t getRecordInfo(MemoryBufferSign sign, const RecordSize &r) const { return records[sign]->getRecord(this, r); }
    uint32_t getRecordInfo(MemoryBufferSign sign, const RecordCallSize &r) const { return records[sign]->getRecord(this, r); }
    uint32_t getRecordInfo(MemoryBufferSign sign, const RecordSuccessSize &r) const { return records[sign]->getRecord(this, r); }
protected:
    virtual void onMemoryBufferReady();
    virtual void onMemoryBufferUnReady(MemoryBufferSign);
    virtual void onPopMemoryBuffer(MemoryBufferSign, std::function<void(MemoryBuffer*)>);
    virtual void onMemoryBufferRecorderDebris(uint32_t size) { records[MEMORY_BUFFER_SIGN_DEBRIS]->setRecord(size, this, RecordSize()); }

    virtual CommonHead* onLookupMemoryBuffer(MemoryBuffer*, std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)>*, int*, int*, int, int) = 0;
    virtual void onMemoryBufferExhaust(MemoryBuffer*) = 0;
    virtual void onForceCreateMemoryBuffer() = 0;
private:
    void onMemoryBufferUnReady0(MemoryBufferList*);

    MemoryBufferList buffer_array[MEMORY_BUFFER_SIGN_SIZE];
    static RecordBasic *const records [MEMORY_BUFFER_SIGN_SIZE];
    RecordInfo record_info;
};

class MemoryManager : private MemoryRecorder, private MemoryProducer{
    typedef void (*init_func)(MemoryManager*, int);

    friend int insertMemoryBufferUseToIdle(char*,char*,int);
    friend char* nextMemoryBufferIdle(char*);
public:
    MemoryManager(int fixed_init_size = MEMORY_BUFFER_SIGN_DEFAULT_INIT_SIZE, uint32_t fixed_buffer_len = MEMORY_BUFFER_ALLOC_FIXED_SIZE,
                                 uint32_t idle_buffer_len = MEMORY_BUFFER_ALLOC_FIXED_SIZE) throw(std::bad_alloc);
    ~MemoryManager() override;

    MemoryManager(const MemoryBuffer&) = delete;
    MemoryManager(MemoryBuffer&&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    //申请内存的最小值
    //方便当use_info移动后的内存可以容纳最少一个MemoryBufferSubstituteInfo
    int minMemoryLen() const { return sizeof(MemoryBufferSubstituteInfo); }
    void ergodicRecorder() { ergodicMemoryBufferRecorder(); }
    MemoryCorrelate* obtainBuffer(uint32_t);
    static void returnBuffer(MemoryCorrelate*);

    static constexpr uint32_t default_fixed_buffer_len = MEMORY_BUFFER_ALLOC_FIXED_SIZE;
    static constexpr uint32_t default_idle_buffer_len = MEMORY_BUFFER_ALLOC_IDLE_SIZE;
private:
    uint32_t allocFixedBufferLen() const { return default_fixed_buffer_len >= memory_fixed_buffer_len
                                                         ? default_fixed_buffer_len : memory_fixed_buffer_len; }
    uint32_t allocIdleBufferLen() const { return default_idle_buffer_len >= memory_idle_buffer_len
                                                        ? default_idle_buffer_len : memory_idle_buffer_len; }

    void initFixedBuffer(int) throw(std::bad_alloc);
    void initIdleBuffer(int) throw(std::bad_alloc);

    void uinitFixedBuffer();
    void uinitIdleBuffer();

    CommonHead* makeInsideMemoryBuffer(const SpaceHead&, int);
    MemoryBuffer* makeMemoryBufferHead(char*, int, MemoryBufferSign);
    MemoryBuffer* initMemoryBufferHead(MemoryBuffer*, char*, int, MemoryBufferSign);

    static void initMemoryCorrelate(MemoryCorrelate*, char*);
    static void releaseMemoryBuffer(MemoryBufferUseInfo*);

    void responseLoadMemoryBuffer(MemoryBuffer*);
protected:
    CommonHead* onLookupMemoryBuffer(MemoryBuffer*, std::function<MemoryBufferUseInfo*(MemoryBuffer*, int, int, int)>*, int*, int*, int, int) override;
    void onConstructMemoryBuffer(CommonHead*, int) override;

    void onForceCreateMemoryBuffer() override;
    void onInitMemoryBuffer(std::function<void(int)>&, int) override;
    void onMemoryBufferExhaust(MemoryBuffer*) override;

    MemoryBufferHolder onRequestLoadMemoryBuffer() override;
    void onResponseMemoryBuffer(MemoryBufferHolder&) override;
    void onReleaseMemoryCorrelate(MemoryCorrelate*) override;
private:
    uint32_t memory_fixed_buffer_len;
    uint32_t memory_idle_buffer_len;
};

#endif //UNTITLED8_SPACEMANAGER_H
