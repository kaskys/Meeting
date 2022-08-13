#ifndef LOCALVIEDWIDGET_H
#define LOCALVIEDWIDGET_H

#include "../Class/Control/mediacontrol.h"

class LocalVideoWidget : public QVideoWidget
{
    Q_OBJECT
public:
    explicit LocalVideoWidget(QWidget *parent = nullptr);
    ~LocalVideoWidget() Q_DECL_OVERRIDE;

    void setVideoWidgetImmediately(bool is_immediately) { is_immediately_start = is_immediately; }
    bool getVideoWidgetImmediately() const { return is_immediately_start;}    
signals:
    void onLocalVideoImmediatelyStart();
    void onLocalVideoError(QCamera::Error);
public slots:
    void onCameraStateChanged(QCamera::State);
    void onCameraError(QCamera::Error);
private:
    bool is_immediately_start;
};

#endif // LOCALVIEDWIDGET_H
