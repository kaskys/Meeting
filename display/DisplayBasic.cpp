//
// Created by abc on 21-10-11.
//

#include "DisplayManager.h"

static const char *addr_spot = ".";

DisplayBasic::DisplayBasic() : codec_util(), monitor_util(), camera_util() {
    codec_util.onInit();
    monitor_util.onInit();
    camera_util.onInit();
}

DisplayBasic::~DisplayBasic() {
    codec_util.onUnInit();
    monitor_util.onUnInit();
    camera_util.onUnInit();
}


void DisplayBasic::onNoteStatus(DisplayBuffer *display_buffer) {
    monitor_util.onDisplayInfo(display_buffer);
}

void DisplayBasic::onNoteParameter(DisplayBuffer *display_buffer) {
    monitor_util.onDisplayInfo(display_buffer);
}

void DisplayBasic::onNoteMediaUpdate(DisplayBuffer *display_buffer, DisplayMediaData *media_data) {
    monitor_util.onDisplayMedia(display_buffer, media_data);
}

DisplayMediaData DisplayBasic::onAnalysisDisplayMedia(char *media_buffer, uint32_t media_len) {
    DisplayMediaData media_data = DisplayMediaData(media_len, media_buffer);
    //将字节流解析媒体数据
    media_data.onAnalysis();
    codec_util.onDecode(&media_data);
    return media_data;
}

DisplayMediaData DisplayBasic::onDisplayExtract0(){
    DisplayMediaData media_data = camera_util.onCameraFrame();
    media_data.onCompile();
    codec_util.onCode(&media_data);
    return media_data;
}