#ifndef LOCALAUDIOWIDGET_H
#define LOCALAUDIOWIDGET_H

#include <QWidget>
#include <QTimerEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPainter>

#include <functional>

#define LOCAL_AUDIO_WIDGET_DEFAULT_COLUMN_SIZE      20
#define LOCAL_AUDIO_WIDGET_DEFAULT_MARGIN           10
//#define LOCAL_AUDIO_VOLUMN_LINE

class LocalAudioWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocalAudioWidget(QWidget *parent = nullptr, uint32_t size = LOCAL_AUDIO_WIDGET_DEFAULT_COLUMN_SIZE);
    ~LocalAudioWidget() Q_DECL_OVERRIDE;

    void clearVolume();
    void updateAudioVolume(qreal);
    void setAudioVolumeWidthMargin(uint32_t);
    void setAudioVolumeHeightMargin(uint32_t);
    void setAudioVolumeSpace(uint32_t);
    void setAudioVolumeColume(uint32_t value) {
        if(value != audio_rect_size) updateAudioRect(value);
    }
#ifdef  LOCAL_AUDIO_VOLUMN_LINE
    QPen getAudioVolumeLinePen() const { return audio_volume_line_pen; }
    QBrush getAudioVolumeLineBrush() const { return audio_volume_line_brush; }
    void setAudioVolumeLinePen(const QPen &pen) { audio_volume_line_pen = pen; }
    void setAudioVolumeLineBrush(const QBrush &brush) { audio_volume_line_brush = brush; }
#endif
    QPen getAudioVolumeRectPen() const { return audio_volume_rect_pen; }
    QBrush getAudioVolumeRectBrush() const { return audio_volume_rect_brush; }
    void setAudioVolumeRectPen(const QPen &pen) { audio_volume_rect_pen = pen; }
    void setAudioVolumeRectBrush(const QBrush &brush) { audio_volume_rect_brush = brush; }
signals:

public slots:
    void onUpdate(qreal);
protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
private:
    void initPen();
    void initBrush();
    void initVolumeSize();
    void updateAudioRect(uint32_t);
    void onPaintVolume(const QRect&, QPainter&);
    void onPaintVolumeLine(const QRect&, QPainter&);
    void onPaintVolumeRect(const QRect&, QPainter&, uint32_t, qreal);

    bool is_update_volume;
    int audio_timer_id;

    uint32_t audio_volume_width_margin;
    uint32_t audio_volume_height_margin;
    uint32_t audio_volume_start;
    uint32_t audio_rect_size;
    uint32_t audio_rect_space;
    qreal *audio_volume_array;

#ifdef  LOCAL_AUDIO_VOLUMN_LINE
    uint32_t audio_volume_line_width;
    uint32_t audio_volume_line_height;
    QPen audio_volume_line_pen;
    QBrush audio_volume_line_brush;
#endif

    uint32_t audio_volume_rect_width;
    uint32_t audio_volume_rect_height;
    QPen audio_volume_rect_pen;
    QBrush audio_volume_rect_brush;
};

#endif // LOCALAUDIOWIDGET_H
