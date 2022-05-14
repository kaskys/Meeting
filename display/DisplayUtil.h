//
// Created by abc on 21-10-13.
//

#ifndef TEXTGDB_DISPLAYUTIL_H
#define TEXTGDB_DISPLAYUTIL_H

#include <new>
#include <map>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <functional>

class DisplayBuffer;
struct DisplayMediaData;
struct DisplayParameter;


/**
 * 显示编解码器
 */
class DisplayCodecUtil final{
public:
    DisplayCodecUtil() = default;
    ~DisplayCodecUtil() = default;

    void onInit() throw(std::runtime_error);
    void onUnInit();

    void onCode(DisplayMediaData*);
    void onDecode(DisplayMediaData*);
private:

};

/**
 * 显示器
 */
class DisplayMonitorUtil final{
public:
    DisplayMonitorUtil() = default;
    ~DisplayMonitorUtil() = default;

    void onInit() throw(std::runtime_error);
    void onUnInit();

    void onDisplayInfo(DisplayBuffer*);
    void onDisplayMedia(DisplayBuffer*, DisplayMediaData*);
private:

};

/**
 * 摄像头
 */
class DisplayCameraUtil final{
public:
    DisplayCameraUtil() = default;
    ~DisplayCameraUtil() = default;

    void onInit() throw(std::runtime_error);
    void onUnInit();

    DisplayMediaData onCameraFrame();
private:

};



#endif //TEXTGDB_DISPLAYUTIL_H
