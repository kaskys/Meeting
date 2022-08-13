#include "audiodevice.h"

AudioInputDevice::AudioInputDevice(QAudioFormat fmat, const std::function<void (qreal)> &func, int len)
    : IOControl(fmat, func), read_pos(0), array_pos(0), array_len(len), ib_len(0), io_buffer(nullptr),
      concurrency_array_buffer(nullptr), read_func(nullptr)
{
    onCreateIOBuffer(0);
}

AudioInputDevice::~AudioInputDevice()
{
    onDestroyIOBuffer();
}

bool AudioInputDevice::copyData(int pos, char *data, int &len)
{
    read_pos = pos;
    if(onConcurrencyType(read_pos, AUDIO_DEVICE_CONCURRENCY_COPY_HOLD, AUDIO_DEVICE_CONCURRENCY_COPY_OUTPUT)){
        len = readData(data, len);
        return true;
    }else{
        return false;
    }
}

void AudioInputDevice::clear()
{
    reset();
    array_pos = 0;
    AudioDeviceConcurrency type = AUDIO_DEVICE_CONCURRENCY_INIT;
    for(int i = 0; i < array_len; i++){
        for(;;){
            type = concurrency_array_buffer[i].load(std::memory_order_acquire);

            if(type == AUDIO_DEVICE_CONCURRENCY_COPY_OUTPUT){
                continue;
            }
            if(type == AUDIO_DEVICE_CONCURRENCY_INIT){
                break;
            }
            if(concurrency_array_buffer[i].compare_exchange_weak(type, AUDIO_DEVICE_CONCURRENCY_INIT, std::memory_order_release, std::memory_order_relaxed)){
                break;
            }
        }
    }
}

int AudioInputDevice::byteIndex()
{
    if(onConcurrencyType(array_pos, AUDIO_DEVICE_CONCURRENCY_COPY_DONE, AUDIO_DEVICE_CONCURRENCY_COPY_HOLD)){
        return array_pos;
    }else{
        return -1;
    }
}

void AudioInputDevice::onCreateIOBuffer(qint64)
{
    if(io_buffer || !(io_buffer = new (std::nothrow) char*[array_len]) || !(concurrency_array_buffer = new (std::nothrow) std::atomic<AudioDeviceConcurrency>[array_len])){
        if(io_buffer) delete io_buffer;
    }
}

void AudioInputDevice::onDestroyIOBuffer()
{
    if(!io_buffer) return;

    for(uint32_t i = 0; i < array_len; i++){
        if(io_buffer[i]) delete[] io_buffer[i];
    }
}

bool AudioInputDevice::onInitIOBuffer()
{
    return onInitBuffer(ib_len);
}

void AudioInputDevice::unInitIOBuffer()
{
    ib_len = buffer_len;
}


char* AudioInputDevice::onRIOBuffer()
{
    return io_buffer[read_pos];
}

char* AudioInputDevice::onWIOBuffer(){
    return io_buffer[array_pos] + buffer_offset;
}

bool AudioInputDevice::onIOReset()
{
    return true;
}

bool AudioInputDevice::isIORread(qint64 &read_len)
{
    AudioDeviceConcurrency type = concurrency_array_buffer[read_pos].load(std::memory_order_acquire);

    if((read_len > 0) && (type == AUDIO_DEVICE_CONCURRENCY_COPY_OUTPUT)){
        read_len = qMin(read_len, buffer_offset); return true;
    }else{
        return false;
    }
}

bool AudioInputDevice::isIOWrite(qint64 &write_len)
{
    AudioDeviceConcurrency type = AUDIO_DEVICE_CONCURRENCY_INIT;
    for(;;){
        type = concurrency_array_buffer[array_pos].load(std::memory_order_acquire);

        if((buffer_offset > 0) && (type == AUDIO_DEVICE_CONCURRENCY_COPY_INPUT)){
            break;
        }
        if(buffer_offset <= 0){
            if((type == AUDIO_DEVICE_CONCURRENCY_INIT) || (type == AUDIO_DEVICE_CONCURRENCY_COPY_DONE) || (type == AUDIO_DEVICE_CONCURRENCY_COPY_HOLD)){
                if(concurrency_array_buffer[array_pos].compare_exchange_weak(type, AUDIO_DEVICE_CONCURRENCY_COPY_INPUT, std::memory_order_release, std::memory_order_relaxed)){
                    break;
                }
            }else{
                if((++array_pos) >= array_len){
                    array_pos = 0;
                }
                return false;
            }
        }
    }
    uint64_t wlen = write_len + buffer_offset;
    if(wlen > buffer_len){ onLoadBuffer(); }
    return (wlen <= buffer_len);
}

qint64 AudioInputDevice::onIODataRead(qint64 read_len)
{
    unLoadBuffer();
    return read_len;
}

qint64 AudioInputDevice::onIODataWrite(qint64 write_len)
{
//    QThread::usleep(1000000/format.sampleRate());
    if((buffer_offset += write_len) >= buffer_len){
        onLoadBuffer();
    }
    return write_len;
}

bool AudioInputDevice::onUpdateFormat(qint64 olen)
{
    return onInitBuffer((ib_len = olen));
}

void AudioInputDevice::onIOReadyRead()
{
    if(read_func) read_func(buffer_offset, array_pos, this);
}

bool AudioInputDevice::onInitBuffer(qint64 olen)
{
    if(!io_buffer) return false;

    bool create_buffer = (buffer_len != olen);
    int pos = 0;
    char *buffer[array_len];

    if(create_buffer){
        for(; pos < array_len; pos++){
            buffer[pos] = new (std::nothrow) char[buffer_len];
            if(!buffer[pos]){
                break;
            }
        }
    }else{
        pos = array_len;
    }

    if(olen <= 0){
        for(int i = 0; i < array_len; i++){
            concurrency_array_buffer[i].store(AUDIO_DEVICE_CONCURRENCY_INIT);
        }
    }else{
        AudioDeviceConcurrency type = AUDIO_DEVICE_CONCURRENCY_INIT;
        for(int i = 0; i < array_len; i++){
            for(;;){
                type = concurrency_array_buffer[i].load(std::memory_order_acquire);

                if(type == AUDIO_DEVICE_CONCURRENCY_COPY_OUTPUT){
                    continue;
                }
                if(type == AUDIO_DEVICE_CONCURRENCY_INIT){
                    break;
                }
                if(concurrency_array_buffer[i].compare_exchange_weak(type, AUDIO_DEVICE_CONCURRENCY_INIT, std::memory_order_release, std::memory_order_relaxed)){
                    break;
                }
            }
        }
    }


    if(pos < array_len){
        for(int i = 0; i < pos; i++){
            delete[] buffer[i];
        }
        return false;
    }else{
        for(int i = 0; i < array_len; i++){
            if(!create_buffer){ delete[] io_buffer[i]; }
            io_buffer[i] = buffer[i];
        }
        return true;
    }
}

void AudioInputDevice::onLoadBuffer()
{
    onConcurrencyType(array_pos, AUDIO_DEVICE_CONCURRENCY_COPY_INPUT, AUDIO_DEVICE_CONCURRENCY_COPY_DONE);
    onIOReadyRead();
    if((++array_pos) >= array_len){
        array_pos = 0;
    }
    reset();
}

void AudioInputDevice::unLoadBuffer()
{
    onConcurrencyType(read_pos, AUDIO_DEVICE_CONCURRENCY_COPY_OUTPUT, AUDIO_DEVICE_CONCURRENCY_INIT);
}

bool AudioInputDevice::onConcurrencyType(int pos, AudioDeviceConcurrency otype, AudioDeviceConcurrency ntype)
{
    AudioDeviceConcurrency type = AUDIO_DEVICE_CONCURRENCY_INIT;

    for(;;){
        type = concurrency_array_buffer[pos].load(std::memory_order_acquire);
        if(type != otype){
            return false;
        }

        if(concurrency_array_buffer[pos].compare_exchange_weak(type, ntype, std::memory_order_release, std::memory_order_relaxed)){
            return true;
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------//



AudioOutputDevice::AudioOutputDevice(QAudioFormat format, const std::function<void (qreal)> &func) : IOControl(format, func), output_pos(0), io_buffer(nullptr)
{
    onCreateIOBuffer(0);
}

AudioOutputDevice::~AudioOutputDevice()
{
    onDestroyIOBuffer();;
}

void AudioOutputDevice::onCreateIOBuffer(qint64 olen)
{
    if(io_buffer && (buffer_len == olen)) return;

    char *buffer = nullptr;
    if(!(buffer = new (std::nothrow) char[buffer_len])){
        return;
    }
    if(io_buffer){
        delete[] io_buffer;
    }
    io_buffer = buffer;
}

void AudioOutputDevice::onDestroyIOBuffer()
{
    if(io_buffer) delete io_buffer;
}

bool AudioOutputDevice::onInitIOBuffer()
{
    if(!io_buffer) return false;
    return reset();
}

char* AudioOutputDevice::onRIOBuffer()
{
    return io_buffer + output_pos;
}

char* AudioOutputDevice::onWIOBuffer()
{
    return io_buffer + buffer_offset;
}

bool AudioOutputDevice::onIOReset()
{
    return true;
}

bool AudioOutputDevice::isIORread(qint64 &read_len)
{
    //因为output_pos.load和output_pos.store存在先行关系，所以获取buffer_offset的值会先与获取output_pos的值（最新）
    uint32_t opos = output_pos.load(std::memory_order_acquire);
    if(!(read_len <= 0) && ((read_len = qMin(read_len, static_cast<qint64>(buffer_offset - opos))) > 0)){
        return true;
    }else{
        return false;
    }
}

bool AudioOutputDevice::isIOWrite(qint64 &write_len)
{
    reset();
    return ((buffer_offset + write_len) <= buffer_len);
}

qint64 AudioOutputDevice::onIODataRead(qint64 read_len)
{
    output_pos += read_len;
    return read_len;
}

qint64 AudioOutputDevice::onIODataWrite(qint64 write_len)
{
    buffer_offset += write_len;
    onIOReadyRead();
    return write_len;
}

bool AudioOutputDevice::onUpdateFormat(qint64 olen)
{
    onCreateIOBuffer(olen);
    return onInitIOBuffer();
}

void AudioOutputDevice::onIOReadyRead()
{
    output_pos.store(0, std::memory_order_release);
}
