#include "localvideowidget.h"

LocalVideoWidget::LocalVideoWidget(QWidget *parent) : QVideoWidget(parent), is_immediately_start(false)
{

}

LocalVideoWidget::~LocalVideoWidget()
{

}

void LocalVideoWidget::onCameraStateChanged(QCamera::State state)
{
    if(state == QCamera::LoadedState){
        if(is_immediately_start){
emit    onLocalVideoImmediatelyStart();
        }
    }
}

void LocalVideoWidget::onCameraError(QCamera::Error error)
{
    is_immediately_start = false;
emit    onLocalVideoError(error);
}
