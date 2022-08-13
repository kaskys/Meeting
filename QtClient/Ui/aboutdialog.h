#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QWidget>
#include <QDesktopWidget>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QWidget
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = 0);
    ~AboutDialog();

private:
    void initLayout();

    Ui::AboutDialog *ui;
};

#endif // ABOUTDIALOG_H
