//
// Created by abc on 21-10-7.
//

#ifndef TEXTGDB_DISPLAYLAYER_H
#define TEXTGDB_DISPLAYLAYER_H

#include "../BasicLayer.h"
#include "../../display/DisplayManager.h"

#define DISPLAY_LAYER_DISPLAY_UPDATE                10
#define DISPLAY_LAYER_DISPLAY_MEDIA                 11

#define DISPLAY_LAYER_CONTROL_THREAD                10
#define DISPLAY_LAYER_CONTROL_PARAMETER             11

#define CONTROL_NOTE_STATUS_DISPLAY_DIFFERENCE      5

class DisplayLayer;
class DisplayThreadUtil;

struct MsgHdr;
using DisplayReleaseFunc = std::function<void()>;
using DisplayMediaFunc = std::function<void(void(*)(DisplayLayer*, NoteMediaInfo*, uint32_t))>;

/*
 * 显示远程端信息
 */
struct DisplayNoteInfo final{
    explicit DisplayNoteInfo(DisplayStatus status, MeetingAddressNote *note) : note_status(status), note_info(note){}
    DisplayStatus note_status;      //显示远程端状态
    MeetingAddressNote *note_info;  //远程端信息
};

/*
 * 显示视音频资源信息
 */
struct DisplayMediaInfo final{
    explicit DisplayMediaInfo(DisplayReleaseFunc rfunc, DisplayMediaFunc mfunc)
                                                       : release_func(std::move(rfunc)), media_func(std::move(mfunc)) {}

    DisplayMediaFunc media_func;        //资源显示函数
    DisplayReleaseFunc release_func;    //资源释放函数
};

/*
 * 显示层参数
 */
class DisplayLayerParameter final {
public:
    void onClear(){
        note_update_size = 0; media_drive_size = 0;
        media_display_size = 0;
    }

    void onNoteUpdate() { note_update_size++; }
    void onDisplayDrive() { media_drive_size++; }
    void onDisplayMedia() { media_display_size++; }

    void onParameter(DisplayLayerParameter *parameter) const {
        parameter->note_update_size = note_update_size;
        parameter->media_drive_size = media_drive_size;
        parameter->media_display_size = media_display_size;
    }
private:
    uint32_t note_update_size;      //远程端更新数量
    uint32_t media_drive_size;      //视音频资源更新数量
    uint32_t media_display_size;    //视音频资源显示数量
};

class DisplayLayerUtil : public LayerUtil{
public:
    BasicLayer* createLayer(BasicControl*) noexcept override;
    void destroyLayer(BasicControl*, BasicLayer*) noexcept override;
};

class DisplayLayer : public BasicLayer{
public:
    using BasicLayer::BasicLayer;
    ~DisplayLayer() override = default;

    void initLayer() override;
    void onInput(MsgHdr*) override;
    void onOutput() override;
    bool isDrive() const override { return true; }
    void onDrive(MsgHdr*) override;
    void onParameter(MsgHdr*) override;
    void onControl(MsgHdr*) override;
    uint32_t onStartLayerLen() const override { return 0; }
    LayerType onLayerType() const override { return LAYER_DISPLAY_TYPE; }

    static DisplayStatus onConvertStatus(short type) { return DisplayStatus(type - CONTROL_NOTE_STATUS_DISPLAY_DIFFERENCE); }
private:
    char* onCreateDisplayBuffer(int);
    void onDestroyDisplayBuffer(char*, int);

    bool onStartDisplayLayer(MsgHdr*);
    void onStopDisplayLayer(MsgHdr*);
    void onDisplayThrow();

    void onDisplayDrive(char*, uint32_t, uint32_t, const std::function<void(char*,int)>&);
    void onDisplayUpdate(DisplayNoteInfo*);
    void onDisplayUpdate0(DisplayStatus, uint32_t, uint32_t);
    void onDisplayMedia(DisplayMediaInfo*);
    void onDisplayMedia0(NoteMediaInfo*, uint32_t) throw(std::bad_alloc);

    void onDisplayParameter(MsgHdr*);
    void onDisplayCorrelateThread(MsgHdr*);

    static DisplayLayerParameter parameter;     //地址层参数类

    DisplayManager *manager;                    //显示层管理类
    DisplayThreadUtil *thread_util;             //显示层关联线程工具类
};

/*
 * 显示层关联线程工具
 */
class DisplayThreadUtil final{
public:
    explicit DisplayThreadUtil(DisplayLayer *layer) : display_layer(layer), display_thread(nullptr){}

    bool isCorrelate() const { return static_cast<bool>(display_thread); }
    void onCorrelateThread(DisplayWordThread *thread) { display_thread = thread; }
    DisplayWordThread* onCorrrelateThread() const { return display_thread; }

    void onRunCorrelateThreadImmediate(const std::function<void()>&);
private:
    DisplayLayer *display_layer;        //显示层
    DisplayWordThread *display_thread;  //Display线程
};

#endif //TEXTGDB_DISPLAYLAYER_H
