#include "mediacontrol.h"

void AudioBasic::updateVolumeFunc(const VolumeFunc &func)
{
    if(io_control) io_control->updateVolumeFunc(func);
}

IOControl* AudioBasic::initAudioDevice(IOControl *ndevice, const AudioDeviceFunc &dfunc)
{
    IOControl *odevice = io_control;
    io_control = ndevice;
    if(dfunc){
        io_control->connectReadCallback(dfunc);
    }
    return odevice;
}

bool AudioBasic::onOpenDevice(QIODevice::OpenMode flag)
{
    if(!(flag & QIODevice::ReadWrite)){
        flag |= QIODevice::ReadWrite;
    }
    return io_control->open(flag);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------//

bool MediaControl::InputAudioControl::onInitAudio(const QAudioDeviceInfo &info, QAudioFormat format, const CompleteFunc &cfunc, const VolumeFunc &vfunc, const AudioDeviceFunc &dfunc)
{
    if((audio_input = new (std::nothrow) QAudioInput(info, format)) && onInitDevice(info, format, vfunc, dfunc) && onOpenDevice(QIODevice::ReadWrite)){
        return onInitComplete(cfunc);
    }else{
        if(audio_input){
            delete audio_input; audio_input = nullptr;
        }
        return false;
    }
}

bool MediaControl::InputAudioControl::onUpdateAudio(const QAudioDeviceInfo &info, QAudioFormat format, const CompleteFunc &cfunc, const VolumeFunc &vfunc, const AudioDeviceFunc &dfunc)
{
    if(!info.isFormatSupported(format)) return false;

    QAudioInput *ainput = audio_input;   
    if(!(audio_input = new (std::nothrow) QAudioInput(info, format))){
        return false;
    }
    if(!onResetDevice(info, format, vfunc, dfunc)){
        delete audio_input;
        audio_input = ainput;
        return false;
    }else{
        delete ainput;
    }

    return onInitComplete(cfunc);
}

void MediaControl::InputAudioControl::onAudioStart()
{
    if(audio_input && io_control){
        audio_input->start(dynamic_cast<QIODevice*>(io_control));
    }
}

void MediaControl::InputAudioControl::onAudioStop()
{
    if(audio_input){
        audio_input->stop();
        io_control->clear();
    }
}

bool MediaControl::InputAudioControl::onInitDevice(const QAudioDeviceInfo &info, QAudioFormat format, const VolumeFunc &vfunc, const AudioDeviceFunc &dfunc)
{
    IOControl *device = new (std::nothrow) AudioInputDevice(format, vfunc);

    if(device){
        this->audio_format = format; this->info = info;

        if(device = initAudioDevice(device, dfunc)){
            device->close(); delete device;
        }
        return true;
    }else{
        return false;
    }
}

bool MediaControl::InputAudioControl::onResetDevice(const QAudioDeviceInfo &info, QAudioFormat format, const VolumeFunc &vfunc, const AudioDeviceFunc &dfunc)
{
    if(io_control->isOpen()){
        if(io_control->updateAudioFormat(format)){
            this->audio_format = format; this->info = info;
            return true;
        }else{
            return false;
        }
    }else{
        if(onOpenDevice(QIODevice::ReadWrite)){
            this->audio_format = format; this->info = info;
            return true;
        }else{
            return false;
        }
    }
}

bool MediaControl::OutputAudioControl::onInitAudio(const QAudioDeviceInfo &info, QAudioFormat format, const CompleteFunc &cfunc, const VolumeFunc &vfunc, const AudioDeviceFunc&)
{
    if((audio_output = new (std::nothrow) QAudioOutput(info, format)) && onInitDevice(info, format, vfunc, nullptr) && onOpenDevice(QIODevice::ReadWrite)){
        return onInitComplete(cfunc);
    }else{
        if(audio_output){
            delete audio_output; audio_output = nullptr;
        }
        return false;
    }
}

void MediaControl::OutputAudioControl::onAudioStart()
{
    if(audio_output && io_control){
        audio_output->setBufferSize(audio_format.bytesForDuration(1000 * 1000));
        audio_output->start(dynamic_cast<QIODevice*>(io_control));
//        odevice = audio_output->start();
    }
}

void MediaControl::OutputAudioControl::onAudioStop()
{
    if(audio_output){
        audio_output->stop();
        io_control->clear();
    }
}

bool MediaControl::OutputAudioControl::onInitDevice(const QAudioDeviceInfo &info, QAudioFormat format, const VolumeFunc &vfunc, const AudioDeviceFunc &dfunc)
{
    this->audio_format = format; this->info = info;
    IOControl *device = new (std::nothrow) AudioOutputDevice(format, vfunc);

    if(device){
        if(device = initAudioDevice(device, dfunc)){
            delete device;
        }
        return true;
    }else{
        return false;
    }
}

bool MediaControl::VideoControl::onInitCamera(const QCameraInfo &info, const CameraFormatInfo &format, LocalVideoWidget *vwidget, const VideoFrameFunc &vfunc)
{
    if(!(camera = new (std::nothrow) QCamera(info)) && onFitCameraInfo(format)) return false;
    this->info = info;
    this->video_callback = vfunc;
    onInitViewWiget(vwidget);
    probe.setSource((QMediaRecorder*)nullptr);
    if(conn_callback){
        QObject::disconnect(conn_callback);
    }

    return true;
}

LocalVideoError MediaControl::VideoControl::onUpdateCamera(const QCameraInfo &info, const CameraFormatInfo &format_info, LocalVideoWidget *vwidget, const VideoFrameFunc &vfunc)
{
    ocamera = camera;
    camera = new (std::nothrow) QCamera(info);

    if(!camera){
        camera = ocamera; ocamera = nullptr;
        return LOCAL_VIDEO_ERROR_INIT;
    }

    if(!onFitCameraInfo(format_info)){
        return LOCAL_VIDEO_ERROR_INVALID_PARAMETER;
    }

    this->info = info;
    this->video_callback = vfunc;
    onInitViewWiget(vwidget);
    probe.setSource((QMediaRecorder*)nullptr);
    if(conn_callback){
        QObject::disconnect(conn_callback);
    }
    return LOCAL_VIDEO_ERROR_NOT;
}

void MediaControl::VideoControl::onCameraLoad(bool s)
{
    video_widget->setVideoWidgetImmediately(s);
    camera->setViewfinder(video_widget);
    camera->load();
}

void MediaControl::VideoControl::onCameraStart()
{
    probe.setSource(camera);
    camera->start();
    if(video_callback){
        conn_callback = QObject::connect(&probe, &QVideoProbe::videoFrameProbed, [=](const QVideoFrame &frame) -> void { video_callback(frame); });
    }
    is_start = true;
}

void MediaControl::VideoControl::onCameraStop()
{
    video_widget->setVideoWidgetImmediately(false);
    probe.setSource((QMediaRecorder*)nullptr);
    if(!conn_callback){
        QObject::disconnect(conn_callback);
    }
    camera->stop();
    is_start = false;
}

void MediaControl::VideoControl::setVideoWidget(QVideoWidget *widget)
{
    camera->setViewfinder(widget);
    releaseOldCamera();
}

void *MediaControl::VideoControl::releaseCamera(QCamera *camera)
{
    if(camera){
        camera->setViewfinder((QVideoWidget*)nullptr);
            if(camera->state() == QCamera::ActiveState){
                camera->stop();
            }
        delete camera;
    }
    return nullptr;
}

bool MediaControl::VideoControl::onFitCameraInfo(const CameraFormatInfo &format_info)
{
    QVideoFrame::PixelFormat format = format_info.getCameraFormat();
    int framt_rate = format_info.getCameraFrameRate();
    QSize ratio = format_info.getCameraRatio();
    QCameraViewfinderSettings camera_setting;

    camera_setting = camera->viewfinderSettings();
    camera_setting.setMinimumFrameRate(framt_rate);
    camera_setting.setMaximumFrameRate(framt_rate);
    camera_setting.setPixelAspectRatio(ratio);
    camera_setting.setPixelFormat(format);

    camera->setViewfinderSettings(camera_setting);
    camera_setting = camera->viewfinderSettings();
    if((framt_rate != camera_setting.minimumFrameRate()) || (framt_rate != camera_setting.maximumFrameRate()) ||
       (ratio != camera_setting.pixelAspectRatio()) || (format != camera_setting.pixelFormat())){
        delete camera; camera = ocamera; ocamera = nullptr;
        return false;
    }
    return true;
}

void MediaControl::VideoControl::onInitViewWiget(LocalVideoWidget *w)
{
    this->video_widget = w;
    QObject::connect(camera, &QCamera::stateChanged, video_widget, &LocalVideoWidget::onCameraStateChanged);
    QObject::connect(camera, static_cast<void(QCamera::*)(QCamera::Error)>(&QCamera::error), video_widget, &LocalVideoWidget::onCameraError);

    QObject::connect(video_widget, &LocalVideoWidget::onLocalVideoImmediatelyStart,
                     [=]() -> void {
                        onCameraStart();
                    });
    QObject::connect(video_widget, &LocalVideoWidget::onLocalVideoError,
                     [=](QCamera::Error) -> void {
                        clearCamera();
                    });
}


//---------------------------------------------------------------------------------------------------//

MediaControl::MediaControl() : cholder(), iholder(), vcontrol(nullptr), icontrol(), ocontrols()
{
    cholder.control = &vcontrol;
    iholder.control = &icontrol;
}

MediaControl::~MediaControl()
{

}

bool MediaControl::onInitCamera(std::pair<const QCameraInfo&, CameraFormatInfo> info, LocalVideoWidget *vwidget, const VideoFrameFunc &vfunc)
{
    if(!vwidget || info.first.isNull() || !vcontrol.onInitCamera(info.first, info.second, vwidget, vfunc)) {
        cholder.camera_error = LOCAL_VIDEO_ERROR_INIT; return false;
    }else{
        return true;
    }
}

LocalVideoError MediaControl::onUpdateLocalCameraInfo(std::pair<const QCameraInfo&, CameraFormatInfo> info, LocalVideoWidget *vwidget, const VideoFrameFunc &vfunc)
{
    if(vcontrol.getCameraInfo() == info.first){ cholder.camera_error = LOCAL_VIDEO_ERROR_REPEAT_FORMAT;  return LOCAL_VIDEO_ERROR_REPEAT_FORMAT; }
    if(info.first.isNull()) { cholder.camera_error = LOCAL_VIDEO_ERROR_INVALID_PARAMETER; return LOCAL_VIDEO_ERROR_INVALID_PARAMETER; }
    if(!vwidget){ cholder.camera_error = LOCAL_VIDEO_ERROR_NOT_VIEW; return LOCAL_VIDEO_ERROR_NOT_VIEW; }

    if((cholder.camera_error = vcontrol.onUpdateCamera(info.first, info.second, vwidget, vfunc)) == LOCAL_VIDEO_ERROR_NOT){
        vcontrol.onCameraLoad(vcontrol.isStartCamera());
    }
    return cholder.camera_error;
}

void MediaControl::onCameraStart()
{
    if(vcontrol.isHoldCamera()){
        if(vcontrol.isLoadCamera()){
            vcontrol.onCameraStart();
        }else{
            vcontrol.onCameraLoad(true);
        }
    }
}

void MediaControl::onCameraStop()
{
    if(vcontrol.isHoldCamera()){
        vcontrol.onCameraStop();
    }
}

MediaControl::AudioControlHolder MediaControl::onInitAudioInfo(const MediaControl::AudioControlHolder &control_holder, std::pair<const QAudioDeviceInfo&, QAudioFormat> info,
                                                               const CompleteFunc &cfunc, const VolumeFunc &vfunc, const AudioDeviceFunc &dfunc)
{
    AudioBasic *control = nullptr;
    if(!control_holder.isValid()){
        try{
            ocontrols.push_back(OutputAudioControl{});
            control = &ocontrols.back();
       }catch(...){
           return AudioControlHolder{nullptr};
       }
    }else{
        control = control_holder.control;
    }

    if(info.first.isNull() || (info.first == control->getAudioDeviceInfo()) || !control->onInitAudio(info.first, info.second, cfunc, vfunc, dfunc)){
        return AudioControlHolder{nullptr};
    }else{
        return AudioControlHolder{control};
    }
}

MediaControl::AudioControlHolder MediaControl::onUpdateAudioInfo(const MediaControl::AudioControlHolder &control_holder, std::pair<const QAudioDeviceInfo&, QAudioFormat> info,
                                                                 const CompleteFunc &cfunc, const VolumeFunc &vfunc, const AudioDeviceFunc &dfunc)
{
    if(!control_holder.isValid()){
        return AudioControlHolder{nullptr};
    }
    if(!isAudioInit(control_holder)){
        return onInitAudioInfo(control_holder, info, cfunc, vfunc, dfunc);
    }

    bool is_start = isAudioStart(control_holder);
    if(is_start){
        onAudioStop(control_holder);
    }

    if(info.first.isNull() || (info.first == control_holder.control->getAudioDeviceInfo()) || !control_holder.control->onUpdateAudio(info.first, info.second, cfunc, vfunc, dfunc)){
        return AudioControlHolder{nullptr};
    }else{
        if(is_start) onAudioStart(control_holder);
        return control_holder;
    }
}

void MediaControl::onAudioStart(const MediaControl::AudioControlHolder &control_holder)
{
    if(control_holder.isValid()){
        control_holder.control->onAudioStart();
    }
}

void MediaControl::onAudioStop(const MediaControl::AudioControlHolder &control_holder)
{
    if(control_holder.isValid()){
        control_holder.control->onAudioStop();
    }
}

void MediaControl::setAudioVolume(const MediaControl::AudioControlHolder &control_holder, qreal value)
{
    if(control_holder.isValid()){
        control_holder.control->setAudioVolume(value);
    }else{
        for(auto begin = ocontrols.begin(), end = ocontrols.end(); begin != end; ++begin){
            begin->setAudioVolume(value);
        }
    }
}

qreal MediaControl::getAudioVolume(const MediaControl::AudioControlHolder &control_holder)
{
    if(control_holder.isValid()){
        return control_holder.control->getAudioVolume();
    }
}
