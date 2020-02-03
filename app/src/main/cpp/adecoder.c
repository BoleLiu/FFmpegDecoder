#include <adecoder.h>
#include <logger.h>

static int adecoder_swr_convert(adecoder_t * decoder);

void adecoder_init() {
    av_register_all();
    LOGI("adecoder_init success !");
}

adecoder_t *adecoder_open_file(char *filepath) {
    adecoder_t *decoder = (adecoder_t *) malloc(sizeof(adecoder_t));
    memset(decoder, 0, sizeof(adecoder_t));

    decoder->pFormatCtx = avformat_alloc_context();
    int ret = avformat_open_input(&decoder->pFormatCtx, filepath, NULL, NULL);
    if (ret != 0) {
        LOGE("adecoder_open failed to open %s ret = %d", filepath, ret);
        adecoder_close(decoder);
        return NULL;
    }

    return init(decoder);

//    ret = avformat_find_stream_info(decoder->pFormatCtx, NULL);
//    if (ret < 0) {
//        LOGE("adecoder_open failed to find stream info, ret = %d", ret);
//        adecoder_close(decoder);
//        return NULL;
//    }
//
//    int audio_stream_idx = av_find_best_stream(decoder->pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
//    if (audio_stream_idx < 0) {
//        LOGE("adecoder_open failed to find stream info");
//        adecoder_close(decoder);
//        return NULL;
//    }
//
//    AVCodecParameters *parameters = decoder->pFormatCtx->streams[audio_stream_idx]->codecpar;
//    decoder->pCodecCtx = avcodec_alloc_context3(NULL);
//    if (decoder->pCodecCtx == NULL) {
//        LOGE("adecoder_open alloc codec context failed");
//        adecoder_close(decoder);
//        return NULL;
//    }
//
//    // 将 AVCodecParameters 中的音视频配置参数赋值给 AVCodecContext
//    avcodec_parameters_to_context(decoder->pCodecCtx, parameters);
//
//    int codec_id = decoder->pCodecCtx->codec_id;
//    AVCodec *avcodec = avcodec_find_decoder(codec_id);
//    if (avcodec == NULL) {
//        LOGE("adecoder_open avcodec_find_codec failed !");
//        adecoder_close(decoder);
//        return NULL;
//    }
//
//    // open 过程中会解析 AVCodecContext 中通过 AVCodecParameters 赋值给过来的 extradata 等信息
//    if (avcodec_open2(decoder->pCodecCtx, avcodec, NULL)) {
//        LOGE("adecoder_open avcodec_open2 failed !");
//        adecoder_close(decoder);
//        return NULL;
//    }
//
//    if (decoder->extradata == NULL) {
//        decoder->extradata = (unsigned char *)malloc(parameters->extradata_size);
//    }
//    memcpy(decoder->extradata, parameters->extradata, parameters->extradata_size);
//    decoder->extradata_size = parameters->extradata_size;
//
//    decoder->in_time_base = decoder->pFormatCtx->streams[audio_stream_idx]->time_base;
//    AVRational out_time_base = {1, decoder->pCodecCtx->sample_rate};
//    decoder->out_time_base = out_time_base;
//
//    LOGI("adecoder in_time_base = %lf out_time_base = %lf", av_q2d(decoder->in_time_base), av_q2d(decoder->out_time_base));
//
//    decoder->sample_rate = parameters->sample_rate;
//    decoder->channel_count = parameters->channels;
//    decoder->max_frame_size = decoder->sample_rate * 4;
//
//    decoder->avFrame = av_frame_alloc();
//    if (!decoder->avFrame) {
//        LOGE("adecoder_open av_frame_alloc failed !");
//        adecoder_close(decoder);
//        return NULL;
//    }
//
//    decoder->swr_ctx = NULL;
//
//    decoder->output_buffer = NULL;
//    decoder->output_buffer_size = 0;
//
//    decoder->avpkt.data = NULL;
//    decoder->avpkt.size = 0;
//
//    av_init_packet(&decoder->avpkt);
//
//    LOGI("adecoder_open %s success !", filepath);
//    return decoder;
}

adecoder_t *adecoder_open_asset(int fd, long offset, long length) {
    adecoder_t *decoder = (adecoder_t *) malloc(sizeof(adecoder_t));
    memset(decoder, 0, sizeof(adecoder_t));

    char filepath[20];
    sprintf(filepath, "pipe:%d", fd);

    decoder->pFormatCtx =  avformat_alloc_context();
    decoder->pFormatCtx->skip_initial_bytes = offset;
    decoder->pFormatCtx->iformat = av_find_input_format("mp3");

    LOGI("adecoder_open_asset iFormat = %p skip_bytes = %lld", decoder->pFormatCtx->iformat, decoder->pFormatCtx->skip_initial_bytes);

    int ret = avformat_open_input(&decoder->pFormatCtx, filepath, NULL, NULL);
    if (ret != 0) {
        LOGE("adecoder_open failed to open %s ret = %d", filepath, ret);
        adecoder_close(decoder);
        return NULL;
    }

    return init(decoder);
}

int adecoder_close(adecoder_t *decoder) {
    if (decoder == NULL) {
        return -1;
    }
    
    if (decoder->avFrame) {
        av_frame_free(&decoder->avFrame);
    }

    if (decoder->swr_dst_buffer) {
        av_freep(&decoder->swr_dst_buffer[0]);
    }
    av_freep(&decoder->swr_dst_buffer);

    if (decoder->swr_ctx) {
        swr_free(&decoder->swr_ctx);
    }

    if (decoder->output_buffer) {
        free(decoder->output_buffer);
    }

    if (decoder->extradata) {
        free(decoder->extradata);
    }

    if (decoder->pFormatCtx) {
        avformat_close_input(&decoder->pFormatCtx);
    }

    if (decoder->pCodecCtx) {
        avcodec_free_context(&decoder->pCodecCtx);
    }

    av_packet_unref(&decoder->avpkt);
    free(decoder);
    return 0;
}

adecode_result_t adecoder_decode_buffer(adecoder_t *decoder, unsigned char* dst, unsigned char* src, int size, int64_t pts) {
    adecode_result_t result;
    result.success = 0;
    result.eof = 0;
    result.decoded_buffer = NULL;
    result.decoded_size = 0;

    if (decoder == NULL) {
        return result;
    }

    decoder->avpkt.data = src;
    decoder->avpkt.size = size;
    decoder->avpkt.pts = pts;

    int64_t rescale_pts = av_rescale_q(decoder->avpkt.pts, decoder->in_time_base, decoder->out_time_base);

    int ret = avcodec_send_packet(decoder->pCodecCtx, src == NULL ? NULL : &decoder->avpkt);
    if (ret != 0) {
        LOGE("adecoder_decode_buffer avcodec_send_packet failed !");
        return result;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(decoder->pCodecCtx, decoder->avFrame);

        if (ret == AVERROR(EAGAIN)) {
            LOGI("adecoder output is not available right now !");
            break;
        } else if (ret == AVERROR_EOF) {
            LOGI("adecoder_decode_buffer output reached eof !");
            result.success = 1;
            result.pts = 0;
            result.eof = 1;
            return result;
        } else if (ret < 0) {
            LOGE("adecoder error during decoding !");
            return result;
        }

        if (adecoder_swr_convert(decoder) < 0) {
            return result;
        }
    }

    memcpy(dst, decoder->output_buffer, decoder->output_buffer_size);

    result.success = 1;
    result.decoded_buffer = decoder->output_buffer;
    result.decoded_size = decoder->output_buffer_size;
    result.pts = rescale_pts;

    decoder->output_buffer_size = 0;
    return result;
}

int adecoder_get_samplerate(adecoder_t *decoder) {
    if (decoder == NULL || decoder->pCodecCtx == NULL) {
        LOGE("adecoder_get_samplerate failed !");
        return -1;
    }
    return (decoder->sample_rate > 0) ? decoder->sample_rate : decoder->pCodecCtx->sample_rate;
}

int adecoder_get_channel_count(adecoder_t *decoder) {
    if (decoder == NULL || decoder->pCodecCtx == NULL) {
        LOGE("adecoder_get_channel_count failed !");
        return -1;
    }
    return (decoder->channel_count > 0) ? decoder->channel_count : decoder->pCodecCtx->channels;
}

static int adecoder_swr_convert(adecoder_t * decoder) {
    enum AVSampleFormat in_sample_fmt = decoder->pCodecCtx->sample_fmt;
    int64_t in_channel_layout = av_get_default_channel_layout(decoder->pCodecCtx->channels);
    int in_sample_rate = decoder->pCodecCtx->sample_rate;

    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_nb_channels = av_get_channel_layout_nb_channels(in_channel_layout);
    int out_sample_rate = in_sample_rate;
    int out_nb_samples = av_rescale_rnd(decoder->avFrame->nb_samples, out_sample_rate, in_sample_rate, AV_ROUND_UP);

    if (decoder->output_buffer == NULL) {
        decoder->output_buffer = (unsigned char *) malloc(decoder->max_frame_size);
    }

    if (decoder->output_buffer == NULL) {
        LOGE("adecoder_swr_convert malloc output buffer failed !");
        return -1;
    }

    if (decoder->swr_ctx == NULL) {
        decoder->swr_ctx = swr_alloc_set_opts(decoder->swr_ctx,
                in_channel_layout, out_sample_fmt, in_sample_rate,
                in_channel_layout, in_sample_fmt, in_sample_rate,
                0, NULL);
        swr_init(decoder->swr_ctx);
    }

    if (decoder->swr_dst_buffer == NULL) {
        int ret = av_samples_alloc_array_and_samples(&decoder->swr_dst_buffer, NULL, out_nb_channels,
                out_nb_samples, out_sample_fmt, 0);
        if (ret < 0) {
            LOGE("adecoder swr_convert could not allocate dst samples");
            return -1;
        }
    }

    swr_convert(decoder->swr_ctx, decoder->swr_dst_buffer, out_nb_samples,
                (const uint8_t **) decoder->avFrame->data, decoder->avFrame->nb_samples);

    int dst_buffer_size = av_samples_get_buffer_size(NULL,
                                                     out_nb_channels,
                                                     decoder->avFrame->nb_samples,
                                                     out_sample_fmt,
                                                     1);

    memcpy(decoder->output_buffer + decoder->output_buffer_size, decoder->swr_dst_buffer[0], dst_buffer_size);

    decoder->output_buffer_size += dst_buffer_size;

    return 0;
}

adecoder_t * init(adecoder_t * decoder) {
    int ret;
    ret = avformat_find_stream_info(decoder->pFormatCtx, NULL);
    if (ret < 0) {
        LOGE("adecoder_open failed to find stream info, ret = %d", ret);
        adecoder_close(decoder);
        return NULL;
    }

    int audio_stream_idx = av_find_best_stream(decoder->pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (audio_stream_idx < 0) {
        LOGE("adecoder_open failed to find stream info");
        adecoder_close(decoder);
        return NULL;
    }

    AVCodecParameters *parameters = decoder->pFormatCtx->streams[audio_stream_idx]->codecpar;

    AVCodec *avcodec = avcodec_find_decoder(parameters->codec_id);
    if (avcodec == NULL) {
        LOGE("adecoder_open avcodec_find_codec failed !");
        adecoder_close(decoder);
        return NULL;
    }

    decoder->pCodecCtx = avcodec_alloc_context3(avcodec);
    if (decoder->pCodecCtx == NULL) {
        LOGE("adecoder_open alloc codec context failed");
        adecoder_close(decoder);
        return NULL;
    }

    // 将 AVCodecParameters 中的音视频配置参数赋值给 AVCodecContext
    avcodec_parameters_to_context(decoder->pCodecCtx, parameters);

    // open 过程中会解析 AVCodecContext 中通过 AVCodecParameters 赋值给过来的 extradata 等信息
    if (avcodec_open2(decoder->pCodecCtx, avcodec, NULL)) {
        LOGE("adecoder_open avcodec_open2 failed !");
        adecoder_close(decoder);
        return NULL;
    }

    if (decoder->extradata == NULL) {
        decoder->extradata = (unsigned char *)malloc(parameters->extradata_size);
    }
    memcpy(decoder->extradata, parameters->extradata, parameters->extradata_size);
    decoder->extradata_size = parameters->extradata_size;

    decoder->in_time_base = decoder->pFormatCtx->streams[audio_stream_idx]->time_base;
    decoder->out_time_base = (AVRational){1, decoder->pCodecCtx->sample_rate};

    LOGI("adecoder in_time_base = %lf out_time_base = %lf", av_q2d(decoder->in_time_base), av_q2d(decoder->out_time_base));

    decoder->sample_rate = parameters->sample_rate;
    decoder->channel_count = parameters->channels;
    decoder->max_frame_size = decoder->sample_rate * 4;

    LOGI("adecoder channel = %d, samplerate = %d， samplefmt = %d %d",
         decoder->channel_count, decoder->sample_rate, decoder->pCodecCtx->sample_fmt, AV_SAMPLE_FMT_S32P);

    decoder->avFrame = av_frame_alloc();
    if (!decoder->avFrame) {
        LOGE("adecoder_open av_frame_alloc failed !");
        adecoder_close(decoder);
        return NULL;
    }

    decoder->swr_ctx = NULL;

    decoder->output_buffer = NULL;
    decoder->output_buffer_size = 0;

    decoder->avpkt.data = NULL;
    decoder->avpkt.size = 0;

    av_init_packet(&decoder->avpkt);

//    LOGI("adecoder_open %s success !", filepath);
    return decoder;
}