#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window |Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);
    setWindowTitle("关于Meeting");
    setWindowOpacity(1);
    initLayout();
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::initLayout()
{
    const QDesktopWidget *desktop = QApplication::desktop();

    setFixedHeight(150);
    setFixedWidth(300);
    move(QPoint((desktop->width() - width()) / 2, (desktop->height() - height()) / 2));

    ui->label_github->setText(QString{"GitHub::https://github.com/kaskys/Meeting"});
    ui->label_author->setText(QString{"作者::kaskys"});
    ui->label_time->setText(QString{"时间::2022-05-15"});

    ui->label_github->adjustSize();
    ui->label_author->adjustSize();
    ui->label_time->adjustSize();

    const int base_height = height() / 6;
    const int increment_height = height() / 3;
    ui->label_github->move((width() - ui->label_github->width()) / 2, base_height);
    ui->label_author->move((width() - ui->label_author->width()) / 2, base_height + increment_height);
    ui->label_time->move((width() - ui->label_time->width()) / 2, base_height + (increment_height * 2));

}
