//
// Created by Jingbo Liu on 2019-09-15.
//

#ifndef FFMPEGDECODER_VDECODER_H
#define FFMPEGDECODER_VDECODER_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <android/native_window.h>
#include "android/native_window_jni.h"

#define VDECODER_FMT_YUV420P AV_PIX_FMT_YUV420P
#define VDECODER_FMT_NV21    AV_PIX_FMT_NV21
#define VDECODER_FMT_NV12    AV_PIX_FMT_NV12
#define VDECODER_FMT_YUYV422 AV_PIX_FMT_YUYV422
#define VDECODER_FMT_YVYU422 AV_PIX_FMT_YVYU422
#define VDECODER_FMT_YUV422P AV_PIX_FMT_YUV422P
#define VDECODER_FMT_UYVY422 AV_PIX_FMT_UYVY422
#define VDECODER_FMT_GRAY8   AV_PIX_FMT_GRAY8
#define VDECODER_FMT_RGB565  AV_PIX_FMT_RGB565
#define VDECODER_FMT_RGB24   AV_PIX_FMT_RGB24
#define VDECODER_FMT_ARGB    AV_PIX_FMT_ARGB
#define VDECODER_FMT_ABGR    AV_PIX_FMT_ABGR
#define VDECODER_FMT_RGBA    AV_PIX_FMT_RGBA

#define DEFAULT_OUTPUT_FMT AV_PIX_FMT_YUV420P // VDECODER_FMT_RGB565

typedef struct vdecode_result {
    int success;
    int eof;
    int got_frame;
    int img_width;
    int img_height;
    unsigned char* img_buffer;
    int img_size;
    int64_t pts;
} vdecode_result_t;

typedef struct _vdecoder {
    AVFormatContext *ic;
    AVCodecContext *context;
    struct SwsContext * sws_ctx;

    AVPacket avpkt;
    AVFrame *avframe;
    AVFrame *output_frame;

    int output_fmt;
    unsigned char *output_buffer;
    int output_buffer_size;

    AVRational in_time_base;
    AVRational out_time_base;

    unsigned char *extradata;
    int extradata_size;

    int output_width;
    int output_height;

    ANativeWindow *pANativeWindow;
    ANativeWindow_Buffer nativeWindow_buffer;
} vdecoder_t;

#if defined __cplusplus
extern "C"
{
#endif // __cplusplus

void vdecoder_init();

vdecoder_t *vdecoder_open(char *filepath);
int vdecoder_close(vdecoder_t *decoder);

int vdecoder_set_output_format(vdecoder_t *decoder, int fmt);

vdecode_result_t vdecoder_decode_buffer(vdecoder_t * decoder, unsigned char * dst, unsigned char *input, int size, int64_t pts);

int vdecoder_decode_buffer_to_surface(vdecoder_t *decoder, unsigned char *input, int size, int oututWidth, int outputHeight);

int vdecoder_decode_get_width(vdecoder_t *decoder);
int vdecoder_decode_get_height(vdecoder_t *decoder);

#if defined __cplusplus
}
#endif // __cplusplus

#endif //FFMPEGDECODER_VDECODER_H
