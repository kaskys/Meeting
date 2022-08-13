#include "filterdialog.h"
#include "ui_filterdialog.h"

FilterDialog::FilterDialog(QWidget *parent) :
    QWidget(parent), ui(new Ui::FilterDialog),
    ip_ui()
{
    ui->setupUi(this);
    setWindowTitle("过滤信息");
    setWindowFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);

    connect(&ip_ui, SIGNAL(onIpValueConfirm()), this, SLOT(on_push_ip_confirm()));
}

FilterDialog::~FilterDialog()
{
    delete ui;
}

void FilterDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    const auto &size = event->size();
    ui->splitter->setGeometry(0, 0, size.width(), size.height());
}

void FilterDialog::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    if(ip_ui.isVisible()){
        ip_ui.close();
    }
}

void FilterDialog::onFilterShow(FilterType)//, FilterAdapter *adater)
{
//    filter_adapter = adater;

    onInitFilter();
    onShowFilter();
}

void FilterDialog::on_push_btn_clicked()
{
    if(!ip_ui.isVisible()){
        ip_ui.show();
    }
}

void FilterDialog::on_remove_btn_clicked()
{

}

void FilterDialog::on_look_btn_clicked()
{
    on_filter_listview_doubleClicked(ui->filter_listview->currentIndex());
}

void FilterDialog::on_filter_listview_clicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    ui->remove_btn->setEnabled(true);
    ui->look_btn->setEnabled(true);
}

void FilterDialog::on_filter_listview_doubleClicked(const QModelIndex &index)
{

}

void FilterDialog::on_push_ip_confirm()
{

}

void FilterDialog::onInitFilter()
{

}

void FilterDialog::onShowFilter()
{
    ui->remove_btn->setEnabled(false);
    ui->look_btn->setEnabled(false);

    QDesktopWidget *desktop = QApplication::desktop();
    const auto &rect = this->geometry();
    move(QPoint{(desktop->width() - rect.width()) / 2, (desktop->height() - rect.height())/ 2});
    this->show();
}
