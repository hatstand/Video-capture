#include "ffmpeg.h"

#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>

#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>

// http://dranger.com/ffmpeg/tutorial05.html

int GetBuffer(AVCodecContext* context, AVFrame* frame) {
  int ret = avcodec_default_get_buffer(context, frame);
  uint64_t* pts_ms = new uint64_t;
  *pts_ms = *reinterpret_cast<uint64_t*>(context->opaque);
  frame->opaque = pts_ms;
  return ret;
}

void ReleaseBuffer(AVCodecContext* context, AVFrame* frame) {
  if (frame) {
    delete reinterpret_cast<uint64_t*>(frame->opaque);
  }
  avcodec_default_release_buffer(context, frame);
}

Ffmpeg::Ffmpeg(QObject* parent) : QThread(parent), last_pts_ms_(0.0) {
  av_register_all();
  avdevice_register_all();

  format_params_ = (AVFormatParameters*) calloc(1, sizeof(AVFormatParameters));
  //format_params_->channel = 1;
  //format_params_->standard = "pal-60";
  format_params_->width = 640;
  format_params_->height = 480;
  format_params_->time_base.den = 30000;
  format_params_->time_base.num = 1001;

  format_ctx_ = avformat_alloc_context();

  /*AVInputFormat* input_format = av_find_input_format("video4linux2");
  if (!input_format)
    error("av_find_input_format");
    */

  const char* filename = "foo.mp4";
  if (av_open_input_file(&format_ctx_, filename, NULL, 0, format_params_) != 0)
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

  codec_ctx_->opaque = new uint64_t;
  codec_ctx_->get_buffer = GetBuffer;
  codec_ctx_->release_buffer = ReleaseBuffer;

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

  moveToThread(this);
}

Ffmpeg::~Ffmpeg() {
}

void Ffmpeg::run() {
  timer_ = new QTimer();
  timer_->setSingleShot(false);
  connect(timer_, SIGNAL(timeout()), SLOT(Timeout()));
  run_ = true;
  last_frame_time_us_ = av_gettime() - 1;
  ProcessFrame();
  exec();
}

void Ffmpeg::ProcessFrame() {
  int64_t time_us = av_gettime();
  int64_t actual_delay_us = time_us - last_frame_time_us_;
  printf("Delay: %" PRId64 "us %fms\n", actual_delay_us, frame_delay_ms_);
  if (actual_delay_us / 1000 < int(frame_delay_ms_)) {  // Closest ms.
    int delay = (frame_delay_ms_ * 1000.0 - actual_delay_us) / 1000.0;
    printf("Waiting: %d\n", delay);
    timer_->start(delay);
    return;
  }

  AVPacket packet;
  while (av_read_frame(format_ctx_, &packet) >= 0) {
    if (packet.stream_index == video_stream_) {
      int frame_finished = 0;

      *reinterpret_cast<uint64_t*>(codec_ctx_->opaque) = packet.pts;

      avcodec_decode_video2(codec_ctx_, yuv_frame_, &frame_finished, &packet);

      double pts_ms = 0.0;

      uint64_t frame_pts_ms = *reinterpret_cast<uint64_t*>(yuv_frame_->opaque);
      if (packet.dts == AV_NOPTS_VALUE && frame_pts_ms != AV_NOPTS_VALUE) {
        pts_ms = frame_pts_ms;
      } else if (packet.dts != AV_NOPTS_VALUE) {
        pts_ms = packet.dts;
      }
      pts_ms *= av_q2d(codec_ctx_->time_base);

      if (frame_finished) {
        {
          // Lock so that we can't get half finished frames.
          QMutexLocker l(&mutex_);
          sws_scale(sws_ctx_, yuv_frame_->data, yuv_frame_->linesize, 0,
            codec_ctx_->height, ((AVPicture*)rgb_frame_)->data, ((AVPicture*)rgb_frame_)->linesize);
        }

        pts_ms = SyncVideo(yuv_frame_, pts_ms);

        emit frameAvailable();

        frame_delay_ms_ = (pts_ms - last_pts_ms_) / 2;
        printf("PTS: %f %f Calculated delay: %f\n", pts_ms, last_pts_ms_, frame_delay_ms_);
        timer_->start(frame_delay_ms_);
        last_pts_ms_ = pts_ms;
        last_frame_time_us_ = av_gettime();

        av_free_packet(&packet);
        return;
      }
    }
  }
}

void Ffmpeg::Timeout() {
  timer_->stop();
  ProcessFrame();
}

double Ffmpeg::SyncVideo(AVFrame* frame, double pts_ms) {
  if (pts_ms != 0.0) {
    video_clock_ = pts_ms;
  } else {
    pts_ms = video_clock_;
  }

  double frame_delay = av_q2d(codec_ctx_->time_base);
  frame_delay += frame->repeat_pict * (frame_delay * 0.5);
  video_clock_ += frame_delay;
  return pts_ms;
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
