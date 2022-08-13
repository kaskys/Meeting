#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <QAudioOutput>
#include <QTime>

#include <atomic>

#include "../Control/iocontrol.h"

#define AUDIO_INPUT_DEVICE_IO_BUFFER_ARRAY_LEN      24

enum AudioDeviceConcurrency{
    AUDIO_DEVICE_CONCURRENCY_INIT = 0,
    AUDIO_DEVICE_CONCURRENCY_COPY_INPUT,
    AUDIO_DEVICE_CONCURRENCY_COPY_DONE,
    AUDIO_DEVICE_CONCURRENCY_COPY_HOLD,
    AUDIO_DEVICE_CONCURRENCY_COPY_OUTPUT
};

class AudioInputDevice : public IOControl
{
public:
    AudioInputDevice(QAudioFormat, const std::function<void(qreal)>&, int len = AUDIO_INPUT_DEVICE_IO_BUFFER_ARRAY_LEN);
    ~AudioInputDevice() override;

    int byteIndex() override;
    bool copyData(int, char*, int&) override;
    void clear() override;
    void connectReadCallback(const std::function<void(int, int, IOControl*)> &func) override { read_func = func; }

    qint64 bytesAvailable() const override { return (isOpen() ? buffer_offset : 0); }
    qint64 bytesToWrite() const override { return (isOpen() ? (buffer_len - buffer_offset) : 0); }
protected:
    bool onInitIOBuffer() override;
    void unInitIOBuffer() override;
    char* onRIOBuffer() override;
    char* onWIOBuffer() override;
    qint64 onIOPos() const override  { return buffer_offset; }
    bool onIOSeek(qint64 pos) override { buffer_offset = pos; return true; }
    bool onIOReset() override;
    bool isIORread(qint64&) override;
    bool isIOWrite(qint64&) override;
    qint64 onIODataRead(qint64) override;
    qint64 onIODataWrite(qint64) override;
    bool onUpdateFormat(qint64) override;
    void onIOReadyRead() override;
private:
    void onCreateIOBuffer(qint64) override;
    void onDestroyIOBuffer() override;
    bool onInitBuffer(qint64);
    void onLoadBuffer();
    void unLoadBuffer();
    bool onConcurrencyType(int, AudioDeviceConcurrency, AudioDeviceConcurrency);

    uint32_t read_pos;      //code thread
    uint32_t array_pos;     //qt thread
    uint32_t array_len;
    uint32_t ib_len;
    QIODevice *io_device;
    char **io_buffer;
    std::atomic<AudioDeviceConcurrency> *concurrency_array_buffer;
    std::function<void(int, int, IOControl*)> read_func;
};

class AudioOutputDevice : public IOControl
{
public:
    AudioOutputDevice(QAudioFormat, const std::function<void(qreal)>&);
    ~AudioOutputDevice() override;

    int byteIndex() override { return 0; }
    bool copyData(int, char*, int&) override { return false; }
    void clear() override { reset(); }
    void connectReadCallback(const std::function<void(int, int, IOControl*)>&) override { /*不需要实现*/ }

    qint64 bytesAvailable() const override { return (isOpen() ? output_pos.load(std::memory_order_acquire) : 0); }
    qint64 bytesToWrite() const override { return (isOpen() ? (buffer_len -  buffer_offset): 0); }
protected:
    bool onInitIOBuffer() override;
    void unInitIOBuffer() override { /*不需要实现*/ }
    char* onRIOBuffer() override;
    char* onWIOBuffer() override;
    qint64 onIOPos() const override { return output_pos.load(std::memory_order_acquire);  }
    bool onIOSeek(qint64 pos) override { output_pos.store(pos, std::memory_order_release); return true;  }
    bool onIOReset() override;
    bool isIORread(qint64&) override;
    bool isIOWrite(qint64&) override;
    qint64 onIODataRead(qint64) override;
    qint64 onIODataWrite(qint64) override;
    bool onUpdateFormat(qint64) override;
    void onIOReadyRead() override;
private:
    void onCreateIOBuffer(qint64) override;
    void onDestroyIOBuffer() override;

    std::atomic<uint32_t> output_pos;  //(code thread)or(qt thread)
    char *io_buffer;
};


#endif // AUDIODEVICE_H
