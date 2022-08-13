#ifndef IPERROR_H
#define IPERROR_H

#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QLabel>
#include <QDebug>
#include <QCursor>
#include <QCloseEvent>

class IpError : public QWidget
{
    Q_OBJECT
public:
    explicit IpError(QWidget *parent = nullptr);
    ~IpError() Q_DECL_OVERRIDE;

    void initErrorValue();
    void setErrorPixmap(const QString&);
signals:

public slots:

protected:
    void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;
    void enterEvent(QEvent*) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent*) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;
private:
    QPixmap map_value;
    QLabel error_value;
};

#endif // IPERROR_H
