#include "ipeditor.h"
#include "ui_ipeditor.h"

const QString IpEditor::spot{"."};

IpEditor::IpEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IpEditor), ip_ivalue(0), ip_svalue()
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    ui->confirm_btn->setEnabled(false);
    ui->textEdit->document()->setMaximumBlockCount(2);
    ip_svalue.reserve(IP_VALUE_STRING_LEN);
}

IpEditor::~IpEditor()
{
    delete ui;
}

void IpEditor::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    QDesktopWidget *desktop = QApplication::desktop();
    const auto &rect = this->geometry();
    move(QPoint{(desktop->width() - rect.width()) / 2, (desktop->height() - rect.height())/ 2});

    if(!ip_svalue.isEmpty()){
        ui->textEdit->setPlainText(ip_svalue);
        ui->confirm_btn->setEnabled(true);
    }
}

void IpEditor::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
}

void IpEditor::on_textEdit_textChanged()
{
    ui->confirm_btn->setEnabled(false);

    QString text = ui->textEdit->toPlainText();

    if(text.contains(spot)){
        if(text.count(spot) == (IP_VALUE_BIT_INET_SIZE - 1) && onValueValid(text)){
            ip_svalue = std::move(text);
            ui->confirm_btn->setEnabled(true);
        }
    }else{
        quint32 bit_value = 10, intercept_pos = 0, value = 0;

        ip_svalue.clear();
        if(text.startsWith("0b") || text.startsWith("0B")){
            bit_value = 2;
            intercept_pos = 2;
        }else if(text.startsWith("0x") || text.startsWith("0X")){
            bit_value = 16;
            intercept_pos = 2;
        }else if(text.startsWith("0")){
            bit_value = 8;
            intercept_pos = 1;
        }

        if(!intercept_pos){
            text = text.mid(intercept_pos);
        }

        bool is_ok = false;
        value = text.toUInt(&is_ok, bit_value);

        if(is_ok){
            ip_ivalue = value;
            ui->confirm_btn->setEnabled(true);
        }
    }

}

void IpEditor::on_confirm_btn_clicked()
{
    if(ip_svalue.isEmpty()){
        ip_svalue = onIpValueToString(ip_ivalue);
    }

    this->close();
emit onIpValueConfirm();
}

void IpEditor::onValueInput(quint32 value)
{
//    quint8 bit_value = 0;
//    for(int i = IP_VALUE_BIT_INET_SIZE - 1; i >= 0; i--){
//        bit_value = ((value >> (i * 8)) & 0xFF);
//        ip_svalue.push_back(QString::number(bit_value));
//        if(i != 0){
//            ip_svalue.push_back(spot);
//        }
//    }
}

void IpEditor::onValueInput(const QString &value)
{
//    bool is_ok = false;
//    quint32 char_value = 0, bit_value = (sizeof(quint32) - 1) * 8;
//    QStringList string_list = value.split(".");
//    ip_value = 0;

//    if(string_list.count() != IP_VALUE_BIT_INET_SIZE){
//        return;
//    }
//    foreach (const auto &data, string_list) {
//        char_value = data.toUInt(&is_ok);
//        if(!is_ok || (char_value > IP_VALUE_BIT_MAX_VALUE)){
//            return;
//        }

//        ip_value |= (char_value << bit_value);
//        bit_value -= 8;
//    }

}

bool IpEditor::onValueValid(const QString &value)
{
    int first = 0, end = 0, size = IP_VALUE_BIT_INET_SIZE;
    for(;;){
        --size;
        end = value.indexOf(spot, first);

        if(size <= 0){
           if((end >= first) || (value.size() == first) || ((value.size() - first) >= IP_VALUE_BIT_INET_SIZE)){
               return false;
           }else{
               return true;
           }
        }else{
            if((end <= first) || (end - first) > IP_VALUE_BIT_INET_SIZE){
                return false;
            }
        }

        first = end + 1;

    }
    return true;
}
