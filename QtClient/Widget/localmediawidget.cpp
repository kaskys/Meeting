#include "localmediawidget.h"

LocalMediaWidget::LocalMediaWidget(QWidget *parent) : QWidget(parent), is_move_widget(false), is_quit_widget(false), widget_margin(CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN), widget_move_value(0),
    trigger_type(ACTION_TRIGGER_TYPE_NONE), correlate_item(nullptr), widget_position(0, 0), widget_iterator(),
    widget_rect(), move_timer(), widget_menu(), color(255, 0, 0), animation_group(), move_animation(this, "widget_geometry"), quit_animation(this, "widget_geometry"), audio_holder(nullptr)
{
    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setMinimumSize(onWidgetMinSize());
    setMaximumSize(onWidgetMaxSize());

    widget_menu.setFixedWidth(100);
    connect(widget_menu.addAction("全屏"), SIGNAL(triggered()), this, SLOT(on_full_action()));
    connect(widget_menu.addAction("移动"), SIGNAL(triggered()), this, SLOT(on_move_action()));
    connect(widget_menu.addAction("关闭"), SIGNAL(triggered()), this, SLOT(on_quit_action()));
    connect(widget_menu.addAction("对位"), SIGNAL(triggered()), this, SLOT(on_contraposition_action()));

    connect(&move_timer, SIGNAL(timeout()), this, SLOT(on_move_timeout()));
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(on_custom_context_menu_rqeuest(const QPoint&)));
}

LocalMediaWidget::~LocalMediaWidget()
{
    new (&animation_group) QSequentialAnimationGroup();
}

void LocalMediaWidget::correlateAuduiControl(const MediaControl::AudioControlHolder &holder)
{
    if(!holder.isValid()) return;
    audio_holder = holder;
    audio_holder.setUpdateVolumeFunc([&](qreal value) -> void {

    });
}

void LocalMediaWidget::onLocalClear()
{
    is_move_widget = false;
    is_quit_widget = false;
    trigger_type = ACTION_TRIGGER_TYPE_NONE;
    widget_move_value = 0;
    widget_position = {0, 0};
    new (&animation_group) QSequentialAnimationGroup();
}

void LocalMediaWidget::onWidgetMoveEnter()
{
    trigger_type = ACTION_TRIGGER_TYPE_SELECT;
    if(move_timer.isActive()){
        move_timer.stop();
    }
    move_timer.setInterval(1000);
    move_timer.start();
    this->setStyleSheet("border-left: 1px solid red; border-right: 1px solid red; border-top: 1px solid red;border-bottom: 1px solid red;");
}

void LocalMediaWidget::onWidgetMoveLeave()
{
    trigger_type = ACTION_TRIGGER_TYPE_NONE;
    if(move_timer.isActive()){
        move_timer.stop();
    }
    this->setStyleSheet("");
}

void LocalMediaWidget::onWidgetMoveInSide()
{
    trigger_type = ACTION_TRIGGER_TYPE_MOVE;
    if(move_timer.isActive()){
        move_timer.stop();
    }
}

void LocalMediaWidget::onWidgetMoveOutSide()
{
    if((trigger_type == ACTION_TRIGGER_TYPE_CONTRAPOSITION_MOVE) || (trigger_type == ACTION_TRIGGER_TYPE_CONTRAPOSITION_DONE)) return;

    trigger_type = ACTION_TRIGGER_TYPE_CONTRAPOSITION_MOVE;
    if(move_timer.isActive()){
        move_timer.stop();
    }
    widget_move_value = 0;
    move_timer.setInterval(100);
    move_timer.start();
}

QAbstractAnimation *LocalMediaWidget::onWidgetMoveAnimation(CustomLayoutItemMoveDir dir, const QPoint &npoint, int right_margin)
{
    disconnect(&quit_animation, SIGNAL(finished()), this, SLOT(on_widget_quit_finish()));
    if((dir == ITEM_MOVE_DIR_LEFT) || (dir == ITEM_MOVE_DIR_RIGHT)){
        new (&move_animation) QPropertyAnimation(this, "widget_geometry");

        move_animation.setDuration(1000);
        move_animation.setStartValue(widget_rect);
        move_animation.setEndValue(QRect{npoint.x(), npoint.y(), widget_rect.width(), widget_rect.height()});
        return &move_animation;
    }else{
        new (&animation_group) QSequentialAnimationGroup();
        new (&move_animation) QPropertyAnimation(this, "widget_geometry");
        new (&quit_animation) QPropertyAnimation(this, "widget_geometry");

        move_animation.setDuration(500);
        quit_animation.setDuration(500);

        if(dir == ITEM_MOVE_DIR_RIGHT_DOWN){
            move_animation.setStartValue(widget_rect);
            move_animation.setEndValue(QRect{0 - widget_rect.width() , widget_rect.y() , widget_rect.width(), widget_rect.height()});

            quit_animation.setStartValue(QRect{npoint.x() + (widget_rect.width() << 1) + right_margin,
                                               npoint.y(), widget_rect.width(), widget_rect.height()});
            quit_animation.setEndValue(QRect{npoint.x() + CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN , npoint.y(),
                                             widget_rect.width(), widget_rect.height()});
        }else {
            move_animation.setStartValue(widget_rect);
            move_animation.setEndValue(QRect{npoint.x(), widget_rect.y(), widget_rect.width(), widget_rect.height()});

            quit_animation.setStartValue(QRect{0 - widget_rect.width(), npoint.y(), widget_rect.width(), widget_rect.height()});
            quit_animation.setEndValue(QRect{0 + right_margin + CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN, npoint.y(), widget_rect.width(), widget_rect.height()});
        }

        animation_group.addAnimation(&move_animation);
        animation_group.addAnimation(&quit_animation);
        return &animation_group;
    }
}

void LocalMediaWidget::onWidgetQuitAnimation()
{
    is_quit_widget = true;

    new (&quit_animation) QPropertyAnimation(this, "widget_geometry");
    quit_animation.setDuration(500);
    quit_animation.setStartValue(widget_rect);
    quit_animation.setEndValue(QRect{widget_rect.x() + (widget_rect.width() / 2),
                                 widget_rect.y() + (widget_rect.height() / 2),
                                 0, 0});
    connect(&quit_animation, SIGNAL(finished()), this, SLOT(on_widget_quit_finish()));
    this->setMinimumSize({0, 0});
    quit_animation.start();
}

void LocalMediaWidget::on_custom_context_menu_rqeuest(const QPoint &point)
{
    Q_UNUSED(point);
    trigger_type = ACTION_TRIGGER_TYPE_NONE;

    widget_menu.exec(QCursor::pos());

    if(trigger_type == ACTION_TRIGGER_TYPE_FULL){

    }else if(trigger_type == ACTION_TRIGGER_TYPE_MOVE){
        onWidgetMoveShow();
    }else if(trigger_type == ACTION_TRIGGER_TYPE_QUIT){
emit on_widget_quit();
    }else if(trigger_type == ACTION_TRIGGER_TYPE_CONTRAPOSITION_DONE){
emit on_widget_contraposition();
    }
}

void LocalMediaWidget::on_move_timeout()
{
    if(trigger_type == ACTION_TRIGGER_TYPE_NONE){
        if((widget_move_value += 100) > 1000){
            onWidgetMoveShow();
        }else{
            update();
        }
    }else if(trigger_type == ACTION_TRIGGER_TYPE_SELECT){
emit on_widget_select();
    }else if(trigger_type == ACTION_TRIGGER_TYPE_CONTRAPOSITION_MOVE){
        if((widget_move_value += 100) >= 1000){
            trigger_type = ACTION_TRIGGER_TYPE_CONTRAPOSITION_DONE;
            move_timer.stop();
        }
emit on_widget_contraposition(widget_move_value, 1000);
    }

}

void LocalMediaWidget::on_full_action()
{
    trigger_type = ACTION_TRIGGER_TYPE_FULL;
}

void LocalMediaWidget::on_move_action()
{
    trigger_type = ACTION_TRIGGER_TYPE_MOVE;
}

void LocalMediaWidget::on_quit_action()
{
    trigger_type = ACTION_TRIGGER_TYPE_QUIT;
}

void LocalMediaWidget::on_contraposition_action()
{
    trigger_type = ACTION_TRIGGER_TYPE_CONTRAPOSITION_DONE;
}

void LocalMediaWidget::on_widget_quit_finish()
{
emit on_widget_finish();
}

void LocalMediaWidget::timerEvent(QTimerEvent *event)
{
    QObject::timerEvent(event);
}

void LocalMediaWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if(is_quit_widget){ return; }

    if(is_move_widget){
        onWidgetMoveClose();
    }else{
        if(event->button() == Qt::LeftButton){
            move_timer.setInterval(100);
            move_timer.start();
        }else if(event->button() == Qt::RightButton){
            //show menu
        }
    }
}

void LocalMediaWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    if(is_quit_widget){ return; }

    if(event->button() == Qt::LeftButton){
        if(is_move_widget){
            onWidgetMoveClose();
        }else{
            move_timer.stop();
            widget_move_value = 0;
        }
    }else if(event->button() == Qt::RightButton){
        //close menu
    }
}

void LocalMediaWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
    if(is_quit_widget){ return; }

    onWidgetMoveShow();
}

void LocalMediaWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if(is_quit_widget){ return; }

    if(is_move_widget){
        this->move(QCursor::pos().x() - (widget_rect.width() >> 1), QCursor::pos().y() - (widget_rect.height() >> 1));
emit    on_widget_move();
    }
}

void LocalMediaWidget::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);

    if(is_quit_widget){ return; }

    if(!is_move_widget){
        QWidget::setGeometry(focus_rect);
    }
}

void LocalMediaWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if(is_quit_widget){ return; }

    if(!is_move_widget){
        QWidget::setGeometry(widget_rect);
    }else{
        this->move(QCursor::pos().x() - (widget_rect.width() >> 1), QCursor::pos().y() - (widget_rect.height() >> 1));
    }
}

void LocalMediaWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter{this};
    this->width(); this->height();

    painter.fillRect(QRect{0, 0, this->geometry().width(), this->geometry().height()}, color);

    QPen pen = painter.pen();
    pen.setWidth(5);
    pen.setColor(Qt::black);
    pen.setStyle(Qt::SolidLine);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);



    if(widget_move_value <= 0){
        return;
    } else if(widget_move_value <= 500){
        QPoint spoint1 = QPoint{0, 0};
        QPoint epoint1 = QPoint{this->geometry().width() * (static_cast<float>(widget_move_value) / 500), 0};

        QPoint spoint2 = QPoint(this->geometry().width(), this->geometry().height());
        QPoint epoint2 = QPoint(this->geometry().width() * (1 - (static_cast<float>(widget_move_value) / 500)), this->geometry().height());

        painter.drawLine(spoint1, epoint1);
        painter.drawLine(spoint2, epoint2);
    }else{
        QPoint spoint1 = QPoint{0, 0};
        QPoint epoint1 = QPoint{this->geometry().width() , 0};

        QPoint spoint2 = QPoint(this->geometry().width(), this->geometry().height());
        QPoint epoint2 = QPoint(this->geometry().width() * (1 - (static_cast<float>(widget_move_value) / 500)), this->geometry().height());

        painter.drawLine(spoint1, epoint1);
        painter.drawLine(spoint2, epoint2);

        spoint1 = QPoint{this->geometry().width(), 0};
        epoint1 = QPoint{this->geometry().width(), this->geometry().height() * (static_cast<float>(widget_move_value - 500) / 500)};

        spoint2 = QPoint(0, this->geometry().height());
        epoint2 = QPoint(0, this->geometry().height() * (1 - (static_cast<float>(widget_move_value - 500) / 500)));

        painter.drawLine(spoint1, epoint1);
        painter.drawLine(spoint2, epoint2);
    }
}

void LocalMediaWidget::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
}

void LocalMediaWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void LocalMediaWidget::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
}

void LocalMediaWidget::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
}

void LocalMediaWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void LocalMediaWidget::onWidgetMoveShow()
{
    is_move_widget = true;
    move_timer.stop();
    widget_move_value = 0;
    trigger_type = ACTION_TRIGGER_TYPE_MOVE;

emit on_widget_mstart();

    color = QColor{0, 0, 255};
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    this->grabMouse();
    this->update();
    this->show();
    this->move(QCursor::pos().x() - (widget_rect.width() >> 1), QCursor::pos().y() - (widget_rect.height() >> 1));
}

void LocalMediaWidget::onWidgetMoveClose()
{
    is_move_widget = false;
    if(move_timer.isActive()){
        move_timer.stop();
    }

    if((trigger_type == ACTION_TRIGGER_TYPE_MOVE) || (trigger_type == ACTION_TRIGGER_TYPE_CONTRAPOSITION_MOVE)){
emit on_widget_mend();
    }else if(trigger_type == ACTION_TRIGGER_TYPE_CONTRAPOSITION_DONE){
emit on_widget_contraposition();
    }

    trigger_type = ACTION_TRIGGER_TYPE_NONE;
    QWidget::setGeometry(widget_rect);
    this->setWindowFlags(~(Qt::Window | Qt::FramelessWindowHint));
    this->releaseMouse();
    this->show();
}
