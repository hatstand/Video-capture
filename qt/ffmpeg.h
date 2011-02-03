#ifndef FFMPEG_H
#define FFMPEG_H

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavdevice/avdevice.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
}

#include <cstdlib>

#include <QImage>
#include <QMutex>
#include <QThread>

class Ffmpeg : public QThread {
Q_OBJECT
public:
  Ffmpeg(QObject* parent = 0);
  virtual ~Ffmpeg();
  QImage getImage();

signals:
  void frameAvailable();

public slots:
  void stop();

private:
  void run();
  void error(const QString& s);

  AVFormatContext* format_ctx_;
  AVFormatParameters* format_params_;

  AVCodecContext* codec_ctx_;
  AVCodec* codec_;

  int video_stream_;

  AVFrame* yuv_frame_;
  AVFrame* rgb_frame_;

  SwsContext* sws_ctx_;

  QMutex mutex_;

  bool run_;
};

#endif
