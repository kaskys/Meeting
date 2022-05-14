//
// Created by abc on 21-2-2.
//

#ifndef TEXTGDB_ADDRESSPARAMTER_H
#define TEXTGDB_ADDRESSPARAMTER_H

#include <cstdint>
#include <utility>
#include <atomic>
#include <netinet/in.h>
#include <list>

struct ParameterHost{
    uint32_t note_port;
    uint32_t note_addr;

    uint32_t start_time;            //启动时间
    uint32_t end_time;              //结束时间
    uint32_t link_rtt;              //与主机的往返时间（远程端变量）
    uint32_t link_frame;            //帧数（主机与远程端一致）
    uint32_t link_delay;        //延迟时间（主机与远程端一致）
    uint32_t link_time;             //链接时间
    uint32_t link_timeout;          //链接超时时间
    uint32_t passive_timeout;          //被动链接超时时间

//    uint32_t apply_join_size;       //申请加入数量
//    uint32_t apply_link_size;       //申请链接数据
//    uint32_t exit_size;             //退出数量（被动或主动）
//    uint32_t determine_join_size;   //确定加入数量
//    uint32_t determine_link_size;   //确定链接数量
//    uint32_t send_sequence_size;    //发送序号数量
};

struct ParameterRemote{
    uint32_t note_port;
    uint32_t note_addr;

    uint32_t remote_join_time;          //远程端加入时间（最后一次）
    uint32_t remote_link_time;          //远程端链接时间（最后一次）
    uint32_t remote_sequence_time;      //发送远程序号时间（最后一次）
    uint32_t remote_exit_time;          //远程端退出时间
    uint32_t link_sequence;             //响应远程端序号

    uint32_t recv_join_size;            //接收申请加入数量
    uint32_t recv_link_size;            //接收申请链接数量
    uint32_t send_synchro_size;         //发送同步数量
    uint32_t recv_synchro_size;         //接收同步数量
    uint32_t send_seqeuence_size;       //发送序号数量
    uint32_t note_normal_size;          //正常数量
    uint32_t note_passive_size;         //被动数量
    uint32_t note_invalid_size;         //主类型无效数量

    uint32_t value_size;                //能够接收往返时间的数量
    uint32_t note_rtt_value[0];         //接收往返时间数组
};

struct ParameterAddress{
    uint32_t address_size;
    uint32_t join_size;
    uint32_t link_size;
    uint32_t normal_size;
    uint32_t transfer_size;
    uint32_t passive_size;
    uint32_t exit_size;
    uint32_t unsupport_size;
    uint32_t invalid_size;
};


class RemoteNoteParameter{
public:
    RemoteNoteParameter() : recv_join_size(0), recv_link_size(0), send_synchro_size(0), recv_synchro_size(0),
                            send_seqeuence_size(0), note_normal_size(0), note_passive_size(0), note_invalid_size(0), note_rtt_value(){}
    ~RemoteNoteParameter(){ note_rtt_value.clear(); }

    void onNoteJoin();
    void onNoteLink();
    void onNoteSynchroSend();
    void onNoteSynchroRecv();
    void onNoteSequence();
    void onNoteRtt(uint32_t*, uint32_t);
    void onNoteNormal();
    void onNotePassive();
    void onNoteInvalid();
    void onData(ParameterRemote*);
    void onClear();
    uint32_t onRttSize();
private:
    uint32_t recv_join_size;            //接收申请加入数量
    uint32_t recv_link_size;            //接收申请链接数量

    uint32_t send_synchro_size;         //发送同步数量
    uint32_t recv_synchro_size;         //接收同步数量
    uint32_t send_seqeuence_size;       //发送序号数量

    uint32_t note_normal_size;          //正常数量
    uint32_t note_passive_size;         //被动数量
    uint32_t note_invalid_size;         //主类型无效数量

    std::list<uint32_t> note_rtt_value; //往返时间链表
};

//class HostNoteParameter{
//public:
//    HostNoteParameter() : apply_join_size(0), apply_link_size(0), exit_size(0), determine_join_size(0),
//                          determine_link_size(0), send_sequence_size(0) {}
//    virtual ~HostNoteParameter() = default;
//
//    void onRemoteJoin(uint32_t, uint32_t);
//    void onRemoteLink(uint32_t, uint32_t);
//    void onRemoteSequence(uint32_t);
//    void onRemoteExit();
//
//    void onData(ParameterHost*);
//    void onClear();
//private:
//    uint32_t apply_join_size;                   //申请加入数量
//    uint32_t apply_link_size;                   //申请链接数据
//    uint32_t exit_size;                         //申请退出数量
//
//    std::atomic<uint32_t> determine_join_size;  //确定加入数量
//    std::atomic<uint32_t> determine_link_size;  //确定链接数量
//    std::atomic<uint32_t> send_sequence_size;   //发送序号数量
//};

class AddressParameter{
#define ADDRESS_PARAMETER_DEFAULT_VALUE     1
public:
    AddressParameter() : address_size(0), join_size(0), link_size(0), normal_size(0), transfer_size(0),
                         passive_size(0), exit_size(0), unsupport_size(0), invalid_size(0) {}
    void onInitAddress(){
        address_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release);
    }
    void onJoinAddress() {
        join_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release);
    }
    void onExitAddress(){
        exit_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release);
        address_size.fetch_sub(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release);
    }
    void onLinkAddress() { link_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release); }
    void onNormalAddress() { normal_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release); }
    void onTransferAddress() { transfer_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release); }
    void onPassiveAddress() { passive_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release); }
    void onUnSupport() { unsupport_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release); }
    void onInvalid() { invalid_size.fetch_add(ADDRESS_PARAMETER_DEFAULT_VALUE, std::memory_order_release); }

    uint32_t getSizeAddress() const { return address_size.load(std::memory_order_consume); }
    uint32_t getJoinAddress() const { return join_size.load(std::memory_order_consume); }
    uint32_t getLinkAddress() const { return link_size.load(std::memory_order_consume); }
    uint32_t getNormalAddress() const { return normal_size.load(std::memory_order_consume); }
    uint32_t getTransferAddress() const { return transfer_size.load(std::memory_order_consume); }
    uint32_t getPassiveAddress() const { return passive_size.load(std::memory_order_consume); }
    uint32_t getExitAddress() const { return exit_size.load(std::memory_order_consume); }
    uint32_t getUnSupportSize() const { return unsupport_size.load(std::memory_order_consume); }
    uint32_t getInvalidtSize() const { return invalid_size.load(std::memory_order_consume); }

    void onData(ParameterAddress*);
    void onClear();
private:
    std::atomic<uint32_t> address_size;
    std::atomic<uint32_t> join_size;
    std::atomic<uint32_t> link_size;
    std::atomic<uint32_t> normal_size;
    std::atomic<uint32_t> transfer_size;
    std::atomic<uint32_t> passive_size;
    std::atomic<uint32_t> exit_size;
    std::atomic<uint32_t> unsupport_size;
    std::atomic<uint32_t> invalid_size;
};

#endif //TEXTGDB_ADDRESSPARAMTER_H
