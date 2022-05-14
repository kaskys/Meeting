//
// Created by abc on 21-2-24.
//

#ifndef TEXTGDB_TRANSMITDATAUTIL_H
#define TEXTGDB_TRANSMITDATAUTIL_H

#ifdef __cplusplus
extern "C"{
#endif
#include "../../transmit/TransmitData.h"
#ifdef __cplusplus
};
#endif
#include "TransmitUtil.h"

#define TRANSMIT_PERMIT_DEFAULT_SIZE            8   //关联SocketWordThread的SOCKET_PERMIT_DEFAULT_SIZE
#define TRANSMIT_LAYER_DEFAULT_OUTPUT_MAX       10  //重复输入最大值

struct MsgHdr;
struct TransmitMemoryData;
class TransmitLayer;
class TransmitThreadUtil;
class SocketWordThread;

void onTransmitInput(int, int, TransmitThreadUtil*);
void onTransmitData(TransmitReadInfo*, int);
void onTransmitUinit(TransmitReadInfo*);
int  onTransmitInit(TransmitReadInfo*);
int  onTransmitReadData(TransmitReadInfo*, int, int);
int  onTransmitBufferCreate(TransmitReadInfo*, int);
std::pair<int,int> onTransmitOutput(int, int, sockaddr_in*, MsgHdr*);

/*
 * 传输输出数据
 */
struct TransmitOutputData{
    int output_len;                     //输出长度
    int output_max;                     //单次输出失败最大次数
    MsgHdr *output_msg;                 //输出信息
    MeetingAddressNote *output_note;    //远程端
    void(*done_func)(TransmitParameter*, MsgHdr*);  //输出完成函数
    void(*cancel_func)(MsgHdr*);                    //输出取消函数（参数错误,终止输出）
};

/**
 * 短暂存储传输输入的数据（用于防止动态内存在读取第一次输入之后及第二次输入之前被释放）
 */
class TransmitInputData final{
public:
    void initInputData(int len, const sockaddr_in &addr, const MemoryReader &reader){
        this->transmit_len = len; this->addr = addr;
        this->store_reader = reader;
    }
    void onTransmitInputData(TransmitThreadUtil*);

    int transmit_len;           //传输输入长度
    sockaddr_in addr;           //传输输入地址
    MemoryReader store_reader;  //传输输入读取器
};

/**
 * SocketThread传输层参数
 * 线程变量(因为SocketThread线程都由ManagerThread线程负责创建和销毁,所以该变量的构造函数与析构函数的相关操作都同步与ManagerThread线程)
 */
class ThreadParameter {
    friend class TransmitLayer;
    friend class TransmitOutputInfo;
public:
    ThreadParameter();
    virtual ~ThreadParameter();
protected:
    TransmitParameter* getThreadParameter() { return &parameter; }
private:
    std::atomic_bool valid;         //是否无效
    ThreadParameter  *prev, *next;  //上一个、像一个
    TransmitParameter parameter;    //传输参数类
};

/*
 * 传输编码工具
 */
class TransmitCodecUtil {
public:
    TransmitCodecUtil() : analysis_util() {
        for(int i = 0; i < TRANSMIT_PERMIT_DEFAULT_SIZE; i++){
            new (reinterpret_cast<void*>(compile_util + i)) TransmitCompileUtil();
        }
    }
    virtual ~TransmitCodecUtil() = default;

    TransmitCompileUtil*  getTransmitCompileUtil(uint32_t pos) { return (compile_util + pos);}
    TransmitAnalysisUtil* getTransmitAnalysisUtil() { return &analysis_util;}
private:
    TransmitCompileUtil compile_util[TRANSMIT_PERMIT_DEFAULT_SIZE]; //传输编码工具类（一个对应一个远程端）
    TransmitAnalysisUtil analysis_util;                             //传输解析工具类（一个对应多个远程端）
};


/**
 * SocketThread传输层工具（一对一关系）
 *  SocketThread线程创建时,会向控制层发送创建消息,控制层会请求传输层创建该工具
 *  SocketThread线程销毁时,会向控制层发送销毁消息,控制层会请求传输层销毁该工具
 */
class TransmitThreadUtil final : public ThreadParameter, public TransmitCodecUtil{
#define TRANSMIT_THREAD_ID_GENERATOR_VALUE  1
    friend class TransmitLayer;
    friend class SocketWordThread;
public:
    explicit TransmitThreadUtil(sockaddr_in*) throw(std::runtime_error);
    ~TransmitThreadUtil() override;

    void onLaunchTransmit(sockaddr_in*, int recv_size = 0, int send_size = 0) throw (std::runtime_error);
    void onTerminateTransmit();

    void onSocketRead();
    void onSocketWrite(const TransmitOutputData&);
    void onSocketInput(TransmitInputData*);
    bool onSocketInputVerify(MeetingAddressNote*) const;
    void onSocketOutput(MsgHdr*, MeetingAddressNote*, void(*)(TransmitParameter*, MsgHdr*), void(*)(MsgHdr*), int, int output_max = TRANSMIT_LAYER_DEFAULT_OUTPUT_MAX);

    void onRunCorrelateThread0(const std::function<void()>&);
    void onRunCorrelateThreadImmediate(const std::function<void()>&);
    bool isRunCorrelateThread() const;
    bool isThreadCorrelateRemote(MeetingAddressNote*) const;

    void correlateTransmit(TransmitLayer *layer) { this->transmit_layer = layer; }
    void correlateThread(SocketWordThread*);
    void correlateRemoteNote(MeetingAddressNote*);
    void setSocketRecvSize(int);
    void setSocketSendSize(int);

    sockaddr_in getSocketAddr() const { return thread_addr; }
    int getSocketRecvSize() const { return thread_socket_recv_size; }
    int getSocketSendSize() const { return thread_socket_send_size; }
    uint32_t getThreadCorrelateId() const { return thread_correlate_id; }

    void onNoteTerminationFunc(MeetingAddressNote*);
    void onNotePermitFunc(MeetingAddressNote *note, uint32_t pos) const { MeetingAddressManager::setNotePermitPos(note, pos); }
    uint32_t onNotePermitFunc(MeetingAddressNote *note) const { return MeetingAddressManager::getNotePermitPos(note); }

    void onError(int);
    void onDisconnect(){ connect_socket(nullptr, thread_open_fd); }
    bool onCreateDynamicBuffer(TransmitMemoryUtil*, int);
    void onCreateFixedBuffer(TransmitMemoryUtil*, int);
    void onDestroyFixedBuffer(TransmitMemoryUtil*, char*, int);
private:
    static int getTransmitSocketFd(TransmitThreadUtil *util) { return util->thread_open_fd; }
    static TransmitThreadUtil* onTransmitThreadUtil(SocketWordThread*);
    bool onSocketWrite0(std::pair<int,int>, int, MsgHdr*, void(*)(TransmitParameter*, MsgHdr*), void(*)(MsgHdr*));

    static std::atomic<uint32_t> only_id_generator; //传输线程id生成器

    int thread_open_fd;                 //socket描述符
    int thread_socket_recv_size;        //socket接收缓存数量
    int thread_socket_send_size;        //socket发送缓存数量
    uint32_t thread_correlate_id;       //传输线程的id
    sockaddr_in thread_addr;            //socket的地址信息

    TransmitLayer *transmit_layer;      //地址层
    SocketWordThread *socket_thread;    //关联的SocketThread线程
};

class TransmitMemoryDynamicUtil final : public TransmitMemoryUtil{
public:
    TransmitMemoryDynamicUtil(int len, InitLocator &locator) : TransmitMemoryUtil(len), dynamic_locator(locator) , dynamic_reader() {}
    ~TransmitMemoryDynamicUtil() override = default;

    bool onCreateMemory(int) override;
    void onCopyMemory(int, const std::function<void(char*)>&) throw(std::logic_error) override;
    MemoryReader onExtractMemory() override;
private:
    InitLocator dynamic_locator;    //动态内存初始化关联器
    MemoryReader dynamic_reader;    //动态内存读取器
};

class TransmitMemoryFixedUtil final : public TransmitMemoryUtil {
public:
    TransmitMemoryFixedUtil(int len, char *buffer, TransmitThreadUtil *util)
            : TransmitMemoryUtil(len), fixed_buffer(buffer), transmit_util(util), fixed_func(nullptr) {
        fixed_func = [&](char *buffer, int len) -> void {
            transmit_util->onDestroyFixedBuffer(this, buffer, len);
        };
    }
    ~TransmitMemoryFixedUtil() override = default;

    bool onCreateMemory(int) override;
    void onCopyMemory(int, const std::function<void(char*)>&) throw(std::logic_error) override;
    MemoryReader onExtractMemory() override;
    void setCreateFixedBuffer(int len, char *fixed, const std::function<void(char*,int)> &func) {
        resetMemoryLen(len);
        fixed_buffer = fixed; fixed_func = func;
    }
private:
    char *fixed_buffer;                         //固定内存指向的内存
    TransmitThreadUtil *transmit_util;          //传输线程工具类
    std::function<void(char*,int)> fixed_func;  //固定内存释放函数
};


#endif //TEXTGDB_TRANSMITDATAUTIL_H
