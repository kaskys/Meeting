#include "localaudiowidget.h"

LocalAudioWidget::LocalAudioWidget(QWidget *parent, uint32_t size) : QWidget(parent), is_update_volume(false), audio_timer_id(0), audio_volume_width_margin(50), audio_volume_height_margin(10),
    audio_volume_start(0), audio_rect_size(0),audio_rect_space(2), audio_volume_array(nullptr)
#ifdef  LOCAL_AUDIO_VOLUMN_LINE
    , audio_volume_line_width(0), audio_volume_line_height(0), audio_volume_line_pen(), audio_volume_line_brush()
#endif
    , audio_volume_rect_width(0), audio_volume_rect_height(0), audio_volume_rect_pen(), audio_volume_rect_brush()
{
    initPen();
    initBrush();
    updateAudioRect(size);
}

LocalAudioWidget::~LocalAudioWidget()
{
    if(audio_volume_array) delete[] audio_volume_array;
}

void LocalAudioWidget::clearVolume()
{
    memset(audio_volume_array, 0, sizeof(qreal) * audio_rect_size);
    update();
}

void LocalAudioWidget::updateAudioVolume(qreal value)
{
    is_update_volume = true;
    audio_volume_array[audio_volume_start] = value;
    if((++audio_volume_start) >= audio_rect_size){
        audio_volume_start = 0;
    }
    update();
}

void LocalAudioWidget::setAudioVolumeWidthMargin(uint32_t value)
{
    if(value == audio_volume_width_margin){ return; }
    if(value >= (this->width() >> 2)) { return; }
    audio_volume_width_margin = value;
    initVolumeSize();
    update();
}

void LocalAudioWidget::setAudioVolumeHeightMargin(uint32_t value)
{
    if(value == audio_volume_height_margin){ return; }
    if(value >= (this->height() >> 2)) { return; }
    audio_volume_height_margin = value;
    initVolumeSize();
    update();
}

void LocalAudioWidget::setAudioVolumeSpace(uint32_t value)
{
    if(value == audio_rect_space) { return; }
    if(value >= audio_volume_rect_width){ return; }
    audio_rect_space = value;
    initVolumeSize();
    update();
}

void LocalAudioWidget::onUpdate(qreal value)
{
    updateAudioVolume(value);
}

void LocalAudioWidget::timerEvent(QTimerEvent *event)
{
    QObject::timerEvent(event);

    if(!is_update_volume){
        updateAudioVolume(0.0f);
    }
    is_update_volume = false;
}

void LocalAudioWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    initVolumeSize();
}

void LocalAudioWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter{this};
    this->width(); this->height();

    onPaintVolume(event->rect(), painter);
}

void LocalAudioWidget::initPen()
{
#ifdef LOCAL_AUDIO_VOLUMN_LINE
    audio_volume_line_width = this->width() - (audio_volume_width_margin << 1);
    audio_volume_line_height = 1;
    audio_volume_line_pen.setWidth(1);
    audio_volume_line_pen.setColor(Qt::blue);
    audio_volume_line_pen.setStyle(Qt::SolidLine);
    audio_volume_line_pen.setJoinStyle(Qt::RoundJoin);
#endif
    audio_volume_rect_pen.setWidth(1);
    audio_volume_rect_pen.setColor(Qt::black);
    audio_volume_rect_pen.setStyle(Qt::SolidLine);
    audio_volume_rect_pen.setJoinStyle(Qt::RoundJoin);
}

void LocalAudioWidget::initBrush()
{
#ifdef LOCAL_AUDIO_VOLUMN_LINE
    audio_volume_line_brush.setStyle(Qt::SolidPattern);
    audio_volume_line_brush.setColor(Qt::blue);
#endif
    audio_volume_rect_brush.setStyle(Qt::SolidPattern);
    audio_volume_rect_brush.setColor(QColor{0,255,127});
}

void LocalAudioWidget::initVolumeSize()
{
#ifdef LOCAL_AUDIO_VOLUMN_LINE
    audio_volume_line_width = this->width() - (audio_volume_width_margin << 1);
    audio_volume_line_height = 1;
#endif
    //this->height / 4
    audio_volume_rect_height = (this->height() - (audio_volume_height_margin << 1)) >> 2;
    audio_volume_rect_width = (this->width() - (audio_volume_width_margin << 1) - ((audio_rect_size - 1) * audio_rect_space)) / audio_rect_size;
}

void LocalAudioWidget::updateAudioRect(uint32_t size)
{
    qreal *array = new (std::nothrow) qreal[size];

    if(!array){
        return;
    }

    memset(array, 0, sizeof(qreal) * size);
    if(audio_volume_array){
        if(size > audio_rect_size){
            if(audio_volume_start == (audio_rect_size - 1)){
                memcpy(array + (size - audio_rect_size), audio_volume_array, sizeof(qreal) * audio_rect_size);
            }else{
                memcpy(array + (size - audio_rect_size), audio_volume_array + (audio_volume_start + 1), sizeof(qreal) * (audio_rect_size - audio_rect_size - 1));
                memcpy(array + (size - audio_rect_size) + (audio_rect_size - audio_rect_size - 1), audio_volume_array, sizeof(qreal) * (audio_volume_start + 1));
            }
        }else{
            if(audio_volume_start == (audio_rect_size - 1)){
                memcpy(array, audio_volume_array + (audio_rect_size - size), sizeof(qreal) * size);
            }else{
                memcpy(array, audio_volume_array + (audio_volume_start + 1) + (audio_rect_size - size), sizeof(qreal) * (size - audio_volume_start - 1));
                memcpy(array + (size - audio_volume_start - 1), audio_volume_array, sizeof(qreal) * (audio_volume_start + 1));
            }
        }
        initVolumeSize();
        delete[] audio_volume_array;
    }

    audio_volume_array = array;
    audio_rect_size = size;
    update();
}

void LocalAudioWidget::onPaintVolume(const QRect &rect, QPainter &painter)
{
#ifdef LOCAL_AUDIO_VOLUMN_LINE
    onPaintVolumeLine(rect, painter);
#endif
    painter.setPen(audio_volume_rect_pen);
    painter.setBrush(audio_volume_rect_brush);
    for(int i = audio_volume_start + 1, pos = 0; pos < audio_rect_size; i++, pos++){
        if(i == audio_rect_size){   i = 0; }
        onPaintVolumeRect(rect, painter, pos, audio_volume_array[i]);
    }
}

void LocalAudioWidget::onPaintVolumeLine(const QRect &rect, QPainter &painter)
{
#ifdef LOCAL_AUDIO_VOLUMN_LINE
    painter.setPen(audio_volume_line_pen);
    painter.setBrush(audio_volume_line_brush);

    int line_x = audio_volume_width_margin,
        line_y = (rect.height() >> 1) - audio_volume_height_margin - (audio_volume_line_height >> 1);

    painter.drawRect(QRect{line_x, line_y, rect.width() - (audio_volume_width_margin << 1), audio_volume_line_height});
#endif
}

void LocalAudioWidget::onPaintVolumeRect(const QRect &rect, QPainter &paineter, uint32_t pos, qreal value)
{
    int rect_height = audio_volume_rect_height + static_cast<int>((audio_volume_rect_height << 1) * value),
        rect_x = audio_volume_width_margin + (pos * (audio_volume_rect_width + audio_rect_space)),
        rect_y = (this->height() - (audio_volume_height_margin << 1)) - (rect_height >> 1);
    paineter.drawRect(QRect{rect_x, rect_y, audio_volume_rect_width, rect_height});
}

