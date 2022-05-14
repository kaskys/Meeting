//
// Created by abc on 20-4-5.
//

#ifndef UNTITLED8_MEETINGADDRESSNOTE_H
#define UNTITLED8_MEETINGADDRESSNOTE_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <atomic>

#define ADDRESS_SIZE_MAX        32

#define NOTE_LEAF               false
#define NOTE_NOTE               true

struct MeetingAddressNote;

struct MeetingAddressNote{
    MeetingAddressNote() = default;
    virtual ~MeetingAddressNote() = default;

    bool man_b;  // 0 : leaf / 1 : note
    union {
        //man_b = 0;
        struct {
            uint16_t man_port;                     //端口
            uint32_t man_key;                      //地址
            uint32_t man_use;                      //地址使用值
            uint32_t man_permit;                   //地址许可序号(用于SocketThread类)
            uint32_t man_global_position;          //地址全局序号(全部远程端的序号)
            uint32_t man_transfer_position;        //地址执行层ExecutorTransferInfo的序号（转移数组序号）
        } man_leaf;
        //man_b = 1;
        struct {
            int man_off;                           //节点值->(位置)
            uint32_t man_size;                     //左子节点和右子节点的leaf数量
            MeetingAddressNote *man_left;          //左子节点
            MeetingAddressNote *man_right;         //右子节点
        } man_note;
    } man_u;
    MeetingAddressNote *man_p;               //父节点
};


#define man_key                 man_u.man_leaf.man_key
#define man_port                man_u.man_leaf.man_port
#define man_use                 man_u.man_leaf.man_use
#define man_permit              man_u.man_leaf.man_permit
#define man_global_position     man_u.man_leaf.man_global_position
#define man_transfer_position   man_u.man_leaf.man_transfer_position

#define man_left                man_u.man_note.man_left
#define man_right               man_u.man_note.man_right
#define man_off                 man_u.man_note.man_off
#define man_size                man_u.man_note.man_size

typedef struct MeetingAddressNote MeetingAddressNote;

/**
 * 插入成功异常
 */
class PushFinal final : public std::exception{
public:
    using exception::exception;
    ~PushFinal() override = default;
    const char* what() const noexcept override { return nullptr; }
};

/**
 * 插入信息
 */
class PushInfo final{
public:
    explicit PushInfo(uint32_t caddr, MeetingAddressNote *pinfo)
            : transfer_note(true), next_off(0), complate_addr(caddr), note_info(pinfo), out_note(nullptr) {}
    ~PushInfo() = default;

    static bool getAddrBit(uint32_t addr, int off_set) { return (addr & (1 << (ADDRESS_SIZE_MAX - off_set - 1))); }

    bool transfer_note;             //是否旋转Note节点(默认true)
    int next_off;
    uint32_t complate_addr;         //与插入的leaf节点地址信息比较的ip地址信息
    MeetingAddressNote *note_info;  //插入的leaf节点
    MeetingAddressNote **out_note;  //插入的leaf节点在parent的左或右子节点信息[(parent->man_right == child) -->  &parent->man_right 或 &parent->man_left]
};

/**
 * 搜索信息
 */
struct SearchInfo final{
    int start_off;                          //开始搜索的偏移点
    MeetingAddressNote *parent;             //父类信息
    MeetingAddressNote *transfer_leaf;      //需要转移的leaf节点
    MeetingAddressNote *compare_key;        //与转移的leaf节点比较的地址信息
    MeetingAddressNote **out_note;          //child在parent的左或右子节点信息[(parent->man_right == child) -->  &parent->man_right 或 &parent->man_left]
};

#endif //UNTITLED8_MEETINGADDRESSNOTE_H
