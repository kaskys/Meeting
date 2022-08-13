#ifndef IOCONTROL_H
#define IOCONTROL_H

#include <QIODevice>
#include <QAudioFormat>
#include <QThread>
#include <qendian.h>
#include <QDebug>

#include <functional>

class IOControl : public QIODevice
{
public:
    IOControl(QAudioFormat, const std::function<void(qreal)>&);
    ~IOControl() override;

    bool isSequential() const override { return false; }
    bool open(QIODevice::OpenMode mode) override;
    void close() override;

    qint64 pos() const override;
    qint64 size() const override;
    bool seek(qint64 pos) override;
    bool atEnd() const override;
    bool reset() override;

    bool canReadLine() const override { return false; }
    bool waitForReadyRead(int msecs) override { return QIODevice::waitForReadyRead(msecs); }
    bool waitForBytesWritten(int msecs) override { return QIODevice::waitForBytesWritten(msecs); }

    bool updateAudioFormat(QAudioFormat);
    void updateVolumeFunc(const std::function<void(qreal)> &func) { this->update_func = func; }

    virtual int byteIndex() = 0;
    virtual bool copyData(int, char*, int&) = 0;
    virtual void clear() = 0;
    virtual void connectReadCallback(const std::function<void(int, int, IOControl*)>&) = 0;
private:
    virtual void onCreateIOBuffer(qint64) = 0;
    virtual void onDestroyIOBuffer() = 0;

    uint32_t onInitControl();
protected:
    virtual bool onInitIOBuffer() = 0;
    virtual void unInitIOBuffer() = 0;
    virtual char* onRIOBuffer() = 0;
    virtual char* onWIOBuffer() = 0;
    virtual bool onIOReset() = 0;
    virtual qint64 onIOPos() const = 0;
    virtual bool onIOSeek(qint64) = 0;
    virtual bool isIORread(qint64&) = 0;
    virtual bool isIOWrite(qint64&) = 0;
    virtual qint64 onIODataRead(qint64) = 0;
    virtual qint64 onIODataWrite(qint64) = 0;
    virtual bool onUpdateFormat(qint64) = 0;
    virtual void onIOReadyRead() = 0;

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

    quint32 m_maxAmplitude;
    qint64 buffer_len;
    qint64 buffer_offset;
    QAudioFormat format;
    std::function<void(qreal)> update_func;
};

#endif // IOCONTROL_H
