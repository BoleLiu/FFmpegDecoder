#ifndef ADECODER_H
#define ADECODER_H

#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

typedef struct adecode_result {
    int success;
    int eof;
    unsigned char* decoded_buffer;
    int decoded_size;
    int64_t pts;
} adecode_result_t;

typedef struct _adecoder {
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    SwrContext *swr_ctx;
    uint8_t ** swr_dst_buffer;

    AVPacket avpkt;
    AVFrame *avFrame;

    AVRational in_time_base;
    AVRational out_time_base;

    unsigned char *extradata;
    int extradata_size;

    int max_frame_size;

    unsigned char* output_buffer;
    int output_buffer_size;

    int sample_rate;
    int channel_count;
} adecoder_t;

#if defined __cplusplus
extern "C"
{
#endif // __cplusplus

void adecoder_init();

adecoder_t * init(adecoder_t * decoder);
adecoder_t *adecoder_open_file(char *filepath);
adecoder_t *adecoder_open_asset(int fd, long offset, long length);
int adecoder_close(adecoder_t *decoder);

adecode_result_t adecoder_decode_buffer(adecoder_t *decoder, unsigned char* dst, unsigned char* src, int size, int64_t pts);

int adecoder_get_samplerate(adecoder_t *decoder);
int adecoder_get_channel_count(adecoder_t *decoder);

#if defined __cplusplus
}
#endif // __cplusplus
#endif