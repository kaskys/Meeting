//
// Created by abc on 20-12-8.
//

#ifndef TEXTGDB_SERVANTCENTRALIZATION_H
#define TEXTGDB_SERVANTCENTRALIZATION_H

#include "CentralizationControl.h"

//中心化远程端发送自己视音频资源时需要构造空TransmitNoteStatus,以便解析
//判断是否启动发送（ServantSynchroUtil）

class ServantCentralization;

/*
 * 中心化远程端同步工具
 */
class ServantSynchroUtil final {
public:
    ServantSynchroUtil() : synchro_flag(false), core_synchro_sequence_gap(0) {}
    ~ServantSynchroUtil() = default;

    bool isServantSynchro() const { return synchro_flag.load(std::memory_order_acquire); }
    void onServantSynchro(MsgHdr *msg) {
        core_synchro_sequence_gap = (*reinterpret_cast<uint32_t*>(msg->buffer) - msg->serial_number);
        synchro_flag.store(true, std::memory_order_release);
    }
    void onClearSynchro(){ synchro_flag.store(false, std::memory_order_release); core_synchro_sequence_gap = 0; }
    uint32_t onServantSequenceGap(uint32_t sequence) const { return (sequence + core_synchro_sequence_gap); }
private:
    std::atomic_bool synchro_flag;          //同步表示（是否执行同步）
    uint32_t core_synchro_sequence_gap;     //主机序号与本机的序号差值（客户机）
};

class ServantCentralization : public CentralizationControl{
public:
    using CentralizationControl::CentralizationControl;
    ~ServantCentralization() override = default;

    bool onCoreControl() const override { return false; }
protected:
    void onLaunchCentralization(MsgHdr*) override;
    void onStopCentralization(MsgHdr*) override;

    void onTransmitMasterInput(TransmitJoin*) override;
    void onTransmitMasterInput(TransmitLink*) override;
    void onTransmitMasterInput(TransmitMedia*) override;
    void onTransmitMasterInput(TransmitExit*) override;

    void onTransmitSharedInput(TransmitSychro*) override {} //暂未实现

    void onTransmitSharedInput(TransmitSequence*) override {} //暂未实现
    void onTransmitSharedInput(TransmitTransfer*) override {} //暂未实现
    void onTransmitSharedInput(TransmitQuery*) override {} //暂未实现
    void onTransmitSharedInput(TransmitText*) override {} //暂未实现
    void onTransmitSharedInput(TransmitError*) override {} //暂未实现

    void onTransmitResponseInput(TransmitRJoin*) override {} //暂未实现
    void onTransmitResponseInput(TransmitRSychro*) override {} //暂未实现
    void onTransmitResponseInput(TransmitRTransfer*) override {} //暂未实现
    void onTransmitResponseInput(TransmitRStatus*) override {} //暂未实现
    void onTransmitResponseInput(TransmitRQuery*) override {} //暂未实现

    void onCentralizationTimeDrive(MsgHdr*) override;

    void onAllocDynamic(MsgHdr*) override;
    void onDeallocDynamic(MsgHdr*) override;
    RemoteNoteInfo* onMatchInputAddress(sockaddr_in) override;
private:
    void onNotePrevent(MsgHdr*, MeetingAddressNote*) override;
    void onNoteDisplay(MsgHdr*, MeetingAddressNote*) override;
    void onNoteLink(MsgHdr*, MeetingAddressNote*) override;
    void onNoteMedia(MsgHdr*, MeetingAddressNote*) override;
    void onNotePassive(MsgHdr*, MeetingAddressNote*) override;
    void onNoteExit(MsgHdr*, MeetingAddressNote*) override;

    void onSequenceGap(MsgHdr*, MeetingAddressNote*) override;
    void onSequenceFrame(MsgHdr*) override;

    bool onTransmitOutputIntercept(MsgHdr*, MeetingAddressNote*);
    bool onTransmitSynchroIntercept(MsgHdr*);
    void onDisplayAnalysis0(TimeSubmitInfo*, DisplaySubmitFunc);
    void onDisplayRelease0(TimeSubmitInfo*);
    void onSequenceFrame0(MediaData*, uint32_t, uint32_t);
    TransmitCodeInfo initTransmitCodeInfo(MediaData*, uint32_t, uint32_t);

    ServantSynchroUtil synchro_util;    //同步工具类
};


#endif //TEXTGDB_SERVANTCENTRALIZATION_H
