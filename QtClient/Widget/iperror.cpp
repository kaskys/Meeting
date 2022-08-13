#include "iperror.h"

IpError::IpError(QWidget *parent) : QWidget(parent), map_value(), error_value()
{
    setMouseTracking(true);
    initErrorValue();
}

IpError::~IpError()
{

}

void IpError::initErrorValue()
{
    error_value.setText("没有外出IP地址!");
    error_value.setMargin(5);
    error_value.setStyleSheet("background-color: rgb(250, 250, 250); border: 1px solid rgb(0,0,0); border-radius:15px");
    error_value.setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
}

void IpError::setErrorPixmap(const QString &map)
{
    map_value = QPixmap(map);
    update();
}

void IpError::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter{this};
    this->width(); this->height();
    painter.drawPixmap(0, 0 , map_value);
}

void IpError::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);
    const auto &rect = this->geometry();
    static QSize error_rect = error_value.sizeHint();
    const int width_margin = -10, heigth_margin = 10;


    error_value.setParent(qobject_cast<QWidget*>(parent()));
    error_value.setGeometry(QRect{ rect.x() + rect.width() + width_margin,
                                   rect.y() + rect.height() + heigth_margin,
                                   error_rect.width(), error_rect.height()
                                 });
    error_value.show();
}

void IpError::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    error_value.setParent(nullptr);
    error_value.hide();
}

void IpError::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    error_value.setParent(nullptr);
    error_value.close();
}
