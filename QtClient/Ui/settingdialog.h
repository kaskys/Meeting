#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QWidget>
#include <QComboBox>
#include <QDesktopWidget>
#include <QResizeEvent>
#include <QSplitter>
#include <QMessageBox>

#include <functional>

#include "Widget/iperror.h"
#include "ipeditor.h"
#include "../Class/Init/initinfo.h"
#include "../Class/Control/mediacontrol.h"

#define SETTING_DIALOG_SHOW_MATCH_CAMERA_FORMAT         0x01
#define SETTING_DIALOG_SHOW_MATCH_AUDIOINPUT_FORMAT     0x02
#define SETTING_DIALOG_SHOW_MATCH_AUDIOOUTPUT_FORMAT    0x04

namespace Ui {
class SettingDialog;
}

class SettingDialog;

enum SetStateType{
    SET_STATE_TYPE_INIT,
    SET_STATE_TYPE_COMFIRM,
    SET_STATE_TYPE_START,
    SET_STATE_TYPE_SERVANT,
    SET_STATE_TYPE_REPEAT
};

class SetCameraType{
    friend class SettingDialog;

    explicit SetCameraType(SettingDialog *dialog) : sdialog(dialog){}
    ~SetCameraType() = default;

    void onDisconnent() const;
    void onRollBack(int);
    void onConnent() const;

    SettingDialog *sdialog;
};

class SetAudioType{
    friend class SettingDialog;

    explicit SetAudioType(SettingDialog *dialog) : sdialog(dialog){}
    ~SetAudioType() = default;

    void onDisconnent() const;
    void onRollBack(int);
    void onConnent() const;

    SettingDialog *sdialog;
};

class SettingDialog : public QWidget
{
    friend class SetCameraType;
    friend class SetAudioType;

    Q_OBJECT

public:
    explicit SettingDialog(QWidget *parent = 0);
    ~SettingDialog();

    void onGlobalInit(QLabel*);
    void onServantInit(const IpLinkInfo&, const CameraFormatInfo&, const QAudioFormat&, const QAudioFormat&);

    bool isSystemGeneration() const { return  dialog_info.getIsSystem(); }
    quint32 getInfoFrame() const { return dialog_info.getInfoFrame(); }
    quint32 getInfoDelay() const { return dialog_info.getInfoDelay(); }
    quint32 getInfoTimeout() const { return dialog_info.getInfoTimeout(); }
    quint32 getIpPort() const { return dialog_info.getIpPort(); }
    quint32 getIpLinkPort() const { return dialog_info.getIpLinkPort(); }
    QString getIpAddr() const { return dialog_info.getIpAddr(); }
    QString getIpLink() const { return dialog_info.getIpLink(); }

    const SettingInfo& getSettInfo() const { return dialog_info; }
    const MediaInfo& getMediaInfo() const { return media_info; }
    SetCameraType& onSetCameraType() { return camera_type; }
    SetAudioType& onSetAudioType() { return audio_type; }

    SetStateType onSettingType() const { return state_type; }
    uint32_t getShowFlag() const { return show_flag; }
    void setShowFlag(uint32_t flag) { show_flag = flag; }
    void orShowFlag(uint32_t flag) { show_flag |= flag; }
    void clrShowFlag(uint32_t flag) { show_flag &= (~flag); }

    template <typename Type> void onShowFormatWarnint(Type &type, int pos){
        if(!isVisible()) return;
        type.onDisconnent();
        type.onRollBack(pos);
        QMessageBox::warning(this, "警告", "格式不兼容，请停止再选择！", QMessageBox::Ok, QMessageBox::Cancel);
        type.onConnent();
    }
    void onSettingShow(uint32_t);
    void onSettingStart();
    void onSettingStop();
    void onCameraInit(const MediaControl::CameraControlHolder&);
    void onAudioInit(const MediaControl::AudioControlHolder&);

    int onCameraBoxChange(int);
    void onAudioInputBoxChange(int);
    void onAudioOutputBoxChange(int);

    std::pair<sockaddr_in, sockaddr_in> onSettingIpInfo();
    std::tuple<uint32_t, uint32_t, uint32_t> onSettingLinkInfo();
    std::pair<const QCameraInfo&, CameraFormatInfo> onSettingCameraInfo();
    std::pair<const QAudioDeviceInfo&, QAudioFormat> onSettingAudioinfo(QAudio::Mode);
Q_SIGNALS:
    void onSettingConfirm();
    void onCameraUpdate(const QCameraInfo&, int, int);
    void onAudioUpdate(const QAudioDeviceInfo&, QAudio::Mode, int);
    void onAudioFormatWarning(QAudio::Mode, int);

    void onCallbackInitCamera(const QList<QCameraInfo>&);
    void onCallbackInitAudio(const QList<QAudioDeviceInfo>&, QAudio::Mode);
protected:
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
private slots:
    void on_ip_btn_clicked();

    void on_info_btn_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_btn_ok_clicked();
    void on_btn_reset_clicked();

    void on_sys_check_clicked(bool);

    void on_refresh_btn_clicked();

    void on_random_btn_clicked();

    void on_iplink_btn_clicked();

    void on_set_ip_confirm();
    void on_camera_btn_clicked();
    void on_audio_btn_clicked();

    void on_camera_drive_box_currentIndexChanged(int index);
    void on_audioinput_drive_box_currentIndexChanged(int index);
    void on_audiooutput_drive_box_currentIndexChanged(int index);
public:
    const static QString default_value;
    const static QString back_value;
    const static QString front_value;
private:
    static QVideoFrame::PixelFormat onIndexToPixelFormat(int index, int size){
        if(index == (size - 1)){ return QVideoFrame::PixelFormat::Format_User; }
        else{ return static_cast<QVideoFrame::PixelFormat>(index + 1); }
    }
    static int onPixelFormatToIndex(QVideoFrame::PixelFormat format, int size){
        if(format == QVideoFrame::Format_User){ return size - 1; }
        else { return static_cast<int>(format) - 1; }
    }
    static QSize onAnalysisResolution(const QString&){
        return {0, 0};
    }
    static int onAnalysisSampleRate(const QString &value){
        static const QString hz{"Hz"};
        return value.left(value.indexOf(hz)).toInt();
    }

    template<typename Container, typename Type> static void onAudioBoxInit(QComboBox *box, const Container &container, const std::function<std::pair<bool, QString>(const Type&)> &func){
        int pos = 0;
        std::pair<bool, QString> init_value;
        foreach (const Type &value, container) {
            init_value = func(value);
            box->addItem(init_value.second);

            if(init_value.first){
                box->setCurrentIndex(pos);
            }
            pos++;
        }
    }

    void initLayout();
    void initIpValue();
    void initLinkValue();
    void initCameraInfo();
    void initAudioInfo(QComboBox*, QComboBox*, QComboBox*, QComboBox*, QComboBox*,  const QList<QAudioDeviceInfo>&, const QAudioDeviceInfo&, QAudio::Mode);
    void initErrorMap();

    void updateCameraDeriveInfo0(const QCameraInfo&, int);
    void updateAudioDeriveInfo0(QComboBox*, QComboBox*, QComboBox*, QComboBox*, const QAudioDeviceInfo&, QAudio::Mode, int);

    void rectButton();
    void rectTabLayout();

    void onIpPageChange();
    void onInfoPageChange();
    void onCameraPageChange();
    void onAudioPageChange();

    void onStoreValue();
    void onStoreValue(const IpLinkInfo&, const CameraFormatInfo&, const QAudioFormat&, const QAudioFormat&);
    void onLoadValue();
    void onEnabledValue(bool);
    void onResetValue();
    void onResetValue(const CameraFormatInfo&,const QAudioFormat&, const QAudioFormat&);

    static SettingInfo dialog_info;
    static MediaInfo media_info;

    Ui::SettingDialog *ui;

    SetCameraType camera_type;
    SetAudioType audio_type;

    SetStateType state_type;
    uint32_t show_flag;
    IpEditor ip_editor;
};

#endif // SETTINGDIALOG_H
