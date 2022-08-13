#include "mainwindow.h"
#include "ui_mainwindow.h"

std::once_flag MainWindow::init_flag{};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), meeting_adapter(nullptr), media_control(nullptr), media_layout(nullptr), about_dialog(), set_dialog(), filter_dialog()
{
    ui->setupUi(this);
    setWindowState(Qt::WindowMaximized);
    setMouseTracking(true);
    setCentralWidget(ui->centralWidget);

    media_control = new MediaControl();
    media_layout = new LocalWidgetLayout(ui->mwidget);
    ui->mwidget->setLayout(media_layout);

    connect(&set_dialog, SIGNAL(onSettingConfirm()), this, SLOT(on_setting_confirm()));
    connect(&set_dialog, SIGNAL(onCameraUpdate(const QCameraInfo&, int, int)), this, SLOT(on_setting_camera(const QCameraInfo&, int, int)));
    connect(&set_dialog, SIGNAL(onAudioUpdate(const QAudioDeviceInfo&, QAudio::Mode, int)), this, SLOT(on_setting_audio(const QAudioDeviceInfo&, QAudio::Mode, int)));
    connect(&set_dialog, SIGNAL(onAudioFormatWarning(QAudio::Mode,int)), this, SLOT(on_setting_format_warning(QAudio::Mode,int)));
    connect(&set_dialog, SIGNAL(onCallbackInitCamera(const QList<QCameraInfo>&)), this, SLOT(on_update_camera(const QList<QCameraInfo>&)));
    connect(&set_dialog, SIGNAL(onCallbackInitAudio(const QList<QAudioDeviceInfo>&, QAudio::Mode)), this, SLOT(on_update_audio(const QList<QAudioDeviceInfo>&, QAudio::Mode)));

    connect(media_layout, SIGNAL(onUpdateWidgetContraposition(int)), this, SLOT(on_update_contraposition_widget(int)));
    connect(media_layout, SIGNAL(onClearWidgetContraposition()), this, SLOT(on_clear_contraposition_widget()));
    connect(media_layout, SIGNAL(onCorrelateWigetContraposition(LocalMediaWidget*)), this, SLOT(on_correlate_contraposition(LocalMediaWidget*)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete media_control;
}

void MainWindow::setAdapter(MeetingAdapter *adapter)
{
    meeting_adapter = adapter;
    meeting_adapter->onInitQtAdapter(sizeof(LocalMediaWidget));
    connect(meeting_adapter->onQtAdapter(), SIGNAL(on_adapter_local_launch()),
            this, SLOT(on_launch_fail()), Qt::QueuedConnection);
    connect(meeting_adapter->onQtAdapter(), SIGNAL(on_adapter_client_launch(IpInfo, TimeqInfo, MediaqInfo)),
            this, SLOT(on_launch_complete(IpInfo, TimeqInfo, MediaqInfo)), Qt::QueuedConnection);
    connect(meeting_adapter->onQtAdapter(), SIGNAL(on_adapter_server_launch(IpInfo, TimeqInfo, MediaqInfo)),
            this, SLOT(on_launch_complete(IpInfo, TimeqInfo, MediaqInfo)), Qt::QueuedConnection);
    connect(meeting_adapter->onQtAdapter(), SIGNAL(on_adapter_input_init(void*, uint32_t, uint32_t, uint32_t)),
            this, SLOT(on_remote_note_init(void*, uint32_t, uint32_t, uint32_t)), Qt::QueuedConnection);
    connect(meeting_adapter->onQtAdapter(), SIGNAL(on_adapter_input_update(uint32_t, const char*, const char*, void*)),
            this, SLOT(on_remote_note_update(uint32_t, const char*, const char*, void*)), Qt::QueuedConnection);
    connect(meeting_adapter->onQtAdapter(), SIGNAL(on_adapter_input_media(const char*,uint32_t, void*)),
            this, SLOT(on_remote_note_media(const char*, uint32_t, void*)), Qt::QueuedConnection);
    connect(meeting_adapter->onQtAdapter(), SIGNAL(on_adapter_input_indicator(void*, const char*)),
            this, SLOT(on_remote_note_indicator(void*, const char*)), Qt::QueuedConnection);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int width =ui->media_widget->width();
    int height = (ui->centralWidget->height() >> 1) + 50;

    ui->splitter->setGeometry(ui->centralWidget->geometry());
    ui->local_box->setGeometry(QRect{0, 0, width, height});
    ui->vs_box->setGeometry(QRect{0, height, width, ui->centralWidget->height()- height});

    ui->label_runtime->setGeometry({10, (ui->media_widget->height() - height) - ui->label_runtime->height() - 10,
                                    ui->label_runtime->width(), ui->label_runtime->height()});
    ui->label_status->setGeometry({10 + ui->label_runtime->width(), (ui->media_widget->height() - height) - ui->label_status->height() - 10,
                                   ui->label_status->width(), ui->label_runtime->height()});

    const QRect &rect = QRect{ui->media_widget->width(), 0, ui->centralWidget->width() - ui->media_widget->width(), ui->centralWidget->height()};
    ui->splitter_group->setGeometry(rect);
    ui->mwidget->setGeometry(QRect{0, 0, rect.width(), rect.height() - 30});
    media_layout->setGeometry(QRect{0, 0, rect.width(), rect.height() - 30});
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    media_layout->onLayoutClose();
    about_dialog.close();
    set_dialog.close();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    std::call_once(init_flag, MainWindow::onGlobalInit, &set_dialog, &filter_dialog, ui->label_global_status,
                   [&]() -> void{
                       disconnect(ui->camera_box, SIGNAL(currentIndexChanged(int)), this, SLOT(on_camera_box_currentIndexChanged(int)));
                       disconnect(ui->audio_box, SIGNAL(currentIndexChanged(int)), this, SLOT(on_audio_box_currentIndexChanged(int)));
                       disconnect(ui->audioinput_slider, SIGNAL(valueChanged(int)), this, SLOT(on_audioinput_slider_valueChanged(int)));
                       disconnect(ui->audiooutput_slider, SIGNAL(valueChanged(int)), this, SLOT(on_audiooutput_slider_valueChanged(int)));
                }, [&]() -> void {
                       connect(ui->camera_box, SIGNAL(currentIndexChanged(int)), this, SLOT(on_camera_box_currentIndexChanged(int)));
                       connect(ui->audio_box, SIGNAL(currentIndexChanged(int)), this, SLOT(on_audio_box_currentIndexChanged(int)));
                       connect(ui->audioinput_slider, SIGNAL(valueChanged(int)), this, SLOT(on_audioinput_slider_valueChanged(int)));
                       connect(ui->audiooutput_slider, SIGNAL(valueChanged(int)), this, SLOT(on_audiooutput_slider_valueChanged(int)));
                   });
}

void MainWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
}

void MainWindow::on_action_launch_triggered()
{
    using LaunchFunc = void(MeetingAdapter::*)(const IpInfo&, const TimeqInfo&, const MediaqInfo&);

    if(!meeting_adapter->isLaunch()){
        LaunchFunc launch_func = nullptr;

        ui->actionclient_mode->setDisabled(true);
        ui->actionserver_mode->setDisabled(true);
        switch (meeting_adapter->onCoreMode()) {
        case SERVER_MODE:
            ui->label_global_status->setText("正在启动Meeting！");
            ui->action_launch->setText(QString{"停止"});

            launch_func = &MeetingAdapter::onMeetingLaunch;
            break;
        case CLIENT_MODE:
            ui->label_global_status->setText("正在链接Meeting！");
            ui->action_launch->setText(QString{"断开"});

            launch_func = &MeetingAdapter::onMeetingLink;
            break;
        default:
            break;
        }

        if(launch_func){

            auto ip = set_dialog.onSettingIpInfo();
            auto time = set_dialog.onSettingLinkInfo();
            auto camera = set_dialog.onSettingCameraInfo();
            auto audio = set_dialog.onSettingAudioinfo(QAudio::AudioInput);

            IpInfo ip_info;
            ip_info.jtime = 1 * 1000 * 1000; //1秒
            ip_info.ltime = 1 * 1000 * 1000; //1秒
            ip_info.ptime = 60 * 1000 * 1000; //60秒
            ip_info.tsize = 10; //触发次数(加入、链接)
            ip_info.link_port = ip.first.sin_port;
            ip_info.link_addr = ip.first.sin_addr.s_addr;
            ip_info.link_port = ip.second.sin_port;
            ip_info.link_addr = ip.second.sin_addr.s_addr;

            TimeqInfo time_info;
            time_info.frame = std::get<0>(time);
            time_info.delay = std::get<1>(time);
            time_info.timeout = std::get<2>(time);
            time_info.note_size = 0;

            MediaqInfo media_info;
            media_info.channel_count = audio.second.channelCount();
            media_info.byte_order = audio.second.byteOrder();
            media_info.sample_size = audio.second.sampleSize();
            media_info.sample_rate = audio.second.sampleRate();

            media_info.ratio = camera.second.getCameraRatio().width();
            media_info.resolution = 0;
            media_info.format = static_cast<uint32_t>(camera.second.getCameraFormat());

            (meeting_adapter->*launch_func)(ip_info, time_info, media_info);
        }
    }else{
        ui->actionclient_mode->setDisabled(false);
        ui->actionserver_mode->setDisabled(false);

        switch (meeting_adapter->onCoreMode()) {
        case SERVER_MODE:
            ui->label_global_status->setText("正在停止Meeting！");
            ui->action_launch->setText(QString{"启动"});
            break;
        case CLIENT_MODE:
            ui->label_global_status->setText("正在断开Meeting！");
            ui->action_launch->setText(QString{"链接"});
            break;
        default:
            break;
        }
        meeting_adapter->onMeetingClose();
    }
}

void MainWindow::on_action_about_triggered()
{
    if(!about_dialog.isVisible()){
        about_dialog.show();
    }
}

void MainWindow::on_action_setting_triggered()
{
    uint32_t flag = 0;

    if(media_control->isCameraStart()){
        flag |= SETTING_DIALOG_SHOW_MATCH_CAMERA_FORMAT;
    }
    if(media_control->isAudioStart(media_control->onAudioInputType())){
        flag |= SETTING_DIALOG_SHOW_MATCH_AUDIOINPUT_FORMAT;
    }

    set_dialog.onSettingShow(flag);
}

void MainWindow::on_actionserver_mode_triggered()
{
    meeting_adapter->onCoreMode(CLIENT_MODE);
    ui->actionclient_mode->setChecked(true);
    ui->actionserver_mode->setChecked(false);

    ui->action_launch->setText(QString{"链接"});
}

void MainWindow::on_actionclient_mode_triggered()
{
    meeting_adapter->onCoreMode(SERVER_MODE);
    ui->actionserver_mode->setChecked(true);
    ui->actionclient_mode->setChecked(false);

    ui->action_launch->setText(QString{"启动"});
}

void MainWindow::on_action_blackList_triggered()
{
    filter_dialog.onFilterShow(FilterBlack);
}

void MainWindow::on_action_whiteList_triggered()
{
    filter_dialog.onFilterShow(FilterBlack);
}

void MainWindow::on_action_linkInfo_triggered()
{

}

void MainWindow::on_action_time_triggered()
{

}

void MainWindow::on_action_linke_triggered()
{

}

void MainWindow::on_action_status_triggered()
{

}

void MainWindow::on_setting_confirm()
{

}

void MainWindow::on_setting_camera(const QCameraInfo&, int pos, int opos)
{    
    ui->camera_box->setCurrentIndex(pos);
    onUpdateCamera(opos);
}

void MainWindow::on_setting_audio(const QAudioDeviceInfo&, QAudio::Mode mode, int pos)
{
    if(mode != QAudio::AudioInput) return;
    ui->audio_box->setCurrentIndex(pos);
    onUpdateAudio(mode);
}

void MainWindow::on_setting_format_warning(QAudio::Mode mode, int pos)
{
    if(mode != QAudio::AudioInput) return;

    disconnect(ui->audio_box, SIGNAL(currentIndexChanged(int)), this, SLOT(on_audio_box_currentIndexChanged(int)));
    ui->audio_box->setCurrentIndex(pos);
    QMessageBox::warning(this, "警告", "音频格式不支持，请打开设置窗口设置音频格式！", QMessageBox::Ok, QMessageBox::Cancel);
    connect(ui->audio_box, SIGNAL(currentIndexChanged(int)), this, SLOT(on_audio_box_currentIndexChanged(int)));
}

void MainWindow::on_update_camera(const QList<QCameraInfo> &infos)
{
    foreach (const auto &info, infos) {
        if(info.position() == QCamera::Position::FrontFace){
            ui->camera_box->addItem(SettingDialog::front_value);
        }else if(info.position() == QCamera::Position::BackFace){
            ui->camera_box->addItem(SettingDialog::back_value);
        }else {
            ui->camera_box->addItem(SettingDialog::default_value);
        }
    }
}

void MainWindow::on_update_audio(const QList<QAudioDeviceInfo> &infos, QAudio::Mode mode)
{
    if(mode == QAudio::AudioInput){
        foreach (const auto &info, infos) {
            ui->audio_box->addItem(info.deviceName());
        }
    }
}

void MainWindow::onGlobalInit(SettingDialog *set, FilterDialog *filter, QLabel *label, const std::function<void()> &sfunc, const std::function<void()> &efunc)
{
    sfunc();

    set->onGlobalInit(label);
    filter->onGlobalInit(label);
    label->setText("启动完成!!!");

    efunc();
}

void MainWindow::onLaunchClientComplete(const IpInfo &ip_info, const TimeqInfo &time_info, const MediaqInfo &media_info)
{
    set_dialog.onServantInit(IpLinkInfo{time_info.frame, time_info.delay, time_info.timeout, ip_info.local_port, ip_info.local_addr
//                                        , ip_info.link_port, ip_info.link_addr
                                       }, CameraFormatInfo{static_cast<QVideoFrame::PixelFormat>(media_info.format), time_info.frame, QSize{media_info.ratio, media_info.ratio}},
                                          QAudioFormat{}, QAudioFormat{});

    ui->video_button->setText("停止（摄像头）");
    ui->video_button->setEnabled(false);
    ui->audio_button->setEnabled(false);

    {
        media_control->onUpdateLocalCameraInfo(set_dialog.onSettingCameraInfo(), ui->local_video,
                                               [&](const QVideoFrame &frame) -> void { meeting_adapter->onUpdateVideoFrame(frame); });
        media_control->onCameraStart();
    }
    {
        media_control->onUpdateAudioInfo(media_control->onAudioInputType(), set_dialog.onSettingAudioinfo(QAudio::AudioInput),
                                       [=](qreal value) -> void {
                                            ui->audioinput_slider->setValue(qRound(value * 100));
                                    }, [=](qreal value) -> void {
                                            ui->local_audio->updateAudioVolume(value);
                                    }, [=](int len, int pos, IOControl *control) -> void {
                                            meeting_adapter->onUpdateAudioInput(len, pos, control);
                                    });
        media_control->onAudioStart(media_control->onAudioInputType());
    }
}

void MainWindow::onLaunchServerComplete(const IpInfo&, const TimeqInfo&, const MediaqInfo&)
{
    set_dialog.onSettingStart();

    if(!media_control->isCameraStart()){
        ui->video_button->setText("停止（摄像头）");
        set_dialog.orShowFlag(SETTING_DIALOG_SHOW_MATCH_AUDIOINPUT_FORMAT);
        if(!media_control->isCameratInit()){
            media_control->onInitCamera(set_dialog.onSettingCameraInfo(), ui->local_video, [&](const QVideoFrame &frame) -> void { meeting_adapter->onUpdateVideoFrame(frame); });
        }
        media_control->onCameraStart();
    }


    ui->video_button->setEnabled(false);
    ui->audio_button->setEnabled(false);
}

void MainWindow::on_camera_box_currentIndexChanged(int index)
{
    disconnect(&set_dialog, SIGNAL(onCameraUpdate(const QCameraInfo&, int)), this, SLOT(on_setting_camera(const QCameraInfo&, int)));
    onUpdateCamera(set_dialog.onCameraBoxChange(index));
    connect(&set_dialog, SIGNAL(onCameraUpdate(const QCameraInfo&, int)), this, SLOT(on_setting_camera(const QCameraInfo&, int)));
}

void MainWindow::onUpdateCamera(int opos)
{
    if(!media_control->isCameratInit()){
        if(!media_control->onInitCamera(set_dialog.onSettingCameraInfo(), ui->local_video, [&](const QVideoFrame &frame) -> void { meeting_adapter->onUpdateVideoFrame(frame); })){
            ui->label_global_status->setText(QString{"初始化摄像头失败！"});
        }
    }else{
        LocalVideoError error = LOCAL_VIDEO_ERROR_NOT;
        if((error = media_control->onUpdateLocalCameraInfo(set_dialog.onSettingCameraInfo(), ui->local_video,
                                                           [&](const QVideoFrame &frame) -> void { meeting_adapter->onUpdateVideoFrame(frame); })) == LOCAL_VIDEO_ERROR_NOT){
            set_dialog.onCameraInit(media_control->onCameraType());
        }else{
            switch (error){
            case LOCAL_VIDEO_ERROR_INVALID_PARAMETER:
                ui->label_global_status->setText(QString{"摄像头参数错误！"});
                break;
            case LOCAL_VIDEO_ERROR_HOLD_CAMER:
                ui->label_global_status->setText(QString{"已关联摄像头！"});
                break;
            case LOCAL_VIDEO_ERROR_INIT:
                ui->label_global_status->setText(QString{"初始化错误！"});
                break;
            case LOCAL_VIDEO_ERROR_INVALID_FORMAT:
                ui->label_global_status->setText(QString{"摄像头格式不支持！"});
                break;
            case LOCAL_VIDEO_ERROR_REPEAT_FORMAT:
                ui->label_global_status->setText(QString{"相同的摄像头！"});
                break;
            case LOCAL_VIDEO_ERROR_NOT_VIEW:
                ui->label_global_status->setText(QString{"没有显示控件！"});
                break;
            default:
                break;
            }
        }
    }
    set_dialog.onCameraInit(media_control->onCameraType().onRollbackPos(opos));
}

void MainWindow::on_audio_box_currentIndexChanged(int index)
{
    disconnect(&set_dialog, SIGNAL(onAudioUpdate(const QAudioDeviceInfo&, QAudio::Mode, int)), this, SLOT(on_setting_audio(const QAudioDeviceInfo&, QAudio::Mode, int)));
    ui->local_audio->clearVolume();
    set_dialog.onAudioInputBoxChange(index);
    onUpdateAudio(QAudio::AudioInput);
    connect(&set_dialog, SIGNAL(onAudioUpdate(const QAudioDeviceInfo&, QAudio::Mode, int)), this, SLOT(on_setting_audio(const QAudioDeviceInfo&, QAudio::Mode, int)));
}

void MainWindow::onUpdateAudio(QAudio::Mode mode)
{
    MediaControl::AudioControlHolder audio_holder;

    if(!media_control->isAudioInit(media_control->onAudioInputType())){
        audio_holder = media_control->onInitAudioInfo(media_control->onAudioInputType(), set_dialog.onSettingAudioinfo(mode),
                                                           [=](qreal value) -> void {
                                                                ui->audioinput_slider->setValue(qRound(value * 100));
                                                        }, [=](qreal value) -> void {
                                                                ui->local_audio->updateAudioVolume(value);
                                                        }, [=](int len, int pos, IOControl *control) -> void {
                                                                meeting_adapter->onUpdateAudioInput(len, pos, control);
                                                        });
    }else{
        audio_holder = media_control->onUpdateAudioInfo(media_control->onAudioInputType(), set_dialog.onSettingAudioinfo(mode),
                                           [=](qreal value) -> void {
                                                ui->audioinput_slider->setValue(qRound(value * 100));
                                        }, [=](qreal value) -> void {
                                                ui->local_audio->updateAudioVolume(value);
                                        }, [=](int len, int pos, IOControl *control) -> void {
                                                meeting_adapter->onUpdateAudioInput(len, pos, control);
                                        });
    }
    if(audio_holder.isValid()){
        set_dialog.onAudioInit(audio_holder);
    }else{
        ui->label_global_status->setText(QString{"更改音频错误！"});
    }
}


void MainWindow::on_video_button_clicked()
{
    if(!media_control->isCameraStart()){
        ui->video_button->setText("停止（摄像头）");
        set_dialog.orShowFlag(SETTING_DIALOG_SHOW_MATCH_CAMERA_FORMAT);
        if(!media_control->isCameratInit()){
            media_control->onInitCamera(set_dialog.onSettingCameraInfo(), ui->local_video, [&](const QVideoFrame &frame) -> void { meeting_adapter->onUpdateVideoFrame(frame); });
        }
        media_control->onCameraStart();
    }else{
        ui->video_button->setText("启动（摄像头）");
        set_dialog.clrShowFlag(SETTING_DIALOG_SHOW_MATCH_CAMERA_FORMAT);
        media_control->onCameraStop();
    }
}

void MainWindow::on_audio_button_clicked()
{
    if(!media_control->isAudioStart(media_control->onAudioInputType())){
        ui->audio_button->setText("停止（音频）");
        set_dialog.orShowFlag(SETTING_DIALOG_SHOW_MATCH_AUDIOINPUT_FORMAT);
        media_control->onUpdateAudioInfo(media_control->onAudioInputType(), set_dialog.onSettingAudioinfo(QAudio::AudioInput),
                                       [=](qreal value) -> void {
                                            ui->audioinput_slider->setValue(qRound(value * 100));
                                    }, [=](qreal value) -> void {
                                            ui->local_audio->updateAudioVolume(value);
                                    }, [=](int len, int pos, IOControl *control) -> void {
                                            meeting_adapter->onUpdateAudioInput(len, pos, control);
                                    });
        media_control->onAudioStart(media_control->onAudioInputType());
    }else{
        ui->audio_button->setText("启动（音频）");
        set_dialog.clrShowFlag(SETTING_DIALOG_SHOW_MATCH_AUDIOINPUT_FORMAT);
        media_control->onAudioStop(media_control->onAudioInputType());
        ui->local_audio->clearVolume();
    }
}

void MainWindow::on_update_contraposition_widget(int value)
{

}

void MainWindow::on_clear_contraposition_widget()
{

}

void MainWindow::on_correlate_contraposition(LocalMediaWidget *media_widget)
{

}

void MainWindow::on_launch_complete(IpInfo ip_info, TimeqInfo time_info, MediaqInfo media_info)
{
    if(ip_info.link_addr && ip_info.link_port){
        onLaunchClientComplete(ip_info, time_info, media_info);
    }else{
        onLaunchServerComplete(ip_info, time_info, media_info);
    }
}

void MainWindow::on_launch_fail()
{
    set_dialog.onSettingStop();
    ui->label_global_status->setText("链接启动失败！！！");
}

void MainWindow::on_remote_note_init(void *widget, uint32_t pos, uint32_t port, uint32_t addr)
{
    (new (widget) LocalMediaWidget())->onInitInfo(pos, port, addr);
}

void MainWindow::on_remote_note_update(uint32_t pos, const char *status, const char *addr, void *media_widget)
{
    reinterpret_cast<LocalMediaWidget*>(media_widget)->onUpdateStatus(pos, status, addr);
}

void MainWindow::on_remote_note_media(const char *media_buffer, uint32_t buffer_len, void *media_widget)
{
    reinterpret_cast<LocalMediaWidget*>(media_widget)->onUpdateMedia(media_buffer, buffer_len, [&]() -> void { meeting_adapter->onMeetingMediaComplete(media_buffer, buffer_len); });
}

void MainWindow::on_remote_note_indicator(void *media_widget, const char *addr)
{

}

void MainWindow::on_audioinput_slider_valueChanged(int value)
{
    qreal linear_volume =  QAudio::convertVolume(value / qreal(100),
                                                QAudio::LogarithmicVolumeScale,
                                                QAudio::LinearVolumeScale);
    media_control->setAudioVolume(media_control->onAudioInputType(), linear_volume);
}

void MainWindow::on_audiooutput_slider_valueChanged(int value)
{
    qreal linear_volume =  QAudio::convertVolume(value / qreal(100),
                                                QAudio::LogarithmicVolumeScale,
                                                QAudio::LinearVolumeScale);
    media_control->setAudioVolume(media_control->onAudioOutputType(), linear_volume);
}
