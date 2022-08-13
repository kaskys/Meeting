#ifndef SETTINGINFO_H
#define SETTINGINFO_H

#include <QSize>
#include <QString>
#include <QVideoFrame>
#include <QCameraInfo>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QNetworkInterface>
#include <QDebug>

#include <vector>
#include <functional>

class SettingInfo
{
public:
    SettingInfo();
    ~SettingInfo();

    SettingInfo(const SettingInfo&);
    SettingInfo(SettingInfo&&);

    SettingInfo& operator=(const SettingInfo&);
    SettingInfo& operator=(SettingInfo&&);

    void clear();

    void setIsSystem(bool value) { is_system = value; }
    void setInfoFrame(uint32_t value) { info_frame = value; }
    void setInfoDelay(uint32_t value) { info_delay = value; }
    void setInfoTimeout(uint32_t value) { info_timeout = value; }
    void setIpPort(uint32_t value) { ip_port = value; }
    void setIpLinkPort(uint32_t value) { ip_link_port = value; }
    void setIpIndex(uint32_t index) { ip_index = index; }
    void setIpAddr(const QString &value) { ip_addr = value; }
    void setIpLink(const QString &value) { ip_link = value; }
    void setHostIp(const QList<QHostAddress> &ips) {
        host_ips = ips;
    }
    void setHostIp(QList<QHostAddress> &&ips){
        host_ips = std::move(ips);
    }

    bool getIsSystem() const { return is_system; }
    uint32_t getInfoFrame() const { return info_frame; }
    uint32_t getInfoDelay() const { return info_delay; }
    uint32_t getInfoTimeout() const { return info_timeout; }
    uint32_t getIpPort() const { return ip_port; }
    uint32_t getIpLinkPort() const { return ip_link_port; }
    uint32_t getIpIndex() const { return ip_index; }
    const QString& getIpAddr() const { return ip_addr; }
    const QString& getIpLink() const { return ip_link; }
    const QList<QHostAddress>& getHostIps() const { return host_ips; }
private:
    bool is_system;

    uint32_t info_frame;
    uint32_t info_delay;
    uint32_t info_timeout;
    uint32_t ip_port;
    uint32_t ip_link_port;
    uint32_t ip_index;
    QString  ip_addr;
    QString  ip_link;

    QList<QHostAddress> host_ips;
};

class MediaInfo{
public:
    MediaInfo();
    ~MediaInfo();

    MediaInfo(const MediaInfo&);
    MediaInfo(MediaInfo&&);

    MediaInfo& operator=(const MediaInfo&);
    MediaInfo& operator=(MediaInfo&&);

    void clear();

    void setMediaCameraRatio(QSize value) { camera_ratio = value; }
    void setMediaCameraResolution(QSize value) { camera_resolution = value; }
    void setMediaCameraFormat(QVideoFrame::PixelFormat value) { camera_format = value; }

    void setMediaAudioInputChannel(int value, int index) { audio_input_channel = {value, index}; }
    void setMediaAudioOutputChannel(int value, int index) { audio_output_channel = {value, index}; }

    void setMediaAudioInputSampleRate(int value, int index) { audio_input_sample_rate = {value, index}; }
    void setMediaAudioOutputSampleRate(int value, int index) { audio_output_sample_rate = {value, index}; }

    void setMediaAudioInputSampleSize(int value, int index) { audio_input_sample_size = {value, index}; }
    void setMediaAudioOutputSampleSize(int value, int index) { audio_output_sample_size = {value, index}; }

    void setMediaAudioInputCodec(const QString &value, int index) { audio_input_codec = {value, index}; }
    void setMediaAudioOutputCodec(const QString &value, int index) { audio_output_codec = {value, index}; }

    void setMediaCameraInfo(const QCameraInfo &info){
        camera_select_info = info;
    }

    void setMediaCameraInfo(const QList<QCameraInfo> &infos) {
        camera_infos = infos;
        camera_defalut_info = QCameraInfo::defaultCamera();
    }
    void setMediaCameraInfo(QList<QCameraInfo> &&infos) {
        camera_infos = std::move(infos);
        camera_defalut_info = QCameraInfo::defaultCamera();
    }

    void setMediaAudioInputInfo(const QAudioDeviceInfo &info){
        audio_input_select_info = info;
    }
    void setMediaAudioOutputInfo(const QAudioDeviceInfo &info){
        audio_output_select_info = info;
    }

    void setMediaAudioInputInfo(const QList<QAudioDeviceInfo> &infos) {
        audio_input_infos = infos;
        audio_input_defalut_info = QAudioDeviceInfo::defaultInputDevice();
    }
    void setMediaAudioInputInfo(QList<QAudioDeviceInfo> &&infos) {
        audio_input_infos = std::move(infos);
        audio_input_defalut_info = QAudioDeviceInfo::defaultInputDevice();
    }

    void setMediaAudioOutputInfo(const QList<QAudioDeviceInfo> &infos) {
        audio_output_infos = infos;
        audio_output_defalut_info = QAudioDeviceInfo::defaultOutputDevice();
    }
    void setMediaAudioOutputInfo(QList<QAudioDeviceInfo> &&infos) {
        audio_output_infos = std::move(infos);
        audio_output_defalut_info = QAudioDeviceInfo::defaultOutputDevice();
    }

    QSize getMediaCameraRatio() const { return camera_ratio; }
    QSize getMediaCameraResolution() const  { return camera_resolution; }
    QVideoFrame::PixelFormat getMediaCameraFormat() const { return camera_format; }

    std::pair<int, int> getMediaAudioInputChannel() const { return audio_input_channel; }
    std::pair<int, int> getMediaAudioOutputChannel() const { return audio_output_channel; }

    std::pair<int, int> getMediaAudioInputSampleRate() const { return audio_input_sample_rate; }
    std::pair<int, int> getMediaAudioOutputSampleRate() const { return audio_output_sample_rate; }

    std::pair<int, int> getMediaAudioInputSampleSize() const { return audio_input_sample_size; }
    std::pair<int, int> getMediaAudioOutputSampleSize() const { return audio_output_sample_size; }

    const std::pair<QString, int>& getMediaAudioInputCodec() const { return audio_input_codec; }
    const std::pair<QString, int>& getMediaAudioOutputCodec() const { return audio_output_codec; }

    const QCameraInfo& getMediaCameraDefaultInfo() const { return camera_defalut_info; }
    const QCameraInfo& getMediaCameraSelectInfo() const { return camera_select_info; }
    const QAudioDeviceInfo& getMediaAudioInputDefaultInfo() const { return audio_input_defalut_info; }
    const QAudioDeviceInfo& getMediaAudioInputSelectInfo() const { return audio_input_select_info; }
    const QAudioDeviceInfo& getMediaAudioOutputDefaultInfo() const { return audio_output_defalut_info; }
    const QAudioDeviceInfo& getMediaAudioOutputSelectInfo() const { return audio_output_select_info; }

    const QList<QCameraInfo>& getMediaCameraInfo() const { return camera_infos; }
    const QList<QAudioDeviceInfo>& getMediaAudioInputInfo() { return audio_input_infos; }
    const QList<QAudioDeviceInfo>& getMediaAudioOutputInfo() { return audio_output_infos; }

    static const std::vector<QString> camera_format_values;
private:
    QSize camera_ratio;
    QSize camera_resolution;
    QVideoFrame::PixelFormat camera_format;

    std::pair<int, int> audio_input_channel;
    std::pair<int, int> audio_output_channel;

    std::pair<int, int> audio_input_sample_rate;
    std::pair<int, int> audio_output_sample_rate;

    std::pair<int, int> audio_input_sample_size;
    std::pair<int, int> audio_output_sample_size;

    std::pair<QString, int> audio_input_codec;
    std::pair<QString, int> audio_output_codec;

    QCameraInfo camera_defalut_info;
    QCameraInfo camera_select_info;
    QAudioDeviceInfo audio_input_defalut_info;
    QAudioDeviceInfo audio_input_select_info;
    QAudioDeviceInfo audio_output_defalut_info;
    QAudioDeviceInfo audio_output_select_info;

    QList<QCameraInfo> camera_infos;
    QList<QAudioDeviceInfo> audio_input_infos;
    QList<QAudioDeviceInfo> audio_output_infos;
};

class CameraFormatInfo{
public:
    CameraFormatInfo(QVideoFrame::PixelFormat format, int rate, QSize ratio) : camera_format(format), camera_rate(rate),  camera_ratio(ratio) {}
    ~CameraFormatInfo() = default;



     QVideoFrame::PixelFormat getCameraFormat() const { return camera_format; }
     int getCameraFrameRate() const { return camera_rate; }
     QSize getCameraRatio() const { return camera_ratio; }
private:
     QVideoFrame::PixelFormat camera_format;
     int camera_rate;
     QSize camera_ratio;
};

class IpLinkInfo{
public:
    IpLinkInfo(uint32_t frame, uint32_t delay, uint32_t timeout, uint32_t port, uint32_t ip) : info_frame(frame), info_delay(delay), info_timeout(timeout), ip_port(port), ip_link(ip) {}
    ~IpLinkInfo() = default;

    uint32_t getLinkFrame() const { return info_frame; }
    uint32_t getLinkDelay() const { return info_delay; }
    uint32_t getLinkTimeout() const { return info_timeout; }
    uint32_t getLinkPort() const { return ip_port; }
    uint32_t getLinkIp() const { return ip_link; }
private:
    uint32_t info_frame;
    uint32_t info_delay;
    uint32_t info_timeout;
    uint32_t ip_port;
    uint32_t ip_link;
};

#endif // SETTINGINFO_H
