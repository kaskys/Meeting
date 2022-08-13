#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QLabel>
#include <QDesktopWidget>
#include <QResizeEvent>
#include <QStringList>
#include <QDebug>
#include <QThread>
#include <QPropertyAnimation>

#include "Ui/ipeditor.h"
#include "../Class/Init/initinfo.h"

namespace Ui {
class FilterDialog;
}

enum FilterType{
    FilterWhite,
    FilterBlack
};

class FilterDialog : public QWidget
{
    Q_OBJECT

public:
    explicit FilterDialog(QWidget *parent = 0);
    ~FilterDialog();

    void onGlobalInit(QLabel *label){
        label->setText("初始化过滤信息!");
        filter_adapter = InitInfo::onInitFilter();
        label->setText("过滤信息初始化完成!");
    }

    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

    void onFilterShow(FilterType);//, FilterAdapter*);
private slots:
    void on_push_btn_clicked();

    void on_remove_btn_clicked();

    void on_look_btn_clicked();

    void on_filter_listview_clicked(const QModelIndex &index);

    void on_filter_listview_doubleClicked(const QModelIndex &index);

    void on_push_ip_confirm();
private:
    void onInitFilter();
    void onShowFilter();

    Ui::FilterDialog *ui;

    FilterAdapter *filter_adapter;
    IpEditor ip_ui;
};

#endif // FILTERDIALOG_H
