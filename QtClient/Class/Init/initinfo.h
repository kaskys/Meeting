#ifndef INITINFO_H
#define INITINFO_H

#include "../Setting/settinginfo.h"
#include "../Adapter/filteradapter.h"

class InitInfo
{
public:
    InitInfo() = default;
    ~InitInfo() = default;

    static void onInitNet(SettingInfo *info) {
        initNetDeviceInfo(info);
    }
    static void onInitMedia(MediaInfo *info) {
        initCameraDeviceInfo(info);
        initAudioInputDeviceInfo(info);
        initAudioOutputDeviceInfo(info);
    }
    static void updateNetDeviceInfo(SettingInfo *info){
        initNetDeviceInfo(info);
    }
    static void updateDefalutAudioInputDeviceInfo(MediaInfo *info){
        initAudioInputDeviceInfo(info);
    }
    static void updateDefalutAudioOutputDeviceInfo(MediaInfo *info){
        initAudioOutputDeviceInfo(info);
    }

    static FilterAdapter* onInitFilter(){
        return initFilterInfo();
    }
private:
    static void initNetDeviceInfo(SettingInfo*);
    static void initCameraDeviceInfo(MediaInfo*);
    static void initAudioInputDeviceInfo(MediaInfo*);
    static void initAudioOutputDeviceInfo(MediaInfo*);

    static FilterAdapter* initFilterInfo();
};

#endif // INITINFO_H
