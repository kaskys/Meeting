#include "settingdialog.h"
#include "ui_settingdialog.h"

void SetCameraType::onDisconnent() const
{
    QObject::disconnect(sdialog->ui->camera_drive_box, SIGNAL(currentIndexChanged(int)), sdialog, SLOT(on_camera_drive_box_currentIndexChanged(int)));
}

void SetCameraType::onRollBack(int pos)
{
    sdialog->ui->camera_drive_box->setCurrentIndex(pos);
}

void SetCameraType::onConnent() const
{
    QObject::connect(sdialog->ui->camera_drive_box, SIGNAL(currentIndexChanged(int)), sdialog, SLOT(on_camera_drive_box_currentIndexChanged(int)));
}

void SetAudioType::onDisconnent() const
{
    QObject::disconnect(sdialog->ui->audioinput_drive_box, SIGNAL(currentIndexChanged(int)), sdialog, SLOT(on_audioinput_drive_box_currentIndexChanged(int)));
}

void SetAudioType::onRollBack(int pos)
{
    sdialog->ui->audioinput_drive_box->setCurrentIndex(pos);
}

void SetAudioType::onConnent() const
{
    QObject::connect(sdialog->ui->audioinput_drive_box, SIGNAL(currentIndexChanged(int)), sdialog, SLOT(on_audioinput_drive_box_currentIndexChanged(int)));
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------//

SettingInfo SettingDialog::dialog_info{};
MediaInfo SettingDialog::media_info{};

const QString SettingDialog::default_value{"默认"};
const QString SettingDialog::back_value{"后置"};
const QString SettingDialog::front_value{"前置"};

SettingDialog::SettingDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingDialog), camera_type(this), audio_type(this),  state_type(SET_STATE_TYPE_INIT), show_flag(0), ip_editor()
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);
    setWindowTitle(QString{"设置"});
    setWindowOpacity(1);
    setMouseTracking(true);

    initLayout();
    initErrorMap();

    ui->tabWidget->setCurrentWidget(ui->tab_ip);
    connect(&ip_editor, SIGNAL(onIpValueConfirm()), this, SLOT(on_set_ip_confirm()));
}

SettingDialog::~SettingDialog()
{
    delete ui;
}

void SettingDialog::onGlobalInit(QLabel *label)
{
    label->setText("初始化配置信息!");
    InitInfo::onInitNet(&dialog_info);
    InitInfo::onInitMedia(&media_info);

    initIpValue();
    initLinkValue();
    initCameraInfo();
    initAudioInfo(ui->audioinput_drive_box, ui->audioinput_codec_box, ui->audioinput_channel_box, ui->audioinput_rate_box, ui->audioinput_size_box,
                  media_info.getMediaAudioInputInfo(), media_info.getMediaAudioInputDefaultInfo(), QAudio::AudioInput);
    initAudioInfo(ui->audiooutput_drive_box, ui->audiooutput_codec_box, ui->audiooutput_channel_box, ui->audiooutput_rate_box, ui->audiooutput_size_box,
                  media_info.getMediaAudioOutputInfo(), media_info.getMediaAudioOutputDefaultInfo(), QAudio::AudioOutput);

    label->setText("配置信息初始化完成!");
}

void SettingDialog::onServantInit(const IpLinkInfo &linfo, const CameraFormatInfo &cformat, const QAudioFormat &iformat, const QAudioFormat &oformat)
{
    onStoreValue(linfo, cformat, iformat, oformat);
    onResetValue(cformat, iformat, oformat);
    state_type = SET_STATE_TYPE_SERVANT;
}

void SettingDialog::onSettingShow(uint32_t flag)
{
    this->show_flag = flag;
    if(!this->isVisible()){
        switch (state_type) {
        case SET_STATE_TYPE_COMFIRM:
            onLoadValue();
        case SET_STATE_TYPE_INIT:
            onEnabledValue(true);
            break;
        case SET_STATE_TYPE_START:
        case SET_STATE_TYPE_SERVANT:
            onEnabledValue(false);
            break;
        default:
            break;
        }
        if(state_type == SET_STATE_TYPE_COMFIRM){
            state_type = SET_STATE_TYPE_REPEAT;
        }

        this->show();
    }
}

void SettingDialog::onSettingStart()
{
    if(state_type == SET_STATE_TYPE_INIT){
        onStoreValue();
    }
    state_type = SET_STATE_TYPE_START;
}

void SettingDialog::onSettingStop()
{
    state_type = SET_STATE_TYPE_COMFIRM;
}

void SettingDialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    rectButton();
    rectTabLayout();
}

void SettingDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    const auto &size = event->size();
    ui->splitter_widget->setGeometry(0, 0, size.width(), size.height());
    rectButton();
    rectTabLayout();
}

void SettingDialog::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);

    if(state_type == SET_STATE_TYPE_REPEAT){
        state_type = SET_STATE_TYPE_COMFIRM;
    }
    if(ip_editor.isVisible()){
        ip_editor.close();
    }
}

void SettingDialog::on_ip_btn_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tab_ip);
    onIpPageChange();
}

void SettingDialog::on_info_btn_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tab_info);
    onInfoPageChange();
}

void SettingDialog::on_camera_btn_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tab_camera);
    onCameraPageChange();
}

void SettingDialog::on_audio_btn_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tab_audio);
    onAudioPageChange();
}

void SettingDialog::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index);
    auto *current_widget = ui->tabWidget->currentWidget();
    if(current_widget == ui->tab_ip){
        onIpPageChange();
    } else if(current_widget == ui->tab_info){
        onInfoPageChange();
    } else if(current_widget == ui->tab_camera){
        onCameraPageChange();
    } else if(current_widget == ui->tab_audio){
        onAudioPageChange();
    }
}

void SettingDialog::on_btn_ok_clicked()
{
    onStoreValue();
    state_type = SET_STATE_TYPE_COMFIRM;
    close();
emit onSettingConfirm();
}

void SettingDialog::on_btn_reset_clicked()
{
    onResetValue();

    initIpValue();
    initCameraInfo();
    initAudioInfo(ui->audioinput_drive_box, ui->audioinput_codec_box, ui->audioinput_channel_box, ui->audioinput_rate_box, ui->audioinput_size_box,
                  media_info.getMediaAudioInputInfo(), media_info.getMediaAudioInputDefaultInfo(), QAudio::AudioInput);
    initAudioInfo(ui->audiooutput_drive_box, ui->audiooutput_codec_box, ui->audiooutput_channel_box, ui->audiooutput_rate_box, ui->audiooutput_size_box,
                  media_info.getMediaAudioOutputInfo(), media_info.getMediaAudioOutputDefaultInfo(), QAudio::AudioOutput);

    onLoadValue();
    state_type = SET_STATE_TYPE_INIT;
}

void SettingDialog::on_sys_check_clicked(bool checked)
{
    ui->port_box->setEnabled(!checked);
    ui->random_btn->setEnabled(!checked);
}

void SettingDialog::on_refresh_btn_clicked()
{
    InitInfo::updateNetDeviceInfo(&dialog_info);
    initIpValue();
}

void SettingDialog::on_random_btn_clicked()
{
    quint32 random_port = 0;

    do{
        random_port = qrand() % 65536;
    }while(random_port <= 1024);

    ui->port_box->setValue(random_port);
}

void SettingDialog::initLayout()
{
    auto *desktop = QApplication::desktop();
    setGeometry(0,0,(desktop->width() / 2) - 50, (desktop->height() / 2) + 100);
    move(QPoint((desktop->width() - width()) / 2, (desktop->height() - height()) / 2));
}

void SettingDialog::initIpValue()
{
    ui->ip_combox->clear();
    ui->ip_error->setVisible(false);

    auto address = dialog_info.getHostIps();
    int pos = 0, eq_pos = -1;
    foreach(auto value, address){
        if(!dialog_info.getIpAddr().isEmpty() && (value.toString() == dialog_info.getIpAddr())){
            eq_pos = pos;
        }
        ui->ip_combox->addItem(value.toString());
        pos++;
    }

    if(eq_pos >= 0){
        ui->ip_combox->setCurrentIndex(eq_pos);
        dialog_info.setIpIndex(eq_pos);
    }else{
        dialog_info.setIpIndex(ui->ip_combox->currentIndex());
        dialog_info.setIpPort(dialog_info.getIsSystem() ? 0 : ui->port_box->value());
        dialog_info.setIpAddr(ui->ip_combox->currentText());
    }
    dialog_info.setIpPort(dialog_info.getIsSystem() ? 0 : ui->port_box->value());

    if(!ui->ip_combox->count()){
        ui->ip_error->setVisible(true);
    }
}

void SettingDialog::initLinkValue()
{
    ui->port_box->setValue(dialog_info.getIpPort());
    ui->lport_box->setValue(dialog_info.getIpLinkPort());

    ui->frame_box->setValue(dialog_info.getInfoFrame());
    ui->delay_box->setValue(dialog_info.getInfoDelay());
    ui->timeout_box->setValue(dialog_info.getInfoTimeout());
}

void SettingDialog::initCameraInfo()
{
    const QList<QCameraInfo> &infos = media_info.getMediaCameraInfo();
    const QCameraInfo &dinfo = media_info.getMediaCameraDefaultInfo();

    if(media_info.getMediaCameraInfo().size() <= 0){
        return;
    }

    int pos = 0, index = 0;
    disconnect(ui->camera_drive_box, SIGNAL(currentIndexChanged(int)), this, SLOT(on_camera_drive_box_currentIndexChanged(int)));
    ui->camera_drive_box->clear();
    ui->camera_format_box->clear();
    ui->camera_resolution_box->clear();

    foreach (const QCameraInfo &info, infos) {
        if(info.position() == QCamera::Position::FrontFace){
            ui->camera_drive_box->addItem(front_value);
        }else if(info.position() == QCamera::Position::BackFace){
            ui->camera_drive_box->addItem(back_value);
        }else {
            ui->camera_drive_box->addItem(default_value);
        }
        if(info == dinfo){
            ui->camera_drive_box->setCurrentIndex((index = pos));
        }
        ++pos;
    }
    connect(ui->camera_drive_box, SIGNAL(currentIndexChanged(int)), this, SLOT(on_camera_drive_box_currentIndexChanged(int)));

emit    onCallbackInitCamera(infos);

    foreach (const auto &value, media_info.camera_format_values) {
        ui->camera_format_box->addItem(value);
    }

    {
        QAbstractItemModel *mode = ui->camera_format_box->model();
        QVideoFrame::PixelFormat format;
        QModelIndex index;

        for(int i = 1, size = mode->rowCount(); i < size; i++){
            index = mode->index(i, 0);
            format = onIndexToPixelFormat(i, media_info.camera_format_values.size());
            if(QVideoFrame::imageFormatFromPixelFormat(format) != QImage::Format::Format_Invalid){
                mode->setData(index, QVariant(1 | 32), Qt::UserRole - 1);
            }else{
                if((format != QVideoFrame::Format_Jpeg) && (format != QVideoFrame::Format_BGRA32) && (format != QVideoFrame::Format_BGRA32_Premultiplied) &&
                   (format != QVideoFrame::Format_BGR32) && (format != QVideoFrame::Format_BGR24) && (format != QVideoFrame::Format_BGR565) &&
                   (format != QVideoFrame::Format_BGR555) && (format != QVideoFrame::Format_AYUV444) && (format != QVideoFrame::Format_YUV444) &&
                   (format != QVideoFrame::Format_YUV420P) && (format != QVideoFrame::Format_YV12) && (format != QVideoFrame::Format_UYVY) &&
                   (format != QVideoFrame::Format_YUYV) && (format != QVideoFrame::Format_NV12) && (format != QVideoFrame::Format_NV21)){
                        mode->setData(index, QVariant(0), Qt::UserRole - 1);
                }else{
                        mode->setData(index, QVariant(1 | 32), Qt::UserRole - 1);
                }
            }
        }

        ui->camera_format_box->setCurrentIndex(onPixelFormatToIndex(media_info.getMediaCameraFormat(), media_info.camera_format_values.size()));
        ui->camera_resolution_box->addItem(QString{"0*0"});
        ui->camera_ratio_box->setValue(media_info.getMediaCameraRatio().width());
    }

    updateCameraDeriveInfo0(dinfo, index);
}

void SettingDialog::initAudioInfo(QComboBox *device_box, QComboBox *codec_box, QComboBox *channel_box, QComboBox *rate_box, QComboBox *size_box,
                                  const QList<QAudioDeviceInfo> &infos, const QAudioDeviceInfo &dinfo, QAudio::Mode mode)
{
    if(infos.size() <= 0){
        return;
    }

    int pos = 0, index = 0;
    disconnect(device_box, SIGNAL(currentIndexChanged(int)),
               this, mode == QAudio::AudioInput ? SLOT(on_audioinput_drive_box_currentIndexChanged(int)) : SLOT(on_audiooutput_drive_box_currentIndexChanged(int)));
    device_box->clear();

    foreach (const QAudioDeviceInfo &info, infos) {
        device_box->addItem(info.deviceName());
        if(info == dinfo){
            device_box->setCurrentIndex((index = pos));
        }
        ++pos;
    }
    connect(device_box, SIGNAL(currentIndexChanged(int)),
               this, mode == QAudio::AudioInput ? SLOT(on_audioinput_drive_box_currentIndexChanged(int)) : SLOT(on_audiooutput_drive_box_currentIndexChanged(int)));

emit onCallbackInitAudio(infos, mode);
    updateAudioDeriveInfo0(codec_box, channel_box, rate_box, size_box, dinfo, mode, index);
}

void SettingDialog::initErrorMap()
{
    ui->ip_error->setErrorPixmap(QString{":image/icons/delete1.bmp"});
}

void SettingDialog::onCameraInit(const MediaControl::CameraControlHolder &holder)
{
    if(holder.onCameraError() == LOCAL_VIDEO_ERROR_INVALID_FORMAT){
        onShowFormatWarnint(onSetCameraType(), holder.onCameraPos());
    }
}

void SettingDialog::onAudioInit(const MediaControl::AudioControlHolder&)
{

}

int SettingDialog::onCameraBoxChange(int index)
{
    int pos = media_info.getMediaCameraInfo().indexOf(media_info.getMediaCameraSelectInfo());
    ui->camera_drive_box->setCurrentIndex(index);
    return pos;
}

void SettingDialog::onAudioInputBoxChange(int index)
{
    ui->audioinput_drive_box->setCurrentIndex(index);
}

void SettingDialog::onAudioOutputBoxChange(int index)
{
    ui->audiooutput_drive_box->setCurrentIndex(index);
}

std::pair<sockaddr_in, sockaddr_in> SettingDialog::onSettingIpInfo()
{
    sockaddr_in local_addr, link_addr;
    local_addr.sin_port = dialog_info.getIpPort();
    local_addr.sin_addr.s_addr = IpEditor::onIpStringToValue(dialog_info.getIpAddr());

    link_addr.sin_port = dialog_info.getIpLinkPort();
    link_addr.sin_addr.s_addr = IpEditor::onIpStringToValue(dialog_info.getIpLink());

    return {local_addr, link_addr};
}

std::tuple<uint32_t, uint32_t, uint32_t> SettingDialog::onSettingLinkInfo()
{
    return std::make_tuple(dialog_info.getInfoFrame(), dialog_info.getInfoDelay(), dialog_info.getInfoTimeout());
}

std::pair<const QCameraInfo&, CameraFormatInfo> SettingDialog::onSettingCameraInfo()
{
    return { media_info.getMediaCameraSelectInfo(),
             CameraFormatInfo{media_info.getMediaCameraFormat(), dialog_info.getInfoFrame(), media_info.getMediaCameraRatio()} };
}

std::pair<const QAudioDeviceInfo&, QAudioFormat> SettingDialog::onSettingAudioinfo(QAudio::Mode mode)
{
    QAudioFormat format;
    if(mode == QAudio::AudioInput){
        format = media_info.getMediaAudioInputSelectInfo().preferredFormat();
        format.setCodec(media_info.getMediaAudioInputCodec().first);
        format.setChannelCount(media_info.getMediaAudioInputChannel().first);
        format.setSampleSize(media_info.getMediaAudioInputSampleSize().first);
        format.setSampleRate(media_info.getMediaAudioInputSampleRate().first);
        return { media_info.getMediaAudioInputSelectInfo(), format };
    }else{
        format = media_info.getMediaAudioOutputSelectInfo().preferredFormat();
        format.setCodec(media_info.getMediaAudioOutputCodec().first);
        format.setChannelCount(media_info.getMediaAudioOutputChannel().first);
        format.setSampleSize(media_info.getMediaAudioOutputSampleSize().first);
        format.setSampleRate(media_info.getMediaAudioOutputSampleRate().first);
        return { media_info.getMediaAudioOutputSelectInfo(), format };
    }
}

void SettingDialog::updateCameraDeriveInfo0(const QCameraInfo &info, int pos)
{
    int opos = media_info.getMediaCameraInfo().indexOf(media_info.getMediaCameraSelectInfo());
    media_info.setMediaCameraInfo(info);
emit    onCameraUpdate(info, pos, opos);
}

void SettingDialog::updateAudioDeriveInfo0(QComboBox *codec_box, QComboBox *channel_box, QComboBox *rate_box, QComboBox *size_box, const QAudioDeviceInfo &info, QAudio::Mode mode, int pos)
{
    QAudioFormat info_format = info.preferredFormat();
    const QList<QString> &codecs = info.supportedCodecs();
    const QList<int> &channels = info.supportedChannelCounts();
    const QList<int> &rates = info.supportedSampleRates();
    const QList<int> &sizes = info.supportedSampleSizes();

    codec_box->clear();
    channel_box->clear();
    rate_box->clear();
    size_box->clear();

    onAudioBoxInit<QList<QString>, QString>(codec_box, codecs,
                   [&](const QString &value) -> std::pair<bool, QString> {
                        return {(value == info_format.codec()), value};
                 });
    onAudioBoxInit<QList<int>, int>(channel_box, channels,
                   [&](const int &value) -> std::pair<bool, QString>{
                        return {(value == info_format.channelCount()), QString::number(value)};
                });
    onAudioBoxInit<QList<int>, int>(rate_box, rates,
                   [&](const int &value) -> std::pair<bool, QString> {
                        return {(value == info_format.sampleRate()), QString::asprintf("%dHz", value)};
                });
    onAudioBoxInit<QList<int>, int>(size_box, sizes,
                   [&](const int &value) -> std::pair<bool, QString> {
                        return {(value == info_format.sampleSize()), QString::number(value)};
                });
    if(mode == QAudio::AudioInput){
        media_info.setMediaAudioInputInfo(info);
        media_info.setMediaAudioInputCodec(info_format.codec(), codec_box->currentIndex());
        media_info.setMediaAudioInputChannel(info_format.channelCount(), channel_box->currentIndex());
        media_info.setMediaAudioInputSampleRate(info_format.sampleRate(), rate_box->currentIndex());
        media_info.setMediaAudioInputSampleSize(info_format.sampleSize(), size_box->currentIndex());
    }else{
        media_info.setMediaAudioOutputInfo(info);
        media_info.setMediaAudioOutputCodec(info_format.codec(), codec_box->currentIndex());
        media_info.setMediaAudioOutputChannel(info_format.channelCount(), channel_box->currentIndex());
        media_info.setMediaAudioOutputSampleRate(info_format.sampleRate(), rate_box->currentIndex());
        media_info.setMediaAudioOutputSampleSize(info_format.sampleSize(), size_box->currentIndex());
    }
emit    onAudioUpdate(info, mode, pos);
}

void SettingDialog::on_camera_drive_box_currentIndexChanged(int index)
{
    if(index >= media_info.getMediaCameraInfo().size()){
        return;
    }
    updateCameraDeriveInfo0(media_info.getMediaCameraInfo().at(index), index);
}

void SettingDialog::on_audioinput_drive_box_currentIndexChanged(int index)
{
    if(index >= media_info.getMediaAudioInputInfo().size()){
        return;
    }

    int pos = media_info.getMediaAudioInputInfo().indexOf(media_info.getMediaAudioInputSelectInfo());

    QAudioFormat format = onSettingAudioinfo(QAudio::AudioInput).second;
    if(!media_info.getMediaAudioInputInfo().at(index).isFormatSupported(format)){
        if(show_flag & SETTING_DIALOG_SHOW_MATCH_AUDIOINPUT_FORMAT){
            return onShowFormatWarnint(onSetAudioType(), pos);
        }else if(state_type == SET_STATE_TYPE_COMFIRM){
emit        onAudioFormatWarning(QAudio::AudioInput, pos);
            return;
        }
    }

    updateAudioDeriveInfo0(ui->audioinput_codec_box, ui->audioinput_channel_box, ui->audioinput_rate_box, ui->audioinput_size_box,
                          media_info.getMediaAudioInputInfo().at(index), QAudio::AudioInput, index);
}

void SettingDialog::on_audiooutput_drive_box_currentIndexChanged(int index)
{
    if(index >= media_info.getMediaAudioOutputInfo().size()) {
        return;
    }

    int pos = media_info.getMediaAudioOutputInfo().indexOf(media_info.getMediaAudioOutputSelectInfo());

    QAudioFormat format = onSettingAudioinfo(QAudio::AudioOutput).second;
    if(!media_info.getMediaAudioOutputInfo().at(index).isFormatSupported(format)){
        if(show_flag & SETTING_DIALOG_SHOW_MATCH_AUDIOOUTPUT_FORMAT){
            //暂没有实现
            return onShowFormatWarnint(onSetAudioType(), pos);
        }else if(state_type == SET_STATE_TYPE_COMFIRM){
emit        onAudioFormatWarning(QAudio::AudioOutput, pos);
        }
    }

    updateAudioDeriveInfo0(ui->audiooutput_codec_box, ui->audiooutput_channel_box, ui->audiooutput_rate_box, ui->audiooutput_size_box,
                          media_info.getMediaAudioOutputInfo().at(index), QAudio::AudioOutput, index);
}


void SettingDialog::rectButton()
{
    int margin_rigth = 10;
    const auto &rect_bottom = ui->box_bottom->geometry();
    const auto &rect_btn = ui->splitter_btn->sizeHint();

    ui->splitter_btn->setGeometry(QRect{ rect_bottom.width() - rect_btn.width() - margin_rigth,
                                        (rect_bottom.height() - rect_btn.height()) / 2,
                                         rect_btn.width(), rect_btn.height() });
}

void SettingDialog::rectTabLayout()
{    
    auto *current_widget = ui->tabWidget->currentWidget();
    if(current_widget == ui->tab_ip){
        auto local_geometry = ui->tab_ip->geometry();
        auto link_geometry = ui->tab_ip->geometry();

        int height = ui->tab_ip->geometry().height() >> 1;

        local_geometry.setHeight(height);
        link_geometry.setY(height);
        link_geometry.setHeight(height);

        ui->iplocal_box->setGeometry(local_geometry);
        ui->iplink_box->setGeometry(link_geometry);
    } else if(current_widget == ui->tab_info){
        ui->infoset_box->setGeometry(ui->tab_info->geometry());
    } else if(current_widget == ui->tab_camera){
        ui->cameraset_box->setGeometry(ui->tab_camera->geometry());
    } else if(current_widget == ui->tab_audio){
        auto input_geometry = ui->tab_audio->geometry();
        auto output_geometry = ui->tab_audio->geometry();

        int height = ui->tab_audio->geometry().height() >> 1;

        input_geometry.setHeight(height);
        output_geometry.setY(height);
        output_geometry.setHeight(height);

        ui->audioinput_box->setGeometry(input_geometry);
        ui->audiooutput_box->setGeometry(output_geometry);
    }
}

void SettingDialog::onIpPageChange()
{
    ui->ip_btn->setEnabled(false);
    ui->info_btn->setEnabled(true);
    ui->camera_btn->setEnabled(true);
    ui->audio_btn->setEnabled(true);

    auto local_geometry = ui->tab_ip->geometry();
    auto link_geometry = ui->tab_ip->geometry();

    int height = ui->tab_ip->geometry().height() >> 1;

    local_geometry.setHeight(height);
    link_geometry.setY(height);
    link_geometry.setHeight(height);

    ui->iplocal_box->setGeometry(local_geometry);
    ui->iplink_box->setGeometry(link_geometry);
}

void SettingDialog::onInfoPageChange()
{
    ui->info_btn->setEnabled(false);
    ui->ip_btn->setEnabled(true);
    ui->camera_btn->setEnabled(true);
    ui->audio_btn->setEnabled(true);

    ui->infoset_box->setGeometry(ui->tab_info->geometry());
}

void SettingDialog::onCameraPageChange()
{
    ui->camera_btn->setEnabled(false);
    ui->ip_btn->setEnabled(true);
    ui->info_btn->setEnabled(true);
    ui->audio_btn->setEnabled(true);

    ui->cameraset_box->setGeometry(ui->tab_camera->geometry());
}

void SettingDialog::onAudioPageChange()
{
    ui->audio_btn->setEnabled(false);
    ui->camera_btn->setEnabled(true);
    ui->ip_btn->setEnabled(true);
    ui->info_btn->setEnabled(true);

    auto input_geometry = ui->tab_audio->geometry();
    auto output_geometry = ui->tab_audio->geometry();

    int height = ui->tab_audio->geometry().height() >> 1;

    input_geometry.setHeight(height);
    output_geometry.setY(height);
    output_geometry.setHeight(height);

    ui->audioinput_box->setGeometry(input_geometry);
    ui->audiooutput_box->setGeometry(output_geometry);
}

void SettingDialog::onStoreValue()
{
    dialog_info.setIsSystem(ui->sys_check->isChecked());
    dialog_info.setInfoFrame(static_cast<quint32>(ui->frame_box->value()));
    dialog_info.setInfoDelay(static_cast<quint32>(ui->delay_box->value()));
    dialog_info.setInfoTimeout(static_cast<quint32>(ui->timeout_box->value()));
    dialog_info.setIpPort(static_cast<quint32>(ui->port_box->value()));
    dialog_info.setIpLinkPort(static_cast<quint32>(ui->lport_box->value()));
    dialog_info.setIpIndex(ui->ip_combox->currentIndex());
    dialog_info.setIpAddr(ui->ip_combox->currentText());
    dialog_info.setIpLink(ui->ip_link->text());

    int ratio = ui->camera_ratio_box->value();
    media_info.setMediaCameraRatio(QSize{ratio, ratio});
    media_info.setMediaCameraFormat(onIndexToPixelFormat(ui->camera_format_box->currentIndex(), media_info.camera_format_values.size()));
    media_info.setMediaCameraResolution(onAnalysisResolution(ui->camera_resolution_box->currentText()));
    media_info.setMediaCameraInfo(media_info.getMediaCameraInfo().at(ui->camera_drive_box->currentIndex()));

    media_info.setMediaAudioInputCodec(ui->audioinput_codec_box->currentText(), ui->audioinput_codec_box->currentIndex());
    media_info.setMediaAudioInputChannel(ui->audioinput_channel_box->currentText().toInt(), ui->audioinput_channel_box->currentIndex());
    media_info.setMediaAudioInputSampleSize(ui->audioinput_size_box->currentText().toInt(), ui->audioinput_size_box->currentIndex());
    media_info.setMediaAudioInputSampleRate(onAnalysisSampleRate(ui->audioinput_rate_box->currentText()), ui->audioinput_rate_box->currentIndex());
    media_info.setMediaAudioInputInfo(media_info.getMediaAudioInputInfo().at(ui->audioinput_drive_box->currentIndex()));

    media_info.setMediaAudioOutputCodec(ui->audiooutput_codec_box->currentText(), ui->audiooutput_codec_box->currentIndex());
    media_info.setMediaAudioOutputChannel(ui->audiooutput_channel_box->currentText().toInt(), ui->audiooutput_channel_box->currentIndex());
    media_info.setMediaAudioOutputSampleSize(ui->audiooutput_size_box->currentText().toInt(), ui->audiooutput_size_box->currentIndex());
    media_info.setMediaAudioOutputSampleRate(onAnalysisSampleRate(ui->audiooutput_rate_box->currentText()), ui->audiooutput_rate_box->currentIndex());
    media_info.setMediaAudioOutputInfo(media_info.getMediaAudioOutputInfo().at(ui->audiooutput_drive_box->currentIndex()));
}

void SettingDialog::onStoreValue(const IpLinkInfo &linfo, const CameraFormatInfo &cformat, const QAudioFormat &iformat, const QAudioFormat &oformat)
{
    dialog_info.setInfoFrame(linfo.getLinkFrame());
    dialog_info.setInfoDelay(linfo.getLinkDelay());
    dialog_info.setInfoTimeout(linfo.getLinkTimeout());
    dialog_info.setIpLinkPort(linfo.getLinkPort());
    dialog_info.setIpLink(IpEditor::onIpValueToString(linfo.getLinkIp()));

    media_info.setMediaCameraRatio(cformat.getCameraRatio());
    media_info.setMediaCameraFormat(cformat.getCameraFormat());

    media_info.setMediaAudioInputCodec(iformat.codec(), 0);
    media_info.setMediaAudioInputChannel(iformat.channelCount(), 0);
    media_info.setMediaAudioInputSampleSize(iformat.sampleSize(), 0);
    media_info.setMediaAudioInputSampleRate(iformat.sampleRate(), 0);

    media_info.setMediaAudioOutputCodec(oformat.codec(), 0);
    media_info.setMediaAudioOutputChannel(oformat.channelCount(), 0);
    media_info.setMediaAudioOutputSampleSize(oformat.sampleSize(), 0);
    media_info.setMediaAudioOutputSampleRate(oformat.sampleRate(), 0);

    ui->ip_link->setText(IpEditor::onIpValueToString(linfo.getLinkIp()));
    ui->lport_box->setValue(linfo.getLinkPort());
    ui->frame_box->setValue(linfo.getLinkFrame());
    ui->delay_box->setValue(linfo.getLinkDelay());
    ui->timeout_box->setValue(linfo.getLinkTimeout());
}

void SettingDialog::onLoadValue()
{
    bool is_system = dialog_info.getIsSystem();
    ui->sys_check->setChecked(is_system);
    ui->port_box->setEnabled(!is_system);
    ui->random_btn->setEnabled(!is_system);

    ui->frame_box->setValue(static_cast<int>(dialog_info.getInfoFrame()));
    ui->delay_box->setValue(static_cast<int>(dialog_info.getInfoDelay()));
    ui->timeout_box->setValue(static_cast<int>(dialog_info.getInfoTimeout()));
    ui->lport_box->setValue(static_cast<int>(dialog_info.getIpPort()));
    ui->port_box->setValue(static_cast<int>(dialog_info.getIpLinkPort()));
    ui->ip_link->setText(dialog_info.getIpLink());

    ui->ip_combox->setCurrentIndex(dialog_info.getIpIndex());

    ui->camera_drive_box->setCurrentIndex(media_info.getMediaCameraInfo().indexOf(media_info.getMediaCameraSelectInfo()));
    ui->camera_format_box->setCurrentIndex(onPixelFormatToIndex(media_info.getMediaCameraFormat(), media_info.camera_format_values.size()));
//    ui->camera_resolution_box->setCurrentIndex();
    ui->camera_ratio_box->setValue(media_info.getMediaCameraRatio().width());

    ui->audioinput_drive_box->setCurrentIndex(media_info.getMediaAudioInputInfo().indexOf(media_info.getMediaAudioInputSelectInfo()));
    ui->audioinput_codec_box->setCurrentIndex(media_info.getMediaAudioInputCodec().second);
    ui->audioinput_channel_box->setCurrentIndex(media_info.getMediaAudioInputChannel().second);
    ui->audioinput_rate_box->setCurrentIndex(media_info.getMediaAudioInputSampleRate().second);
    ui->audioinput_size_box->setCurrentIndex(media_info.getMediaAudioInputSampleSize().second);

    ui->audiooutput_drive_box->setCurrentIndex(media_info.getMediaAudioOutputInfo().indexOf(media_info.getMediaAudioOutputSelectInfo()));
    ui->audiooutput_codec_box->setCurrentIndex(media_info.getMediaAudioOutputCodec().second);
    ui->audiooutput_channel_box->setCurrentIndex(media_info.getMediaAudioOutputChannel().second);
    ui->audiooutput_rate_box->setCurrentIndex(media_info.getMediaAudioOutputSampleRate().second);
    ui->audiooutput_size_box->setCurrentIndex(media_info.getMediaAudioOutputSampleSize().second);
}

void SettingDialog::onEnabledValue(bool enabled)
{
    ui->btn_reset->setEnabled(enabled);
    ui->sys_check->setEnabled(enabled);

    ui->ip_combox->setEnabled(enabled);
    ui->port_box->setEnabled(enabled);
    ui->random_btn->setEnabled(enabled);
    ui->refresh_btn->setEnabled(enabled);
    ui->ip_link->setEnabled(enabled);
    ui->lport_box->setEnabled(enabled);
    ui->iplink_btn->setEnabled(enabled);

    ui->frame_box->setEnabled(enabled);
    ui->delay_box->setEnabled(enabled);
    ui->timeout_box->setEnabled(enabled);

    ui->camera_drive_box->setEnabled(enabled);
    ui->camera_format_box->setEnabled(enabled);
    ui->camera_ratio_box->setEnabled(enabled);
    ui->camera_resolution_box->setEnabled(enabled);

    ui->audioinput_drive_box->setEnabled(enabled);
    ui->audioinput_codec_box->setEnabled(enabled);
    ui->audioinput_channel_box->setEnabled(enabled);
    ui->audioinput_rate_box->setEnabled(enabled);
    ui->audioinput_size_box->setEnabled(enabled);

    ui->audiooutput_drive_box->setEnabled(enabled);
    ui->audiooutput_codec_box->setEnabled(enabled);
    ui->audiooutput_channel_box->setEnabled(enabled);
    ui->audiooutput_rate_box->setEnabled(enabled);
    ui->audiooutput_size_box->setEnabled(enabled);
}

void SettingDialog::onResetValue()
{
    dialog_info.clear();
    media_info.clear();
}

void SettingDialog::onResetValue(const CameraFormatInfo &cformat, const QAudioFormat &iformat, const QAudioFormat &oformat)
{
    ui->camera_format_box->clear();
    ui->camera_format_box->addItem(media_info.camera_format_values.at(onPixelFormatToIndex(cformat.getCameraFormat(), media_info.camera_format_values.size())));

    ui->audioinput_codec_box->clear();
    ui->audioinput_channel_box->clear();
    ui->audioinput_size_box->clear();
    ui->audioinput_rate_box->clear();
    ui->audioinput_codec_box->addItem(iformat.codec());
    ui->audioinput_channel_box->addItem(QString::number(iformat.channelCount()));
    ui->audioinput_size_box->addItem(QString::number(iformat.sampleSize()));
    ui->audioinput_rate_box->addItem(QString::number(iformat.sampleRate()));

    ui->audiooutput_codec_box->clear();
    ui->audiooutput_channel_box->clear();
    ui->audiooutput_size_box->clear();
    ui->audiooutput_rate_box->clear();
    ui->audiooutput_codec_box->addItem(oformat.codec());
    ui->audiooutput_channel_box->addItem(QString::number(oformat.channelCount()));
    ui->audiooutput_size_box->addItem(QString::number(oformat.sampleSize()));
    ui->audiooutput_rate_box->addItem(QString::number(oformat.sampleRate()));
}

void SettingDialog::on_iplink_btn_clicked()
{
    if(!dialog_info.getIpLink().isEmpty()){
        ip_editor.setIpValue(dialog_info.getIpLink());
    }

    if(!ip_editor.isVisible()){
        ip_editor.show();
    }
}

void SettingDialog::on_set_ip_confirm()
{
    QString ip_value = ip_editor.getIpValue();
    ui->ip_link->setText(ip_value);
}

