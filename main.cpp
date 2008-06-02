extern "C" {
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avdevice.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>
}

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace std;

void save_frame(AVFrame* frame, int w, int h) {
	FILE* file = fopen("screeny.ppm", "wb");

	fprintf(file, "P6\n%d %d\n255\n", w, h);

	for (int y = 0; y < h; ++y)
		fwrite(frame->data[0] + y*frame->linesize[0], 1, w*3, file);
	
	fclose(file);
}

int main (int argc, char** argv) {
	// Initialise SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
		cerr << "Could not init SDL" << endl;
		return -1;
	}

	// Load ffmpeg codecs and devices (v4l2)
	av_register_all();
	avdevice_register_all();

	AVFormatContext* format_ctx;

	AVFormatParameters* format_params = (AVFormatParameters*)malloc(sizeof(AVFormatParameters));

	// Set format parameters (time_base = 59.94)
	format_params->channel = 1;
	format_params->standard = "pal-60";
	format_params->width = 640;
	format_params->height = 480;
	format_params->time_base.den = 30000;
	format_params->time_base.num = 1001;
	//format_params->pix_fmt = PIX_FMT_YUV410P;
	
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

	// Find the video stream
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

	// Get the codec context for the video stream
	AVCodecContext* codec_ctx = format_ctx->streams[video_stream]->codec;

	// Get the actual codec for the video stream
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);

	if (codec == NULL) {
		cerr << "Could not find codec" << endl;
		return -4;
	}

	if (codec->capabilities & CODEC_CAP_TRUNCATED)
		codec_ctx->flags |= CODEC_FLAG_TRUNCATED;

	// Load codec
	if (avcodec_open(codec_ctx, codec) < 0) {
		cerr << "Could not open codec" << endl;
		return -5;
	}

	// Allocate a YUV420P frame and an RGB frame.
	AVFrame* frame = avcodec_alloc_frame();
	AVFrame* frame_rgb = avcodec_alloc_frame();

	// Malloc the rgb frame.
	int size = avpicture_get_size(PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);
	uint8_t* buffer = (uint8_t*) av_malloc(size * sizeof(uint8_t));
	avpicture_fill((AVPicture*)frame_rgb, buffer, PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);

	// Initialise SWScaler for colourspace conversion.
	SwsContext* ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
		codec_ctx->width, codec_ctx->height, PIX_FMT_RGB24,
		0, NULL, NULL, NULL);
	if (!ctx) {
		cerr << "Could not get sw context" << endl;
		return -6;
	}

	// Create a window the same height as the video
	SDL_Surface* screen = SDL_SetVideoMode(codec_ctx->width, codec_ctx->height, 0, 0);
	if (!screen) {
		cerr << "Could not set video mode" << endl;
		return -1;
	}

	// Create an overlay of the right format (YV12, YUV420P has U & V switched).
	SDL_Overlay* bmp = SDL_CreateYUVOverlay(codec_ctx->width, codec_ctx->height, SDL_YV12_OVERLAY, screen);

	AVPacket packet;
	// Actually read a frame
	while (av_read_frame(format_ctx, &packet) >= 0) {
		if (packet.stream_index == video_stream) {
			int frame_finished = 0;
			// Decode frame.
			avcodec_decode_video(codec_ctx, frame, &frame_finished, packet.data, packet.size);

			// We have a full frame
			if (frame_finished) {

				// Set overlay to use this frame's data.
				SDL_LockYUVOverlay(bmp);

				// 1 & 2 switched for YUV420P->YV12 conversion
				memcpy(bmp->pixels[0], frame->data[0], frame->linesize[0] * codec_ctx->height);
				memcpy(bmp->pixels[1], frame->data[2], frame->linesize[2] * codec_ctx->height);
				memcpy(bmp->pixels[2], frame->data[1], frame->linesize[1] * codec_ctx->height);

				bmp->pitches[0] = frame->linesize[0];
				bmp->pitches[1] = frame->linesize[2];
				bmp->pitches[2] = frame->linesize[1];

				SDL_UnlockYUVOverlay(bmp);

				SDL_Rect rect;
				rect.x = 0;
				rect.y = 0;
				rect.w = codec_ctx->width;
				rect.h = codec_ctx->height;

				// Actually display frame
				SDL_DisplayYUVOverlay(bmp, &rect);
				
				// Convert from YUV420P to RGB
				sws_scale(ctx, frame->data, frame->linesize, 0,
					codec_ctx->height, ((AVPicture*)frame_rgb)->data, ((AVPicture*)frame_rgb)->linesize);
				
				// Save frame
				save_frame(frame_rgb, codec_ctx->width, codec_ctx->height);
			}
		}

		av_free_packet(&packet);

		SDL_Event event;
		SDL_PollEvent(&event);
		switch (event.type) {
			case SDL_QUIT:
				SDL_Quit();
				return 0;
			default:
				break;
		}
	}

	return 0;
}
