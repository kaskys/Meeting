#include "localwidgetlayout.h"

LocalWidgetLayout::LocalWidgetLayout(QWidget *parent) : QLayout(parent), update_layout(false), layout_item_row(1),
    layout_item_column(CUSTOM_LAYOUT_LAYOUT_COLUMN_DEFAULT_SIZE), layout_item_number(CUSTOM_LAYOUT_LAYOUT_COLUMN_DEFAULT_SIZE),
    layout_item_margin_top(0), layout_item_margin_left(0),
    old_item_iterator(), pre_item_iterator(), new_item_iterator(),
    item_min(LocalMediaWidget::onWidgetMinSize()), item_max(LocalMediaWidget::onWidgetMaxSize()),
    item_avg(0, 0) , layout_rect(0, 0, 0, 0),  select_item(nullptr), user_item(nullptr),  move_animation_group(), layout_items()
{
    user_item = new QWidgetItem(new QPushButton(parentWidget()));

    old_item_iterator = layout_items.end();
    pre_item_iterator = layout_items.end();
    new_item_iterator = layout_items.end();

    qobject_cast<QPushButton*>(user_item->widget())->setStyleSheet("background-color:red");
    qobject_cast<QPushButton*>(user_item->widget())->setMinimumSize(QSize{30, 30});
    qobject_cast<QPushButton*>(user_item->widget())->setMaximumSize(QSize{50, 50});

    connect(user_item->widget(), SIGNAL(clicked()), this, SLOT(on_button_clicked()));
}

LocalWidgetLayout::~LocalWidgetLayout()
{
    new (&move_animation_group) QParallelAnimationGroup();
    delete user_item;
    foreach (auto item, layout_items) {
        delete item;
    }
}

QSize LocalWidgetLayout::sizeHint() const
{
    QSize item_size = user_item->sizeHint();

    if((item_avg.width() <= 0) || (item_avg.height() <= 0)){
        return item_size;
    }else{
        return QSize{item_avg.width() * qMin(layout_item_column, static_cast<int>(layout_items.size())), item_avg.height() * layout_item_row};
    }
}

void LocalWidgetLayout::setGeometry(const QRect &rect)
{
#define CUSTOM_LAYOUT_GEOMETRY_MIN_UPDATE_UPPER   2
#define CUSTOM_LAYOUT_GEOMETRY_MIN_UPDATE_LOWER   -2
    static std::function<int(int, int, int)> value_func = [=](int new_size, int old_size, int number) -> int{
        int update_value = new_size - old_size;
        if((update_value <= CUSTOM_LAYOUT_GEOMETRY_MIN_UPDATE_UPPER) &&
           (update_value >= CUSTOM_LAYOUT_GEOMETRY_MIN_UPDATE_LOWER)){
            return 0;
        }
        return update_value / number;
    };
    if((rect.width() == layout_rect.width()) && (rect.height() == layout_rect.height()) && !update_layout){
        return;
    }

    int update_width_value = 0, update_height_value = 0;
    int twidth = rect.width(), theight = rect.height();
    int column_number =  0;
    QSize item_size = item_avg;
    QLayoutItem *widget_item = nullptr;

    if(!layout_items.size()){
        column_number = 1;
    }else{
        column_number = qMin(static_cast<int>(layout_items.size()), layout_item_column);
    }

    if(twidth == layout_rect.width()){
        if(!(update_height_value = value_func(theight, layout_rect.height(), layout_item_row)) && !update_layout){
            return;
        }
    }else if(theight == layout_rect.height()){
        if(!(update_width_value = value_func(twidth, layout_rect.width(), column_number)) && !update_layout){
            return;
        }
    }else{
        update_width_value = value_func(twidth, layout_rect.width(), column_number);
        update_height_value = value_func(theight, layout_rect.height(), layout_item_row);
    }

    if(update_width_value || update_layout){
        item_avg.setWidth(item_avg.width() + update_width_value);

        if((item_avg.width() >= item_min.width()) && (item_avg.width() <= item_max.width())){
            item_size.setWidth(item_avg.width());
            layout_item_margin_left = 0;
        }else if(item_avg.width() > item_max.width()){
            item_avg.setWidth(item_max.width());
            layout_item_margin_left = (item_avg.width() - item_max.width()) >> 1;
        }else{
            layout_item_margin_left = 0;
        }

        layout_rect.setWidth(layout_rect.width() + (update_width_value * column_number));
    }

    if(update_height_value || update_layout){
        item_avg.setHeight(item_avg.height() + update_height_value);

        if((item_avg.height() >= item_min.height()) && (item_avg.height() <= item_max.height())){
            item_size.setHeight(item_avg.height());
            layout_item_margin_top = 0;
        }else if(item_avg.height() > item_max.height()){
            item_avg.setHeight(item_max.height());
            layout_item_margin_top = (item_avg.height() - item_max.height()) >> 1;
        }else{
            layout_item_margin_top = 0;
        }
        layout_rect.setHeight(layout_rect.height() + (update_height_value * layout_item_row));
    }


    for(int column = 0, row = 0, pos = 0; pos < layout_items.size(); pos++){
        widget_item = itemAt(pos + 1);

        qobject_cast<LocalMediaWidget*>(widget_item->widget())
                ->setGeometry(QRect{(column * item_avg.width()) + layout_item_margin_left,
                                    (row * item_avg.height()) + layout_item_margin_top,
                                     item_size.width(), item_size.height()});
        if((++column) >= layout_item_column){
            column = 0;
            row++;
        }
    }

    user_item->setGeometry(QRect{((twidth - user_item->widget()->width()) >> 1) + layout_item_margin_left,
                                 theight - user_item->widget()->height(),
                                 user_item->widget()->width(), user_item->widget()->height()});
    update_layout = false;
}

QRect LocalWidgetLayout::geometry() const
{
    return layout_rect;
}

void LocalWidgetLayout::addWidget(QWidget *widget)
{
    addItem(new QWidgetItem(widget));
}

void LocalWidgetLayout::addItem(QLayoutItem *item)
{
    bool fixed_layout = false;
    int min_width = 0,  min_height = 0;
    LocalMediaWidget *cwidget = qobject_cast<LocalMediaWidget*>(item->widget());
    cwidget->correlateLayoutItem(item);
    cwidget->onLocalClear();

    auto iterator = layout_items.insert(layout_items.end(), item);

    connect(item->widget(), SIGNAL(on_widget_mstart()), this, SLOT(on_item_mstart()));
    connect(item->widget(), SIGNAL(on_widget_mend()), this, SLOT(on_item_mend()));
    connect(item->widget(), SIGNAL(on_widget_move()), this, SLOT(on_item_move()));
    connect(item->widget(), SIGNAL(on_widget_contraposition()), this, SLOT(on_item_contraposition()));
    connect(item->widget(), SIGNAL(on_widget_contraposition(int, int)), this, SLOT(on_item_contraposition(int, int)));
    connect(item->widget(), SIGNAL(on_widget_select()), this, SLOT(on_item_select()));
    connect(item->widget(), SIGNAL(on_widget_quit()), this, SLOT(on_item_close()));
    connect(item->widget(), SIGNAL(on_widget_finish()), this, SLOT(on_itme_finish()));


    if(layout_items.size() <= layout_item_column){
        item_avg.setWidth(qMax(layout_rect.width() / static_cast<int>(layout_items.size()), LocalMediaWidget::onWidgetMinWidthSize()));

        if(layout_rect.width() < (min_width = item_avg.width() * layout_items.size())){
            fixed_layout = true;
            layout_rect.setWidth(min_width);
        }
    }

    if((++layout_item_number) > layout_item_column){
        layout_item_row++;
        item_avg.setHeight(qMax(layout_rect.height() / layout_item_row, LocalMediaWidget::onWidgetMinHeightSize()));
        layout_item_number -= layout_item_column;

        if(layout_rect.height() < (min_height = item_avg.height() * layout_item_row)){
            fixed_layout = true;
            layout_rect.setHeight(min_height);
        }
    }

    update_layout = true;
    parentWidget()->setGeometry(layout_rect);
    if(fixed_layout){
        parentWidget()->setMinimumSize(QSize{layout_rect.width() + 30, layout_rect.height() + 10});
    }

    cwidget->setWidgetIterator(iterator);
    cwidget->setWidgetPosition(layout_item_row - 2, layout_item_number);
    cwidget->show();
}

QLayoutItem *LocalWidgetLayout::itemAt(int index) const
{
    if((index < 0) || (index >= count())){ return nullptr; }
    if(index == 0) { return user_item; }
    auto begin = layout_items.begin();

    for(int pos = index - 1; pos > 0; --pos){
        ++begin;
    }

    return (*begin);
}

QLayoutItem* LocalWidgetLayout::takeAt(int index)
{
    if((index < 0) || (index >= count())){ return nullptr; }
    if(index == 0) { return user_item; }

    auto begin = layout_items.begin();
    QLayoutItem *item = nullptr;

    for(int pos = index - 1; pos > 0; --pos){
        ++begin;
    }
    item = (*begin);
    layout_items.erase(begin);
    return item;
}

void LocalWidgetLayout::setLayoutItemColumn(int column)
{
    if((column < CUSTOM_LAYOUT_LAYOUT_COLUMN_MIN_SIZE) || (column > CUSTOM_LAYOUT_LAYOUT_COLUMN_MAX_SIZE)){
        return;
    }
    bool fixed_layout = false;
    int need_column = qMin(column, static_cast<int>(layout_items.size())),
        need_row = 1 + (layout_items.size() / column) + ((layout_items.size() % column) ? 1 : 0);
    int min_width = need_column * LocalMediaWidget::onWidgetMinWidthSize(),
        min_height = need_row * LocalMediaWidget::onWidgetMinHeightSize();

    update_layout = true;
    item_avg = QSize(qMax(layout_rect.width() / (this->layout_item_column = column), LocalMediaWidget::onWidgetMinWidthSize()),
                     qMax(layout_rect.height() / (this->layout_item_row = need_row), LocalMediaWidget::onWidgetMinHeightSize()));

    if(layout_rect.width() < min_width){
        fixed_layout = true;
        layout_rect.setWidth(min_width);
    }
    if(layout_rect.height() < min_height){
        fixed_layout = true;
        layout_rect.setHeight(min_height);
    }

    auto iterator = layout_items.begin();
    for(int i = 0, size = layout_items.size(), item_row = 0, item_column = 0; i < size; ++iterator, ++i){
        qobject_cast<LocalMediaWidget*>((*iterator)->widget())->setWidgetPosition(item_row, ++item_column);

        if(item_column >= column){
            ++item_row;
            item_column = 0;
        }
    }

    layout_rect.setWidth(layout_rect.width() + 20);
    layout_rect.setHeight(layout_rect.height() + 20);
    parentWidget()->setGeometry(layout_rect);
    if(fixed_layout){
        parentWidget()->setMinimumSize(QSize{layout_rect.width() + 20, layout_rect.height() + 20});
    }
}

void LocalWidgetLayout::onLayoutClose()
{

}

void LocalWidgetLayout::unCorrelateWidgetContraposition(LocalMediaWidget *lwidget)
{
    if(!lwidget) return;
    if(!lwidget->correlateLayoutItem()){
        addWidget(lwidget);
    }else{
        addItem(lwidget->correlateLayoutItem());
    }
}

void LocalWidgetLayout::on_button_clicked()
{
    LocalMediaWidget *widget = new LocalMediaWidget(parentWidget());
    addWidget(widget);
}

void LocalWidgetLayout::on_item_mstart()
{
    new_item_iterator = old_item_iterator = qobject_cast<LocalMediaWidget*>(sender())->getWidgetIterator();
    ++new_item_iterator;
}

void LocalWidgetLayout::on_item_move()
{
    QPoint gpoint = QCursor::pos();
    QPoint cpoint = parentWidget()->mapFromGlobal(gpoint);

    if(!qobject_cast<LocalMediaWidget*>(sender())->isWidgetMove()) return;

    if(!this->geometry().contains(cpoint)){
        qobject_cast<LocalMediaWidget*>(sender())->onWidgetMoveOutSide();
        if(select_item){
            qobject_cast<LocalMediaWidget*>(select_item)->onWidgetMoveLeave();
        }
        select_item = nullptr;
    }else{
emit    onClearWidgetContraposition();
        qobject_cast<LocalMediaWidget*>(sender())->onWidgetMoveInSide();

        if(select_item){
            if(!select_item->geometry().contains(cpoint)){
                qobject_cast<LocalMediaWidget*>(select_item)->onWidgetMoveLeave();
                if((select_item = onFindItemAt(cpoint))){
                    qobject_cast<LocalMediaWidget*>(select_item)->onWidgetMoveEnter();
                }
            }
        }else{
            if((select_item = onFindItemAt(cpoint))){
                qobject_cast<LocalMediaWidget*>(select_item)->onWidgetMoveEnter();
            }
        }
    }
}

void LocalWidgetLayout::on_item_contraposition()
{
    LocalMediaWidget *cwidget = qobject_cast<LocalMediaWidget*>(sender());
    cwidget->onWidgetQuitAnimation();
emit    onCorrelateWigetContraposition(qobject_cast<LocalMediaWidget*>(sender()));
}

void LocalWidgetLayout::on_item_contraposition(int value, int max)
{
    if(select_item) return;
    if(value >= max){
        resetMoveLayout();
    }
    qDebug() << "on_item_contraposition->" << value << "," << max;
emit    onUpdateWidgetContraposition(value);
}

void LocalWidgetLayout::on_item_mend()
{
    std::pair<int, int> cposition{0, 0};
    LocalMediaWidget *cwidget = qobject_cast<LocalMediaWidget*>((*old_item_iterator)->widget());
    auto citerator = old_item_iterator;

    if(select_item){
        qobject_cast<LocalMediaWidget*>(select_item)->onWidgetMoveLeave();
    }

    if(new_item_iterator != (++old_item_iterator)){
        cposition = (new_item_iterator == layout_items.end()) ? LocalMediaWidget::onPosition(layout_items.size(), layout_item_column)
                                                              : LocalMediaWidget::onPosition(qobject_cast<LocalMediaWidget*>((*new_item_iterator)->widget()), layout_item_column);

        layout_items.erase(citerator);
        cwidget->setWidgetIterator(layout_items.insert(new_item_iterator, (*citerator)));
        cwidget->setWidgetPosition(cposition);
//        setGeometry(layout_rect);
        update_layout = true;
    }

    old_item_iterator = layout_items.end();
    new_item_iterator = layout_items.end();
    pre_item_iterator = layout_items.end();
}

void LocalWidgetLayout::on_item_select()
{
    if(!select_item){
        return;
    }
    if(select_item != qobject_cast<QWidget*>(sender())){
        return;
    }
    qobject_cast<LocalMediaWidget*>(sender())->onWidgetMoveLeave();

    pre_item_iterator = new_item_iterator;
    new_item_iterator = qobject_cast<LocalMediaWidget*>(select_item)->getWidgetIterator();

    if(pre_item_iterator == layout_items.end()){
        onMoveLayout(new_item_iterator, pre_item_iterator, ITEM_MOVE_DIR_LEFT);
    }else{
        if(qobject_cast<LocalMediaWidget*>((*new_item_iterator)->widget()) >=
           qobject_cast<LocalMediaWidget*>((*pre_item_iterator)->widget())){
            onMoveLayout(pre_item_iterator, ++new_item_iterator, ITEM_MOVE_DIR_RIGHT);
        }else{
            onMoveLayout(new_item_iterator, pre_item_iterator, ITEM_MOVE_DIR_LEFT);
        }
    }
}

void LocalWidgetLayout::on_item_close()
{
    LocalMediaWidget *cwidget = qobject_cast<LocalMediaWidget*>(sender());
    cwidget->onWidgetQuitAnimation();
}

void LocalWidgetLayout::on_itme_finish()
{
    QWidget *item = qobject_cast<QWidget*>(sender());
    auto iterator = qobject_cast<LocalMediaWidget*>(item)->getWidgetIterator();

    onMoveLayout(++iterator, layout_items.end(), ITEM_MOVE_DIR_RIGHT, true);
    onReleaseItem(item);
}

void LocalWidgetLayout::on_move_finish()
{
    if((--layout_item_number) <= 0){
        layout_item_number = layout_item_column;
        layout_item_row--;
        item_avg.setHeight(layout_rect.height() / layout_item_row);
        update_layout = true;
        setGeometry(layout_rect);
    }
}

void LocalWidgetLayout::timerEvent(QTimerEvent *event)
{
    QObject::timerEvent(event);
}

QWidget *LocalWidgetLayout::onFindItemAt(const QPoint &p)
{
    QWidget *child = nullptr;
    for(auto begin = layout_items.begin(), end = layout_items.end(); begin != end; ++begin){
        child = qobject_cast<QWidget*>((*begin)->widget());
        if (!child || child->isWindow() || child->isHidden() || child->testAttribute(Qt::WA_TransparentForMouseEvents)) {
            continue;
        }
        if (!child->geometry().contains(p)){
            continue;
        }
        return child;
    }
    return 0;
}

void LocalWidgetLayout::resetMoveLayout()
{
    auto citerator = old_item_iterator;
    if((++citerator) != new_item_iterator){
        if(new_item_iterator == layout_items.end()){
            onMoveLayout(citerator, new_item_iterator, ITEM_MOVE_DIR_LEFT);
        }else if(citerator == layout_items.end()){
            onMoveLayout(new_item_iterator, citerator, ITEM_MOVE_DIR_RIGHT);
        }else{
            if(qobject_cast<LocalMediaWidget*>((*citerator)->widget()) >
               qobject_cast<LocalMediaWidget*>((*new_item_iterator)->widget())){
                onMoveLayout(new_item_iterator, citerator, ITEM_MOVE_DIR_RIGHT);
            }else{
                onMoveLayout(citerator, new_item_iterator, ITEM_MOVE_DIR_LEFT);
            }
        }
    }
}

void LocalWidgetLayout::onMoveLayout(std::list<QLayoutItem*>::iterator start_iterator, std::list<QLayoutItem*>::iterator end_iterator, CustomLayoutItemMoveDir move_dir, bool is_connect)
{
    new (&move_animation_group) QParallelAnimationGroup();
    if(is_connect){
        connect(&move_animation_group, SIGNAL(finished()), this, SLOT(on_move_finish()));
    }else{
        disconnect(&move_animation_group, SIGNAL(finished()), this, SLOT(on_move_finish()));
    }

    CustomLayoutItemMoveDir upgrade_dir = static_cast<CustomLayoutItemMoveDir>(static_cast<int>(move_dir) + 1);

    for( ; start_iterator != end_iterator; ++start_iterator){
        if(start_iterator == old_item_iterator){
            continue;
        }
        if(qobject_cast<LocalMediaWidget*>((*start_iterator)->widget())->isUpgrade(move_dir, layout_item_column)){
            move_animation_group.addAnimation(onMoveItem(start_iterator, upgrade_dir));
        }else{
            move_animation_group.addAnimation(onMoveItem(start_iterator, move_dir));
        }
    }

    move_animation_group.start();
}

QAbstractAnimation* LocalWidgetLayout::onMoveItem(std::list<QLayoutItem*>::iterator &iterator, CustomLayoutItemMoveDir move_dir)
{
    LocalMediaWidget *item = qobject_cast<LocalMediaWidget*>((*iterator)->widget());
    QAbstractAnimation *animation = nullptr;

    switch (move_dir) {
    case ITEM_MOVE_DIR_LEFT:
        item->updateLeftPosition();
        animation = qobject_cast<LocalMediaWidget*>(item)->onWidgetMoveAnimation(ITEM_MOVE_DIR_LEFT, QPoint{item->x() + item_avg.width(), item->y()}, 0);
        break;
    case ITEM_MOVE_DIR_RIGHT:
        item->updateRightPosition();
        animation = qobject_cast<LocalMediaWidget*>(item)->onWidgetMoveAnimation(ITEM_MOVE_DIR_RIGHT, QPoint{item->x() - item_avg.width(), item->y()}, 0);
        break;
    case ITEM_MOVE_DIR_LEFT_UP:
        item->updateLeftUpPosition();
        animation = qobject_cast<LocalMediaWidget*>(item)->onWidgetMoveAnimation(ITEM_MOVE_DIR_LEFT_UP,
                                                                             QPoint{layout_rect.width() + item_avg.width(), item->y() + item_avg.height()} , layout_item_margin_left);
        break;
    case ITEM_MOVE_DIR_RIGHT_DOWN:
        item->updateRightDownPosition(layout_item_column);
        animation = qobject_cast<LocalMediaWidget*>(item)->onWidgetMoveAnimation(ITEM_MOVE_DIR_RIGHT_DOWN,
                                                                             QPoint{layout_rect.width() - item_avg.width(), item->y() - item_avg.height()} , layout_item_margin_left);
        break;
    default:
        break;
    }

    return animation;
}

void LocalWidgetLayout::onReleaseItem(QWidget *item)
{
    LocalMediaWidget *media_widget =  qobject_cast<LocalMediaWidget*>(item);
    if(media_widget->onWidgetOperatorType() == LOCAL_MEDIA_WIDGET_TYPE_QUIT){
        item->close();
        item->setParent(nullptr);
        delete media_widget;
    }else if(media_widget->onWidgetOperatorType() == LOCAL_MEDIA_WIDGET_TYPE_CONTRAPOSITION_DONE){
        item->setParent(nullptr);
emit    onCorrelateWigetContraposition(media_widget);
    }
}
