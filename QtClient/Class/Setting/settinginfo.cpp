#include "settinginfo.h"

const std::vector<QString> MediaInfo::camera_format_values{"ARGB32", "ARGB_32_Premultiplied", "RGB32", "RGB24", "RGB565", "RGB555", "ARGB8565_Premultiplied",
                                                           "BGRA32", "BGRA32_Premultiplied", "BGR32", "BGR24", "BGR565", "BGR555", "BGRA5658_Premultiplied",
                                                           "AYUV444", "AYUV444_Premultiplied", "YUV444", "YUV420P", "YV12", "UYVY", "YUYV", "NV12",
                                                           "IMC1", "IMC2", "IMC3", "IMC4", "Y8", "Y16", "JPEG", "CAMERARAW", "ADOBEDNG",  "USER"};

SettingInfo::SettingInfo() : is_system(false), info_frame(30), info_delay(0), info_timeout(0), ip_port(8080), ip_link_port(8080), ip_index(0), ip_addr(), ip_link()
{

}

SettingInfo::~SettingInfo()
{

}

SettingInfo::SettingInfo(const SettingInfo &info) : is_system(info.is_system), info_frame(info.info_frame), info_delay(info.info_delay), info_timeout(info.info_timeout),
                                                    ip_port(info.ip_port), ip_link_port(info.ip_link_port), ip_index(info.ip_index), ip_addr(info.ip_addr), ip_link(info.ip_link)
{

}

SettingInfo::SettingInfo(SettingInfo && info) : is_system(info.is_system), info_frame(info.info_frame), info_delay(info.info_delay), info_timeout(info.info_timeout),
                                                ip_port(info.ip_port), ip_link_port(info.ip_link_port), ip_index(info.ip_index), ip_addr(std::move(info.ip_addr)), ip_link(std::move(info.ip_link))
{

}

SettingInfo& SettingInfo::operator=(const SettingInfo &info)
{
    is_system = info.is_system;
    info_frame = info.info_frame;
    info_delay = info.info_delay;
    info_timeout = info.info_timeout;

    ip_port = info.ip_port;
    ip_link_port = info.ip_link_port;
    ip_index = info.ip_index;
    ip_addr = info.ip_addr;
    ip_link = info.ip_link;
    return *this;
}

SettingInfo& SettingInfo::operator=(SettingInfo &&info)
{
    is_system = info.is_system;
    info_frame = info.info_frame;
    info_delay = info.info_delay;
    info_timeout = info.info_timeout;

    ip_port = info.ip_port;
    ip_link_port = info.ip_link_port;
    ip_index = info.ip_index;
    ip_addr = std::move(info.ip_addr);
    ip_link = std::move(info.ip_link);

    info = SettingInfo{};
    return *this;
}

void SettingInfo::clear()
{
    is_system = false;
    info_frame = 10;
    info_delay = 0;
    info_timeout = 0;
    ip_port = 8080;
    ip_link_port = 8080;
    ip_index = 0;
    ip_addr.clear();
    ip_link.clear();
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------//

MediaInfo::MediaInfo() : camera_ratio(300, 300), camera_resolution(0, 0), camera_format(QVideoFrame::PixelFormat::Format_YUYV),
                         audio_input_channel(0, 0), audio_output_channel(0, 0), audio_input_sample_rate(0, 0), audio_output_sample_rate(0, 0),
                         audio_input_sample_size(0, 0), audio_output_sample_size(0, 0), audio_input_codec(QString(), 0), audio_output_codec(QString(), 0),
                         camera_defalut_info(), camera_select_info(),
                         audio_input_defalut_info(), audio_input_select_info(), audio_output_defalut_info(), audio_output_select_info(),
                         camera_infos(), audio_input_infos(), audio_output_infos()
{

}

MediaInfo::~MediaInfo()
{

}

MediaInfo::MediaInfo(const MediaInfo &info) : camera_ratio(info.camera_ratio), camera_resolution(info.camera_resolution), camera_format(info.camera_format),
                                              audio_input_channel(info.audio_input_channel), audio_output_channel(info.audio_output_channel),
                                              audio_input_sample_rate(info.audio_input_sample_rate), audio_output_sample_rate(info.audio_output_sample_rate),
                                              audio_input_sample_size(info.audio_input_sample_size), audio_output_sample_size(info.audio_output_sample_size),
                                              audio_input_codec(info.audio_input_codec), audio_output_codec(info.audio_output_codec),
                                              camera_defalut_info(), camera_select_info(info.camera_select_info),
                                              audio_input_defalut_info(), audio_input_select_info(info.audio_input_select_info),
                                              audio_output_defalut_info(), audio_output_select_info(info.audio_output_select_info),
                                              camera_infos(info.camera_infos), audio_input_infos(info.audio_input_infos), audio_output_infos(info.audio_output_infos)
{
    camera_defalut_info = QCameraInfo::defaultCamera();
    audio_input_defalut_info = QAudioDeviceInfo::defaultInputDevice();
    audio_output_defalut_info = QAudioDeviceInfo::defaultOutputDevice();
}

MediaInfo::MediaInfo(MediaInfo &&info) : camera_ratio(info.camera_ratio), camera_resolution(info.camera_resolution), camera_format(info.camera_format),
                                         audio_input_channel(info.audio_input_channel), audio_output_channel(info.audio_output_channel),
                                         audio_input_sample_rate(info.audio_input_sample_rate), audio_output_sample_rate(info.audio_output_sample_rate),
                                         audio_input_sample_size(info.audio_input_sample_size), audio_output_sample_size(info.audio_output_sample_size),
                                         audio_input_codec(std::move(info.audio_input_codec)), audio_output_codec(std::move(info.audio_output_codec)),
                                         camera_infos(std::move(info.camera_infos)), camera_select_info(std::move(info.camera_select_info)),
                                         audio_input_select_info(std::move(info.audio_input_select_info)), audio_output_select_info(std::move(info.audio_output_select_info)),
                                         audio_input_infos(std::move(info.audio_input_infos)), audio_output_infos(std::move(info.audio_output_infos))
{
    camera_defalut_info = QCameraInfo::defaultCamera();
    audio_input_defalut_info = QAudioDeviceInfo::defaultInputDevice();
    audio_output_defalut_info = QAudioDeviceInfo::defaultOutputDevice();
}

MediaInfo& MediaInfo::operator=(const MediaInfo &info)
{
    camera_ratio = info.camera_ratio;  camera_resolution = info.camera_resolution; camera_format = info.camera_format;
    audio_input_channel = info.audio_input_channel; audio_output_channel = info.audio_output_channel;
    audio_input_sample_rate = info.audio_input_sample_rate; audio_output_sample_rate = info.audio_output_sample_rate;
    audio_input_sample_size = info.audio_input_sample_size; audio_output_sample_size = info.audio_output_sample_size;
    audio_input_codec = info.audio_input_codec; audio_output_codec = info.audio_output_codec;
    camera_defalut_info = QCameraInfo::defaultCamera(); audio_input_defalut_info = QAudioDeviceInfo::defaultInputDevice(); audio_output_defalut_info = QAudioDeviceInfo::defaultOutputDevice();
    camera_select_info = info.camera_select_info; audio_input_select_info = info.audio_input_select_info; audio_output_select_info = info.audio_output_select_info;
    camera_infos = info.camera_infos;
    audio_input_infos = info.audio_input_infos; audio_output_infos = info.audio_output_infos;

    return *this;
}

MediaInfo& MediaInfo::operator=(MediaInfo &&info)
{
    camera_ratio = info.camera_ratio;  camera_resolution = info.camera_resolution; camera_format = info.camera_format;
    audio_input_channel = info.audio_input_channel; audio_output_channel = info.audio_output_channel;
    audio_input_sample_rate = info.audio_input_sample_rate; audio_output_sample_rate = info.audio_output_sample_rate;
    audio_input_sample_size = info.audio_input_sample_size; audio_output_sample_size = info.audio_output_sample_size;
    audio_input_codec = std::move(info.audio_input_codec); audio_output_codec = std::move(info.audio_output_codec);
    camera_defalut_info = QCameraInfo::defaultCamera(); audio_input_defalut_info = QAudioDeviceInfo::defaultInputDevice(); audio_output_defalut_info = QAudioDeviceInfo::defaultOutputDevice();
    camera_select_info = std::move(info.camera_select_info); audio_input_select_info = std::move(info.audio_input_select_info); audio_output_select_info = std::move(info.audio_output_select_info);
    camera_infos = std::move(info.camera_infos);
    audio_input_infos = std::move(info.audio_input_infos); audio_output_infos = std::move(info.audio_output_infos);

    return *this;

}

void MediaInfo::clear()
{
    camera_ratio = QSize(300, 300);
    camera_resolution = QSize(0, 0);
    camera_format = QVideoFrame::PixelFormat::Format_YUYV;
    audio_input_channel = std::pair<int, int>(0, 0);
    audio_output_channel = std::pair<int, int>(0, 0);
    audio_input_sample_rate = std::pair<int, int>(0, 0);
    audio_output_sample_rate = std::pair<int, int>(0, 0);
    audio_input_sample_size = std::pair<int, int>(0, 0);
    audio_output_sample_size = std::pair<int, int>(0, 0);
    audio_input_codec.first.clear(); audio_input_codec.second = 0;
    audio_output_codec.first.clear(); audio_output_codec.second = 0;
    camera_select_info = camera_defalut_info;
    audio_input_select_info = audio_input_defalut_info;
    audio_output_select_info = audio_output_defalut_info;
}
