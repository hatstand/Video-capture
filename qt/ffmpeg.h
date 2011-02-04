#ifndef FFMPEG_H
#define FFMPEG_H

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavdevice/avdevice.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
}

#include <cstdlib>

#include <QEventLoop>
#include <QImage>
#include <QMutex>
#include <QThread>
#include <QTimer>

class Ffmpeg : public QThread {
Q_OBJECT
public:
  Ffmpeg(const char* filename, QObject* parent = 0);
  virtual ~Ffmpeg();
  QImage getImage();

signals:
  void frameAvailable();

public slots:
  void stop();
  void Timeout();

private:
  void run();
  void error(const QString& s);

  void ProcessFrame();
  double SyncVideo(AVFrame* frame, double pts);

  AVFormatContext* format_ctx_;
  AVFormatParameters* format_params_;

  AVCodecContext* codec_ctx_;
  AVCodec* codec_;

  int video_stream_;

  AVFrame* yuv_frame_;
  AVFrame* rgb_frame_;
  AVFrame* deint_frame_;

  SwsContext* sws_ctx_;

  double video_clock_;
  double last_pts_ms_;
  double frame_delay_ms_;
  int64_t last_frame_time_us_;

  QTimer* timer_;

  QMutex mutex_;

  bool run_;
};

#endif
