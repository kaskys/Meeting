#ifndef LOCALMEDIAWIDGET_H
#define LOCALMEDIAWIDGET_H

#include <QApplication>
#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QShowEvent>
#include <QTimerEvent>
#include <QTimer>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QCursor>
#include <QMenu>
#include <QRect>
#include <QLayout>
#include <QDebug>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>

#include <list>
#include <functional>

#include "../Class/Control/mediacontrol.h"


#define CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN   8
#define CUSTOM_VIDEO_WIDGET_MIN_WIDTH_SIZE   200
#define CUSTOM_VIDEO_WIDGET_MIN_HEIGHT_SIZE  200
#define CUSTOM_VIDEO_WIDGET_MAX_WIDTH_SIZE   1000
#define CUSTOM_VIDEO_WIDGET_MAX_HEIGHT_SIZE  1000

enum CustomLayoutItemMoveDir{
    ITEM_MOVE_DIR_LEFT = 1,
    ITEM_MOVE_DIR_LEFT_UP,
    ITEM_MOVE_DIR_RIGHT,
    ITEM_MOVE_DIR_RIGHT_DOWN
};

enum ActionTriggerType{
    ACTION_TRIGGER_TYPE_NONE = 1,
    ACTION_TRIGGER_TYPE_QUIT,
    ACTION_TRIGGER_TYPE_CONTRAPOSITION_DONE,
    ACTION_TRIGGER_TYPE_CONTRAPOSITION_MOVE,
    ACTION_TRIGGER_TYPE_FULL,
    ACTION_TRIGGER_TYPE_MOVE,
    ACTION_TRIGGER_TYPE_SELECT
};

enum LocalMediaWidgetType{
    LOCAL_MEDIA_WIDGET_TYPE_NONE = 1,
    LOCAL_MEDIA_WIDGET_TYPE_QUIT,
    LOCAL_MEDIA_WIDGET_TYPE_CONTRAPOSITION_DONE,
    LOCAL_MEDIA_WIDGET_TYPE_CONTRAPOSITION_MOVE,
    LOCAL_MEDIA_WIDGET_TYPE_FULL,
    LOCAL_MEDIA_WIDGET_TYPE_MOVE,
    LOCAL_MEDIA_WIDGET_TYPE_SELECT
};

class LocalMediaWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QRect widget_geometry READ widgetGeometry WRITE setWidgetGeometry)
public:
    explicit LocalMediaWidget(QWidget *parent = nullptr);
    ~LocalMediaWidget() Q_DECL_OVERRIDE;

    bool operator>(const LocalMediaWidget &widget){
        if(this->widget_position.first > widget.widget_position.first){
            return true;
        }else if(this->widget_position.first == widget.widget_position.first){
            return (this->widget_position.second > widget.widget_position.second);
        }else{
            return false;
        }
    }
    bool operator>=(const LocalMediaWidget &widget){
        if((this->widget_position.first == widget.widget_position.first)  &&
           (this->widget_position.second == widget.widget_position.second)){
            return true;
        }else{
            return this->operator>(widget);
        }
    }

    static int onWidgetMinWidthSize() { return CUSTOM_VIDEO_WIDGET_MIN_WIDTH_SIZE; }
    static int onWidgetMinHeightSize() { return CUSTOM_VIDEO_WIDGET_MIN_HEIGHT_SIZE; }
    static QSize onWidgetMinSize() { return QSize{CUSTOM_VIDEO_WIDGET_MIN_WIDTH_SIZE, CUSTOM_VIDEO_WIDGET_MIN_HEIGHT_SIZE}; }
    static QSize onWidgetMaxSize() { return QSize{CUSTOM_VIDEO_WIDGET_MAX_WIDTH_SIZE, CUSTOM_VIDEO_WIDGET_MAX_HEIGHT_SIZE}; }

    void setGeometry(int x, int y, int w, int h){
        focus_rect = QRect{x, y, w, h};
        setGeometry0();
    }
    void setGeometry(const QRect &rect){
        focus_rect = rect;
        setGeometry0();
    }

    QRect widgetGeometry() const {
        return widget_rect;
    }
    void setWidgetGeometry(const QRect &rect) {
        focus_rect = QRect{rect.x() - CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN, rect.y() - CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN,
                           rect.width() + (CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN << 1),
                           rect.height() + (CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN << 1)};
        QWidget::setGeometry((widget_rect = rect));
    }

    void setWidgetPosition(std::pair<int, int> value){
        this->widget_position = value;
    }
    void setWidgetPosition(int row, int column){
        this->widget_position = {row, column};
    }
    void updateLeftPosition(){
        widget_position.second++;
    }
    void updateRightPosition(){
        widget_position.second--;
    }
    void updateLeftUpPosition(){
        widget_position.first++;
        widget_position.second = 1;
    }
    void updateRightDownPosition(int column){
        widget_position.first--;
        widget_position.second = column;
    }

    std::pair<int, int> getWidgetPosition() const{
        return widget_position;
    }

    LocalMediaWidgetType onWidgetOperatorType() const {
        return static_cast<LocalMediaWidgetType>(trigger_type);
    }

    bool isUpgrade(CustomLayoutItemMoveDir dir, int column) const {
        if(((dir == ITEM_MOVE_DIR_RIGHT) && (widget_position.second == 1)) ||
           ((dir == ITEM_MOVE_DIR_LEFT) && (widget_position.second == column))){
            return true;
        }else{
            return false;
        }

    }
    void correlateAuduiControl(const MediaControl::AudioControlHolder&);

    void correlateLayoutItem(QLayoutItem *item) { this->correlate_item = item; }
    QLayoutItem* correlateLayoutItem() const { return correlate_item; }

    void setWidgetIterator(const std::list<QLayoutItem*>::iterator& iterator){
        this->widget_iterator = iterator;
    }
    const std::list<QLayoutItem*>::iterator& getWidgetIterator() const& {
        return widget_iterator;
    }
    std::list<QLayoutItem*>::iterator getWidgetIterator() &&{
        return std::move(widget_iterator);
    }

    bool isWidgetMove() const { return (trigger_type != ACTION_TRIGGER_TYPE_CONTRAPOSITION_DONE); }
    void onLocalClear();
    void onWidgetMoveEnter();
    void onWidgetMoveLeave();
    void onWidgetMoveInSide();
    void onWidgetMoveOutSide();


    QAbstractAnimation* onWidgetMoveAnimation(CustomLayoutItemMoveDir, const QPoint&, int);
    void onWidgetQuitAnimation();

    static std::pair<int, int> onPosition(int rank, int column){
        int wc = rank % column;
        if(!wc){
            return {(rank / column) - 1, column};
        }else{
            return {rank / column , wc};
        }
    }
    static std::pair<int, int> onPosition(LocalMediaWidget *widget, int column){
        int wc = widget->widget_position.second <= 0  ? column : widget->widget_position.second - 1;
        int wr = widget->widget_position.first - (wc >= column) ? 1 : 0;
        return {wr, wc};
    }

    void onInitInfo(uint32_t pos, uint32_t port, uint32_t addr) {}
    void onUpdateStatus(uint32_t pos, const char *status, const char *addr) {}
    void onUpdateMedia(const char *buffer, uint32_t len, const std::function<void()> &release_func) {}
signals:
    void on_widget_quit();
    void on_widget_contraposition();
    void on_widget_contraposition(int, int);
    void on_widget_finish();
    void on_widget_mstart();
    void on_widget_move();
    void on_widget_mend();
    void on_widget_select();
public slots:
    void on_custom_context_menu_rqeuest(const QPoint&);
    void on_move_timeout();

    void on_full_action();
    void on_move_action();
    void on_quit_action();
    void on_contraposition_action();

    void on_widget_quit_finish();
protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void enterEvent(QEvent *event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void moveEvent(QMoveEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
private:
    void setGeometry0(){
        widget_rect = QRect{focus_rect.x() + CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN, focus_rect.y() + CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN,
                            focus_rect.width() - (CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN << 1),
                            focus_rect.height() - (CUSTOM_VIDEO_WIDGET_DEFAULT_MARGIN << 1) };
        QWidget::setGeometry(widget_rect);
    }

    void onWidgetMoveShow();
    void onWidgetMoveClose();

    bool is_move_widget;
    bool is_quit_widget;

    int widget_margin;
    int widget_move_value;
    ActionTriggerType trigger_type;

    QLayoutItem *correlate_item;
    std::pair<int, int> widget_position;
    std::list<QLayoutItem*>::iterator widget_iterator;

    QRect widget_rect;
    QRect focus_rect;
    QTimer move_timer;
    QMenu widget_menu;

    QColor color;

    QSequentialAnimationGroup animation_group;
    QPropertyAnimation move_animation;
    QPropertyAnimation quit_animation;

    MediaControl::AudioControlHolder audio_holder;
};

#endif // LOCALMEDIAWIDGET_H
