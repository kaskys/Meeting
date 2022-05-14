//
// Created by abc on 20-12-8.
//

#ifndef TEXTGDB_MASTERCENTRALIZATION_H
#define TEXTGDB_MASTERCENTRALIZATION_H

#include "CentralizationControl.h"

class MasterCentralization : public CentralizationControl {
    using MediaAnalysisFunc = std::function<void(MsgHdr*)>;
public:
    using CentralizationControl::CentralizationControl;
    ~MasterCentralization() override = default;

    bool onCoreControl() const override { return true; }
protected:
    void onTransmitMasterInput(TransmitJoin*) override;
    void onTransmitMasterInput(TransmitLink*) override;
    void onTransmitMasterInput(TransmitMedia*) override;
    void onTransmitMasterInput(TransmitExit*) override;

    void onTransmitSharedInput(TransmitSychro*) override;
    void onTransmitSharedInput(TransmitSequence*) override;
    void onTransmitSharedInput(TransmitTransfer*) override;
    void onTransmitSharedInput(TransmitQuery*) override;
    void onTransmitSharedInput(TransmitText*) override;
    void onTransmitSharedInput(TransmitError*) override;

    void onTransmitResponseInput(TransmitRJoin*) override;
    void onTransmitResponseInput(TransmitRSychro*) override;
    void onTransmitResponseInput(TransmitRTransfer*) override;
    void onTransmitResponseInput(TransmitRStatus*) override;
    void onTransmitResponseInput(TransmitRQuery*) override;

    void onCentralizationTimeDrive(MsgHdr*) override;

    void onLaunchCentralization(MsgHdr*) override;
    void onStopCentralization(MsgHdr*) override;
    void onAllocDynamic(MsgHdr*) override;
    void onDeallocDynamic(MsgHdr*) override;
    RemoteNoteInfo* onMatchInputAddress(sockaddr_in) override;
private:
    static ExecutorNoteMediaInfo* initExecutorNoteInfo(uint32_t nsize, uint32_t isize, uint32_t sequence, uint32_t timeout,
                                                       MasterCentralization *control, TimeSubmitInfo *submit_info,
                                                       const std::function<void(const std::function<void(MeetingAddressNote*,uint32_t)>&)> &note_func){
        size_t mlen = (sizeof(MeetingAddressNote*) * nsize) + (sizeof(TimeInfo*) * isize);
        ExecutorNoteMediaInfo *note_media_info = new (mlen, std::nothrow) ExecutorNoteMediaInfo(nsize, isize, sequence, timeout);
        if(!note_media_info) { return nullptr; }
                              //最后运行该函数（3）
        return note_media_info->setSubmitFunc(std::bind(submit_info->submit_func, getControlLayer<TimeLayer>(control->layer_array, LAYER_TIME_TYPE),
                                                        std::placeholders::_1))
                              //再运行该函数（2）
                              ->setTransmitFunc(std::bind(&MasterCentralization::onNoteTransmit0, control, std::placeholders::_1, std::placeholders::_2,
                                                          std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6))
                              //先运行该函数（1）
                              ->setExecutorFunc(std::bind(&MasterCentralization::onNoteExecutor0, control, std::placeholders::_1, std::placeholders::_2))
                              ->initNoteInfo(note_func, submit_info->submit_info_array);
    };

    static uint32_t onCalculateMediaMsgLen(uint32_t msg_len, uint32_t note_size){
        return sizeof(MsgHdr) + std::max(((sizeof(short) * note_size) + (sizeof(TransmitNoteStatus) * note_size) + msg_len), sizeof(TransmitCodeInfo));
    }

    void onNotePrevent(MsgHdr*, MeetingAddressNote*) override;
    void onNoteDisplay(MsgHdr*, MeetingAddressNote*) override;
    void onNoteLink(MsgHdr*, MeetingAddressNote*) override;
    void onNoteMedia(MsgHdr*, MeetingAddressNote*) override;
    void onNotePassive(MsgHdr*, MeetingAddressNote*) override;
    void onNoteExit(MsgHdr*, MeetingAddressNote*) override;

    void onSequenceGap(MsgHdr*, MeetingAddressNote*) override;
    void onSequenceFrame(MsgHdr*) override;

    bool onTransmitSynchroIntercept(MsgHdr*);
    bool onTransmitOutputIntercept(MsgHdr*, MeetingAddressNote*);

    void onNoteTransmit0(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*, uint32_t, uint32_t, uint32_t);
    void onNoteExecutor0(ExecutorNoteInfo*, const MediaAnalysisFunc&);
    void onMediaAnalysis0(ExecutorNoteInfo*, const MediaAnalysisFunc&);
    void onDisplayAnalysis0(TimeSubmitInfo*, DisplaySubmitFunc);
    void onGenerateMediaMsg(MsgHdr*, NoteMediaInfo2*, TransmitMemoryFixedUtil&, uint32_t sequence);

    void onMasterOutput(ControlDriveInfo*, uint32_t);

    //生命周期-启动前需要从配置文件获取信息
    //默认序号信息
    static SequenceData default_sequence_data;
};

#endif //TEXTGDB_MASTERCENTRALIZATION_H
