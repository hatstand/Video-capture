#include "ffmpeg.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>

Ffmpeg::Ffmpeg(QObject* parent) : QThread(parent) {
  av_register_all();
  avdevice_register_all();

  format_params_ = (AVFormatParameters*) calloc(1, sizeof(AVFormatParameters));
  //format_params_->channel = 1;
  //format_params_->standard = "pal-60";
  format_params_->width = 640;
  format_params_->height = 480;
  format_params_->time_base.den = 30000;
  format_params_->time_base.num = 1001;

  format_ctx_ = av_alloc_format_context();

  AVInputFormat* input_format = av_find_input_format("video4linux2");
  if (!input_format)
    error("av_find_input_format");

  const char* filename = "/dev/video0";
  if (av_open_input_file(&format_ctx_, filename, input_format, 0, format_params_) != 0)
    error("av_open_input_file");

  if (av_find_stream_info(format_ctx_) < 0)
    error("av_find_stream_info");

  dump_format(format_ctx_, 0, filename, false);

  video_stream_ = -1;
  for (unsigned int i = 0; i < format_ctx_->nb_streams; ++i) {
    if (format_ctx_->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
      video_stream_ = i;
      break;
    }
  }

  if (video_stream_ == -1)
    error("video_stream");
  
  codec_ctx_ = format_ctx_->streams[video_stream_]->codec;
  codec_ = avcodec_find_decoder(codec_ctx_->codec_id);

  if (codec_ == NULL)
    error("codec");

  if (codec_->capabilities & CODEC_CAP_TRUNCATED)
    codec_ctx_->flags |= CODEC_CAP_TRUNCATED;

  if (avcodec_open(codec_ctx_, codec_) < 0)
    error("avcodec_open");

  yuv_frame_ = avcodec_alloc_frame();
  rgb_frame_ = avcodec_alloc_frame();

  // Allocate RGB frame. YUV is allocated by ffmpeg on frame read.
  int frame_size = avpicture_get_size(PIX_FMT_RGB24, codec_ctx_->width, codec_ctx_->height);
  uint8_t* buffer = (uint8_t*) av_malloc(frame_size * sizeof(uint8_t));
  avpicture_fill((AVPicture*)rgb_frame_, buffer, PIX_FMT_RGB24, codec_ctx_->width, codec_ctx_->height);

  sws_ctx_ = sws_getContext(codec_ctx_->width, codec_ctx_->height, codec_ctx_->pix_fmt,
    codec_ctx_->width, codec_ctx_->height, PIX_FMT_RGB24,
    SWS_BICUBIC, NULL, NULL, NULL);

  if (!sws_ctx_)
    error("sws_getContext");
}

Ffmpeg::~Ffmpeg() {
}


void Ffmpeg::run() {
  run_ = true;

  AVPacket packet;
  while (av_read_frame(format_ctx_, &packet) >= 0) {
    if (packet.stream_index == video_stream_) {
      int frame_finished = 0;

      // Lock so that we can't get half finished frames.
      QMutexLocker l(&mutex_);
      avcodec_decode_video(codec_ctx_, yuv_frame_, &frame_finished, packet.data, packet.size);

      if (frame_finished) {
        sws_scale(sws_ctx_, yuv_frame_->data, yuv_frame_->linesize, 0,
          codec_ctx_->height, ((AVPicture*)rgb_frame_)->data, ((AVPicture*)rgb_frame_)->linesize);

        emit frameAvailable();
      }
    }

    av_free_packet(&packet);

    QCoreApplication::processEvents();

    if (!run_)
      return;
  }
}

void Ffmpeg::error(const QString& s) {
  qFatal(s.toAscii());
}

QImage Ffmpeg::getImage() {
  QMutexLocker l(&mutex_);
  return QImage(rgb_frame_->data[0], codec_ctx_->width, codec_ctx_->height, QImage::Format_RGB888);
}

void Ffmpeg::stop() {
  run_ = false;
}
