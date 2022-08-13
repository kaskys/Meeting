#include "initinfo.h"

void InitInfo::initNetDeviceInfo(SettingInfo *info)
{
    QList<QHostAddress> ips = QNetworkInterface::allAddresses();
    for(int i = 0; i < ips.size();){
        QHostAddress value = ips.at(i);
        if(!((value.protocol() == QAbstractSocket::IPv4Protocol) && !value.isLoopback())){
            ips.removeAt(i);
        }else{
            ++i;
        }
    }
    info->setHostIp(std::move(ips));
}

void InitInfo::initCameraDeviceInfo(MediaInfo *info)
{
    info->setMediaCameraInfo(QCameraInfo::availableCameras());
}

void InitInfo::initAudioInputDeviceInfo(MediaInfo *info)
{
    info->setMediaAudioInputInfo(QAudioDeviceInfo::availableDevices(QAudio::AudioInput));
}

void InitInfo::initAudioOutputDeviceInfo(MediaInfo *info)
{
    info->setMediaAudioOutputInfo(QAudioDeviceInfo::availableDevices(QAudio::AudioOutput));
}

FilterAdapter *InitInfo::initFilterInfo()
{

}

