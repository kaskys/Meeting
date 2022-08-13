#include "iocontrol.h"

IOControl::IOControl(QAudioFormat fmat, const std::function<void (qreal)> &ufunc) : QIODevice(nullptr), m_maxAmplitude(0),buffer_len(0),
                                                                                    buffer_offset(0), format(fmat), update_func(ufunc)
{
    buffer_len = onInitControl();
}

IOControl::~IOControl()
{
    close();
}

uint32_t IOControl::onInitControl()
{
    switch (format.sampleSize()) {
    case 8:
        if(format.sampleType() == QAudioFormat::SignedInt){
            m_maxAmplitude = 255;
        }
        if(format.sampleType() == QAudioFormat::UnSignedInt){
            m_maxAmplitude = 127;
        }
        break;
    case 16:
        if(format.sampleType() == QAudioFormat::SignedInt){
            m_maxAmplitude = 32767;
        }
        if(format.sampleType() == QAudioFormat::UnSignedInt){
            m_maxAmplitude = 65535;
        }
        break;
    case 32:
        if(format.sampleType() == QAudioFormat::SignedInt){
            m_maxAmplitude = 0x7FFFFFFF;
        }
        if(format.sampleType() == QAudioFormat::UnSignedInt){
            m_maxAmplitude = 0xFFFFFFFF;
        }
        if(format.sampleType() == QAudioFormat::Float){
            m_maxAmplitude = 0x7FFFFFFF;
        }
        break;
    default:
        break;
    }
    return format.bytesForDuration(1000 * 1000);
}

bool IOControl::open(QIODevice::OpenMode mode)
{
    if(!(mode & QIODevice::Unbuffered)){
        mode |= QIODevice::Unbuffered;
    }
    if(!QIODevice::open(mode)){
        return false;
    }

    if(!onInitIOBuffer()){
        QIODevice::close();
        return false;
    }
    return true;
}

void IOControl::close()
{
    if(isOpen()){
        QIODevice::close();
        unInitIOBuffer();
    }
}

qint64 IOControl::pos() const
{
    return isOpen() ? onIOPos() : 0;
}

qint64 IOControl::size() const
{
    return isOpen() ? buffer_len : 0;
}

bool IOControl::seek(qint64 pos)
{
    if(!isOpen()|| (pos < 0) || (pos >= onIOPos())){
        return false;
    }else{
        return onIOSeek(pos);
    }
}

bool IOControl::atEnd() const
{
    return (buffer_offset >= buffer_len);
}

bool IOControl::reset()
{
    if(isOpen()){
        buffer_offset = 0;
        return onIOReset();
    }else{
        return false;
    }
}

bool IOControl::updateAudioFormat(QAudioFormat format)
{
    qint64 olen = buffer_len;
    this->format = format;
    buffer_len = onInitControl();
    return onUpdateFormat(olen);
}

qint64 IOControl::readData(char *data, qint64 maxlen)
{
    if(!isOpen()){ return 0; }
    if(!data || (maxlen <= 0)) { return 0; }
    if(!isIORread(maxlen)) { return 0; }

    memcpy(data, onRIOBuffer() , maxlen);

    return onIODataRead(maxlen);
}

qint64 IOControl::writeData(const char *data, qint64 len)
{
    if(!isOpen()) { return 0; }
    if(!data || (len <= 0)) { return 0; }
    if(!m_maxAmplitude) { return 0; }

    const int channelBytes = format.sampleSize() / 8;
    const int sampleByte = format.channelCount() * channelBytes;
    const int numSamples = len / sampleByte;

    if(!isIOWrite(len)){
        return 0;
    }

    if(update_func){
        quint32 offset = 0;
        quint32 maxValue = 0;
        qreal m_level;
        const unsigned char *ptr = reinterpret_cast<const unsigned char*>(data);

        for (int i = 0; i < numSamples; ++i) {
            for (int j = 0; j < format.channelCount(); ++j) {
                quint32 value = 0;

                if (format.sampleSize() == 8 && format.sampleType() == QAudioFormat::UnSignedInt) {
                    value = *reinterpret_cast<const quint8*>(ptr);
                } else if (format.sampleSize() == 8 && format.sampleType() == QAudioFormat::SignedInt) {
                    value = qAbs(*reinterpret_cast<const qint8*>(ptr));
                } else if (format.sampleSize() == 16 && format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint16>(ptr);
                    else
                        value = qFromBigEndian<quint16>(ptr);
                } else if (format.sampleSize() == 16 && format.sampleType() == QAudioFormat::SignedInt) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian){
                        value = qAbs(qFromLittleEndian<qint16>(ptr));
                    } else{
                        value = qAbs(qFromBigEndian<qint16>(ptr));
                    }
                } else if (format.sampleSize() == 32 && format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint32>(ptr);
                    else
                        value = qFromBigEndian<quint32>(ptr);
                } else if (format.sampleSize() == 32 && format.sampleType() == QAudioFormat::SignedInt) {
                    if (format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint32>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint32>(ptr));
                } else if (format.sampleSize() == 32 && format.sampleType() == QAudioFormat::Float) {
                    value = qAbs(*reinterpret_cast<const float*>(ptr) * 0x7fffffff); // assumes 0-1.0
                }

                maxValue = qMax(value, maxValue);
                ptr += channelBytes;
                offset += channelBytes;
            }
        }

        maxValue = qMin(maxValue, m_maxAmplitude);
        m_level = qreal(maxValue) / m_maxAmplitude;
        update_func(m_level);
    }
    memcpy(onWIOBuffer(), data, len);
    return onIODataWrite(len);
}
