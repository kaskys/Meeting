#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QCloseEvent>

#include <mutex>
#include <functional>

#include "Class/Adapter/meetingadapter.h"
#include "Layout/localwidgetlayout.h"
#include "Ui/aboutdialog.h"
#include "Ui/settingdialog.h"
#include "Ui/filterdialog.h"
#include "Class/Control/mediacontrol.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setAdapter(MeetingAdapter*);
protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *event) Q_DECL_OVERRIDE;
private slots:
    void on_action_launch_triggered();

    void on_action_about_triggered();

    void on_action_setting_triggered();

    void on_actionserver_mode_triggered();

    void on_actionclient_mode_triggered();

    void on_action_blackList_triggered();

    void on_action_whiteList_triggered();

    void on_action_linkInfo_triggered();

    void on_action_time_triggered();

    void on_action_linke_triggered();

    void on_action_status_triggered();

    void on_setting_confirm();
    void on_setting_camera(const QCameraInfo&, int, int);
    void on_setting_audio(const QAudioDeviceInfo&, QAudio::Mode, int);
    void on_setting_format_warning(QAudio::Mode, int);
    void on_update_camera(const QList<QCameraInfo>&);
    void on_update_audio(const QList<QAudioDeviceInfo>&, QAudio::Mode);
    void on_camera_box_currentIndexChanged(int index);

    void on_audio_box_currentIndexChanged(int index);

    void on_audioinput_slider_valueChanged(int value);

    void on_audiooutput_slider_valueChanged(int value);

    void on_video_button_clicked();

    void on_audio_button_clicked();

    void on_update_contraposition_widget(int);
    void on_clear_contraposition_widget();
    void on_correlate_contraposition(LocalMediaWidget*);

    void on_launch_complete(IpInfo, TimeqInfo, MediaqInfo);
    void on_launch_fail();
    void on_remote_note_init(void*, uint32_t, uint32_t, uint32_t);
    void on_remote_note_update(uint32_t, const char*, const char*, void*);
    void on_remote_note_media(const char*, uint32_t, void*);
    void on_remote_note_indicator(void*, const char*);
private:
    static std::once_flag init_flag;
    static void onGlobalInit(SettingDialog*, FilterDialog*, QLabel*);

    void onLaunchClientComplete(const IpInfo&, const TimeqInfo&, const MediaqInfo&);
    void onLaunchServerComplete(const IpInfo&, const TimeqInfo&, const MediaqInfo&);

    void onUpdateCamera(int);
    void onUpdateAudio(QAudio::Mode);

    Ui::MainWindow *ui;
    MeetingAdapter *meeting_adapter;
    MediaControl *media_control;
    LocalWidgetLayout *media_layout;

    AboutDialog about_dialog;
    SettingDialog set_dialog;
    FilterDialog filter_dialog;
};

#endif // MAINWINDOW_H
