#ifndef MEDIACONTROL_H
#define MEDIACONTROL_H

#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QCamera>
#include <QCameraInfo>
#include <QVideoProbe>
#include <QVideoFrame>
#include <QVideoProbe>
#include <QVideoWidget>
#include <QIODevice>

#include <list>

#include "../Setting/settinginfo.h"
#include "../IO/audiodevice.h"
#include "../../Widget/localvideowidget.h"

class LocalVideoWidget;

enum LocalVideoError {
    LOCAL_VIDEO_ERROR_NOT,
    LOCAL_VIDEO_ERROR_INVALID_PARAMETER,
    LOCAL_VIDEO_ERROR_HOLD_CAMER,
    LOCAL_VIDEO_ERROR_INIT,
    LOCAL_VIDEO_ERROR_INVALID_FORMAT,
    LOCAL_VIDEO_ERROR_REPEAT_FORMAT,
    LOCAL_VIDEO_ERROR_NOT_VIEW
};

using CompleteFunc = std::function<void(qreal)>;
using VolumeFunc = std::function<void(qreal)>;
using AudioDeviceFunc = std::function<void(int, int, IOControl*)>;
using VideoFrameFunc = std::function<void(const QVideoFrame&)>;

class AudioBasic{
public:
    AudioBasic() : io_control(nullptr), audio_format(), info() {}
    virtual ~AudioBasic() = default;

    virtual bool onInitAudio(const QAudioDeviceInfo&, QAudioFormat, const CompleteFunc& , const VolumeFunc&, const AudioDeviceFunc&) = 0;
    virtual bool onUpdateAudio(const QAudioDeviceInfo&, QAudioFormat, const CompleteFunc& , const VolumeFunc&, const AudioDeviceFunc&) = 0;
    virtual const QAudioDeviceInfo& getAudioDeviceInfo() const { return info; }
    virtual void setAudioVolume(qreal) = 0;
    virtual qreal getAudioVolume() const = 0;
    virtual bool isHoldAudio() const = 0;
    virtual bool isAudioStart() const = 0;
    virtual void onAudioStart() = 0;
    virtual void onAudioStop() = 0;

    void updateVolumeFunc(const VolumeFunc&);
private:
    virtual bool onInitDevice(const QAudioDeviceInfo&, QAudioFormat, const VolumeFunc&, const AudioDeviceFunc&) = 0;
protected:
    bool onInitComplete(const CompleteFunc &func) { if(func){ func(getAudioVolume()); } return true; }
    IOControl* initAudioDevice(IOControl*, const AudioDeviceFunc&);
    bool onOpenDevice(QIODevice::OpenMode);

    IOControl *io_control;
    QAudioFormat audio_format;
    QAudioDeviceInfo info;
};

class MediaControl
{
    class InputAudioControl : public AudioBasic{
    public:
        InputAudioControl() : AudioBasic(), audio_input(nullptr) {}
        ~InputAudioControl() override = default;

        bool onInitAudio(const QAudioDeviceInfo&, QAudioFormat, const CompleteFunc&, const VolumeFunc&, const AudioDeviceFunc&) override;
        bool onUpdateAudio(const QAudioDeviceInfo&, QAudioFormat, const CompleteFunc& , const VolumeFunc&, const AudioDeviceFunc&) override;
        void setAudioVolume(qreal value) override { audio_input->setVolume(value); }
        qreal getAudioVolume() const override { return audio_input->volume(); }

        bool isHoldAudio() const override { return audio_input; }
        bool isAudioStart() const override { return (audio_input->state() != QAudio::StoppedState); }
        void onAudioStart() override;
        void onAudioStop() override;
    private:
        bool onInitDevice(const QAudioDeviceInfo&, QAudioFormat, const VolumeFunc&, const AudioDeviceFunc&) override;
        bool onResetDevice(const QAudioDeviceInfo&, QAudioFormat, const VolumeFunc&, const AudioDeviceFunc&);
        QAudioInput *audio_input;
    };

    class OutputAudioControl : public AudioBasic{
    public:
        OutputAudioControl() : AudioBasic(), audio_output(nullptr) {}
        ~OutputAudioControl() override = default;

        bool onInitAudio(const QAudioDeviceInfo&, QAudioFormat, const CompleteFunc&, const VolumeFunc&, const AudioDeviceFunc&) override;
        bool onUpdateAudio(const QAudioDeviceInfo&, QAudioFormat, const CompleteFunc& , const VolumeFunc&, const AudioDeviceFunc&) override { return false; /* 输出不支持更改. */}
        void setAudioVolume(qreal value) override { audio_output->setVolume(value); }
        qreal getAudioVolume() const override { return audio_output->volume(); }

        bool isHoldAudio() const override { return audio_output; }
        bool isAudioStart() const override { return (audio_output->state() != QAudio::StoppedState); }
        void onAudioStart() override;
        void onAudioStop() override;
    private:
        bool onInitDevice(const QAudioDeviceInfo&, QAudioFormat, const VolumeFunc&, const AudioDeviceFunc&) override;

        QAudioOutput *audio_output;
    };

    class VideoControl{
    public:
        explicit VideoControl(const std::function<void(const QVideoFrame&)> &func) : is_start(false), camera(nullptr), ocamera(nullptr), probe(), info(), video_callback(func), conn_callback() {}
        ~VideoControl() = default;

        bool isLoadCamera() const { return (camera->state() != QCamera::UnloadedState); }
        bool isHoldCamera() const { return camera; }
        bool isStartCamera() const { return is_start; }
        const QCameraInfo& getCameraInfo() const { return info; }
        bool onInitCamera(const QCameraInfo&, const CameraFormatInfo&, LocalVideoWidget*, const VideoFrameFunc&);
        LocalVideoError onUpdateCamera(const QCameraInfo&, const CameraFormatInfo&, LocalVideoWidget*, const VideoFrameFunc&);
        void onCameraLoad(bool s = false);
        void onCameraStart();
        void onCameraStop();
        void setVideoWidget(QVideoWidget*);
        void releaseOldCamera() { ocamera = reinterpret_cast<QCamera*>(releaseCamera(ocamera)); }
        void releaseHoldCamera() { camera = reinterpret_cast<QCamera*>(releaseCamera(camera)); }
        QString onCameraErrorString() { return camera->errorString(); }
    private:
        static void* releaseCamera(QCamera*);
        bool onFitCameraInfo(const CameraFormatInfo&);
        void onInitViewWiget(LocalVideoWidget*);
        void clearCamera() { releaseOldCamera(); releaseHoldCamera(); }

        bool is_start;
        QCamera *camera;
        QCamera *ocamera;
        LocalVideoWidget *video_widget;
        QVideoProbe probe;
        QCameraInfo info;
        std::function<void(const QVideoFrame&)> video_callback;
        QMetaObject::Connection conn_callback;
    };
public:
    class AudioControlHolder {
        friend class MediaControl;
    public:
        explicit AudioControlHolder(AudioBasic *audio = nullptr) : control(audio) {}
        ~AudioControlHolder() = default;

        bool isValid() const { return control; }
        void setUpdateVolumeFunc(const VolumeFunc &func) { control->updateVolumeFunc(func); }
    private:
        AudioBasic *control;
    };

    class CameraControlHolder {
        friend class MediaControl;
    public:
        explicit CameraControlHolder(VideoControl *c = nullptr) : camera_pos(0), camera_error(), control(c) {}
        ~CameraControlHolder() = default;

        bool isValid() const { return control; }
        int onCameraPos() const { return camera_pos; }
        LocalVideoError onCameraError() const { return camera_error; }
        CameraControlHolder& onRollbackPos(int pos) { camera_pos = pos; return *this;}
    private:
        int camera_pos; //rollback_pos
        LocalVideoError camera_error;
        VideoControl *control;
    };
public:
    MediaControl();
    ~MediaControl();

    MediaControl::AudioControlHolder onAudioInputType() { return iholder; }
    MediaControl::AudioControlHolder onAudioOutputType() { return AudioControlHolder(nullptr); }
    MediaControl::CameraControlHolder onCameraType() { return cholder; }

    bool onInitCamera(std::pair<const QCameraInfo&, CameraFormatInfo>, LocalVideoWidget*, const VideoFrameFunc&);
    LocalVideoError onUpdateLocalCameraInfo(std::pair<const QCameraInfo&, CameraFormatInfo>, LocalVideoWidget*, const VideoFrameFunc&);

    bool isCameratInit() const { return vcontrol.isHoldCamera(); }
    bool isCameraStart() const { return vcontrol.isStartCamera(); }
    void onCameraStart();
    void onCameraStop();
    void releaseOldCamera() { vcontrol.releaseOldCamera(); }
    void releaseHoldCamera() { vcontrol.releaseHoldCamera(); }
    QString onCameraErrorString() { return vcontrol.onCameraErrorString(); }

    MediaControl::AudioControlHolder onInitAudioInfo(const AudioControlHolder&, std::pair<const QAudioDeviceInfo&, QAudioFormat>,
                                                     const CompleteFunc&, const VolumeFunc&, const AudioDeviceFunc&);
    MediaControl::AudioControlHolder onUpdateAudioInfo(const AudioControlHolder&, std::pair<const QAudioDeviceInfo&, QAudioFormat>,
                                                       const CompleteFunc&, const VolumeFunc&, const AudioDeviceFunc&);
    bool isAudioInit(const AudioControlHolder &control_holder) const { return (control_holder.isValid() && control_holder.control->isHoldAudio()); }
    bool isAudioStart(const AudioControlHolder &control_holder) const { return (control_holder.control->isHoldAudio() && control_holder.control->isAudioStart()); }
    void onAudioStart(const AudioControlHolder&);
    void onAudioStop(const AudioControlHolder&);
    void setAudioVolume(const AudioControlHolder&, qreal);
    qreal getAudioVolume(const AudioControlHolder&);
private:
    CameraControlHolder cholder;
    AudioControlHolder iholder;

    VideoControl vcontrol;
    InputAudioControl icontrol;
    std::list<OutputAudioControl> ocontrols;
};

#endif // MEDIACONTROL_H
