extern "C" {
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avdevice.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>
}

#include <cstdlib>
#include <iostream>

using namespace std;

int main (int argc, char** argv) {
	av_register_all();
	avdevice_register_all();

	AVFormatContext* format_ctx;

	AVFormatParameters* format_params = (AVFormatParameters*)malloc(sizeof(AVFormatParameters));

	format_params->channel = 0;
	format_params->standard = "pal";
	format_params->width = 640;
	format_params->height = 480;
	format_params->time_base.den = 60000;
	format_params->time_base.num = 1001;
	
	const char* filename = "/dev/video0";

	AVInputFormat* iformat = av_find_input_format("video4linux2");
	cerr << (int)iformat << endl;

	if (av_open_input_file(&format_ctx, filename, iformat, 0, format_params) != 0) {
		cerr << "Could not open file" << endl;
		return -1;
	}

	if (av_find_stream_info(format_ctx) < 0) {
		cerr << "Could not find stream" << endl;
		return -2;
	}

	dump_format(format_ctx, 0, argv[1], false);

	int video_stream = -1;
	for (int i = 0; i < format_ctx->nb_streams; ++i) {
		if (format_ctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			video_stream = i;
			break;
		}
	}

	if (video_stream == -1) {
		cerr << "No video stream" << endl;
		return -3;
	}

	AVCodecContext* codec_ctx = format_ctx->streams[video_stream]->codec;

	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);

	if (codec == NULL) {
		cerr << "Could not find codec" << endl;
		return -4;
	}

	if (codec->capabilities & CODEC_CAP_TRUNCATED)
		codec_ctx->flags |= CODEC_FLAG_TRUNCATED;

	if (avcodec_open(codec_ctx, codec) < 0) {
		cerr << "Could not open codec" << endl;
		return -5;
	}

	AVFrame* frame = avcodec_alloc_frame();
	AVFrame* frame_rgb = avcodec_alloc_frame();

	int size = avpicture_get_size(PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);
	uint8_t* buffer = (uint8_t*) av_malloc(size * sizeof(uint8_t));
	avpicture_fill((AVPicture*)frame_rgb, buffer, PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);

	AVPacket packet;
	SwsContext* ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
		codec_ctx->width, codec_ctx->height, PIX_FMT_RGB24,
		0, NULL, NULL, NULL);

	while (av_read_frame(format_ctx, &packet) >= 0) {
		if (packet.stream_index == video_stream) {
			int frame_finished = 0;
			avcodec_decode_video(codec_ctx, frame, &frame_finished, packet.data, packet.size);

			if (frame_finished) {
			//	img_convert((AVPicture*)frame_rgb, PIX_FMT_RGB24, (AVPicture*)frame,
			//		codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height);
				

				if (!ctx) {
					cerr << "Could not get sw context" << endl;
					return -6;
				}

				int rgb_stride[3] = { codec_ctx->width * 3, codec_ctx->width * 3, codec_ctx->width * 3 };
				int stride[3] = { codec_ctx->width, codec_ctx->width, codec_ctx->width };
				sws_scale(ctx, frame->data, frame->linesize, 0,
					codec_ctx->height, ((AVPicture*)frame_rgb)->data, ((AVPicture*)frame_rgb)->linesize);
			}
		}

		av_free_packet(&packet);
	}

	return 0;
}
