#ifndef IPEDITOR_H
#define IPEDITOR_H

#include <QWidget>
#include <QString>
#include <QShowEvent>
#include <QHideEvent>;
#include <QDesktopWidget>
#include <QDebug>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IP_VALUE_BIT_INET_SIZE      4
#define IP_VALUE_BIT_MAX_VALUE      255
#define IP_VALUE_STRING_LEN         16

namespace Ui {
class IpEditor;
}

class IpEditor : public QWidget
{
    Q_OBJECT

public:
    explicit IpEditor(QWidget *parent = 0);
    ~IpEditor();

    QString getIpValue() const { return ip_svalue; }
    void setIpValue(QString &&value) { ip_svalue = std::move(value); }
    void setIpValue(const QString &value) { ip_svalue = value; }

    static uint32_t onIpStringToValue(const QString &value) { return inet_addr(value.toStdString().c_str()); }
    static QString  onIpValueToString(quint32 value) { return QString{inet_ntoa(in_addr{.s_addr = value })}; }
Q_SIGNALS:
    void onIpValueConfirm();
protected:
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    void hideEvent(QHideEvent *event) Q_DECL_OVERRIDE;
private slots:
    void on_textEdit_textChanged();

    void on_confirm_btn_clicked();
private:
    void onValueInput(quint32);
    void onValueInput(const QString&);
    bool onValueValid(const QString&);

    static const QString spot;

    Ui::IpEditor *ui;
    quint32 ip_ivalue;
    QString ip_svalue;
};

#endif // IPEDITOR_H
