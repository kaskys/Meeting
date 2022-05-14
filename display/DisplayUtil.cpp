//
// Created by abc on 21-10-13.
//

#include "DisplayManager.h"

void DisplayCodecUtil::onInit() throw(std::runtime_error) {

}

void DisplayCodecUtil::onUnInit() {

}

void DisplayCodecUtil::onCode(DisplayMediaData *media_data) {

}

void DisplayCodecUtil::onDecode(DisplayMediaData *media_data) {

}

//--------------------------------------------------------------------------------------------------------------------//

void DisplayMonitorUtil::onInit() throw(std::runtime_error) {

}

void DisplayMonitorUtil::onUnInit() {

}

void DisplayMonitorUtil::onDisplayInfo(DisplayBuffer *display_buffer) {

}

void DisplayMonitorUtil::onDisplayMedia(DisplayBuffer *display_buffer, DisplayMediaData *media_data) {

}

//--------------------------------------------------------------------------------------------------------------------//

void DisplayCameraUtil::onInit() throw(std::runtime_error) {

}

void DisplayCameraUtil::onUnInit() {

}

DisplayMediaData DisplayCameraUtil::onCameraFrame() {

}