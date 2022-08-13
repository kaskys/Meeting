#ifndef LOCALWIDGETLAYOUT_H
#define LOCALWIDGETLAYOUT_H


#include <QPushButton>

#include "../Widget/localmediawidget.h"

#define CUSTOM_LAYOUT_LAYOUT_COLUMN_DEFAULT_SIZE    4
#define CUSTOM_LAYOUT_LAYOUT_COLUMN_MIN_SIZE        2
#define CUSTOM_LAYOUT_LAYOUT_COLUMN_MAX_SIZE        12

class LocalWidgetLayout : public QLayout
{
    Q_OBJECT
public:
    explicit LocalWidgetLayout(QWidget *parent = nullptr);
    ~LocalWidgetLayout() Q_DECL_OVERRIDE;

    QSize minimumSize() const Q_DECL_OVERRIDE { return QLayout::minimumSize(); }
    QSize maximumSize() const Q_DECL_OVERRIDE { return QLayout::maximumSize(); }
    Qt::Orientations expandingDirections() const Q_DECL_OVERRIDE { return QLayout::expandingDirections(); }
    bool isEmpty() const Q_DECL_OVERRIDE { return (!user_item && layout_items.empty()); }

    QSize sizeHint() const Q_DECL_OVERRIDE;
    void setGeometry(const QRect&) Q_DECL_OVERRIDE;
    QRect geometry() const Q_DECL_OVERRIDE;

    void addWidget(QWidget*w);
    void addItem(QLayoutItem*) Q_DECL_OVERRIDE;
    QLayoutItem *itemAt(int index) const Q_DECL_OVERRIDE;
    QLayoutItem *takeAt(int index) Q_DECL_OVERRIDE;
    int count() const Q_DECL_OVERRIDE { return layout_items.size() + 1; }

    int getLayoutItemColumn() const { return layout_item_column; }
    int minLayoutItemColumn() const { return CUSTOM_LAYOUT_LAYOUT_COLUMN_MIN_SIZE; }
    int maxLayoutItemColumn() const { return CUSTOM_LAYOUT_LAYOUT_COLUMN_MAX_SIZE; }

    void setCoord(const QPoint &point){
        layout_rect.moveTo(point);
    }
    void setArea(const QSize &size){
        setGeometry(QRect{0, 0, size.width(), size.height()});
    }
    void setLayoutItemColumn(int column);
    void onLayoutClose();

    void unCorrelateWidgetContraposition(LocalMediaWidget*);
signals:
    void onUpdateWidgetContraposition(int);
    void onClearWidgetContraposition();
    void onCorrelateWigetContraposition(LocalMediaWidget*);
public slots:
    void on_button_clicked();
    void on_item_mstart();
    void on_item_move();
    void on_item_contraposition();
    void on_item_contraposition(int, int);
    void on_item_mend();
    void on_item_select();
    void on_item_close();
    void on_itme_finish();
    void on_move_finish();
protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
private:
    QWidget* onFindItemAt(const QPoint&);

    void resetMoveLayout();
    void onMoveLayout(std::list<QLayoutItem*>::iterator, std::list<QLayoutItem*>::iterator, CustomLayoutItemMoveDir, bool is_connect = false);
    QAbstractAnimation* onMoveItem(std::list<QLayoutItem*>::iterator&, CustomLayoutItemMoveDir);
    void onReleaseItem(QWidget*);

    bool update_layout;
    int layout_item_row;
    int layout_item_column;
    int layout_item_number;
    int layout_item_margin_top;
    int layout_item_margin_left;

    std::list<QLayoutItem*>::iterator old_item_iterator;
    std::list<QLayoutItem*>::iterator pre_item_iterator;
    std::list<QLayoutItem*>::iterator new_item_iterator;

    QSize item_min;
    QSize item_max;
    QSize item_avg;
    QRect layout_rect;

    QWidget *select_item;
    QLayoutItem *user_item;
    QParallelAnimationGroup move_animation_group;
    std::list<QLayoutItem*> layout_items;
};

#endif // LOCALWIDGETLAYOUT_H
