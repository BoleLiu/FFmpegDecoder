//
// Created by Jingbo Liu on 2019-09-15.
//

#include "include/vdecoder.h"
#include <logger.h>
#include <libavutil/imgutils.h>

static int vdecoder_sws_convert(vdecoder_t * decoder, int output_width, int output_height);

void vdecoder_init()
{
    av_register_all();
    // avformat_network_init();
    LOGI("vdecoder_init success ");
}

vdecoder_t *vdecoder_open(char *filepath)
{
    if (filepath == NULL) {
        return NULL;
    }
    vdecoder_t * decoder = (vdecoder_t *) malloc(sizeof(vdecoder_t));
    memset(decoder, 0, sizeof(vdecoder_t));

    decoder->ic = avformat_alloc_context();
    int ret = avformat_open_input(&decoder->ic, filepath, NULL, NULL);
    if (ret < 0) {
        LOGE("vdecoder_open failed to open %s, ret = %d !", filepath, ret);
        vdecoder_close(decoder);
        return NULL;
    }

    ret = avformat_find_stream_info(decoder->ic, NULL);
    if (ret < 0) {
        LOGE("vdecoder_open failed to find stream info, ret = %d !", ret);
        vdecoder_close(decoder);
        return NULL;
    }

    int video_stream_idx = av_find_best_stream(decoder->ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_idx < 0) {
        LOGE("vdecoder_open failed to find stream info !");
        vdecoder_close(decoder);
        return NULL;
    }

    AVCodecParameters *parameters = decoder->ic->streams[video_stream_idx]->codecpar;

    AVCodec *avcodec = avcodec_find_decoder(parameters->codec_id);
    if (avcodec == NULL) {
        LOGE("vdecoder_open avcodec_find_decoder failed !");
        vdecoder_close(decoder);
        return NULL;
    }

    decoder->context = avcodec_alloc_context3(avcodec);
    if (decoder->context == NULL) {
        LOGE("vdecoder_open context failed !");
        vdecoder_close(decoder);
        return NULL;
    }
    avcodec_parameters_to_context(decoder->context, parameters);

    decoder->context->framerate = av_guess_frame_rate(decoder->ic, decoder->ic->streams[video_stream_idx], NULL);

    if (decoder->extradata == NULL) {
        decoder->extradata = (unsigned char *)malloc(decoder->context->extradata_size);
    }
    memcpy(decoder->extradata, decoder->context->extradata, decoder->context->extradata_size);
    decoder->extradata_size = decoder->context->extradata_size;
    LOGI("vdecoder_open extradata = %s extradata_size = %d frame_rate = %lf", decoder->extradata, decoder->extradata_size, av_q2d(decoder->context->framerate));

    decoder->in_time_base = decoder->ic->streams[video_stream_idx]->time_base;
    // 视频的解码 time_base 通常为帧率的倒数
    decoder->out_time_base = av_inv_q(decoder->context->framerate);

    // decoder->context = decoder->ic->streams[video_stream_idx]->codec;
    // if (decoder->context == NULL) {
    //     LOGE("vdecoder_open context fail !");
    //     vdecoder_close(decoder);
    //     return NULL;
    // }

    if (avcodec->capabilities & CODEC_CAP_TRUNCATED) {
        decoder->context->flags |= CODEC_FLAG_TRUNCATED;
        LOGI("vdecoder_open support uncomplete frame");
    }

    if (avcodec_open2(decoder->context, avcodec, NULL) < 0) {
        LOGE("vdecoder_open avcodec_open2 fail ");
        vdecoder_close(decoder);
        return NULL;
    }

    decoder->avframe = av_frame_alloc();
    decoder->output_frame = av_frame_alloc();
    if (!decoder->avframe || !decoder->output_frame) {
        LOGE("vdecoder_open avcodec_alloc_frame fail ");
        return NULL;
    }

    decoder->sws_ctx = NULL;

    decoder->output_buffer = NULL;

    decoder->avpkt.data = NULL;
    decoder->avpkt.size = 0;

    decoder->output_width = 0;
    decoder->output_height = 0;

    decoder->output_buffer = NULL;
    decoder->output_buffer_size = 0;
    decoder->output_fmt = DEFAULT_OUTPUT_FMT;

    decoder->pANativeWindow = NULL;

    LOGE("vdecoder vdecoder_open %d", 10);
    av_init_packet(&decoder->avpkt);

    LOGI("vdecoder_open %s success !", filepath);

    return decoder;
}

int vdecoder_close(vdecoder_t *decoder)
{
    if (decoder == NULL) {
        return -1;
    }

    if (decoder->avframe) {
        av_frame_free(&decoder->avframe);
    }

    if (decoder->output_frame) {
        av_frame_free(&decoder->output_frame);
    }

    if (decoder->sws_ctx) {
        sws_freeContext(decoder->sws_ctx);
    }

    if (decoder->output_buffer) {
        free(decoder->output_buffer);
    }

    if (decoder->extradata) {
        free(decoder->extradata);
    }

    if (decoder->ic) {
        avformat_close_input(&decoder->ic);
    }

    av_free_packet(&decoder->avpkt);

    free(decoder);

    LOGI("vdecoder_close success ");

    return 0;
}

int vdecoder_set_output_format(vdecoder_t *decoder, int fmt)
{
    if (decoder == NULL) {
        LOGE("vdecoder_set_output_format failed, decoder ptr is null.");
        return -1;
    }
    decoder->output_fmt = fmt;
    LOGI("vdecoder_set_output_format : %d", fmt);
    return 0;
}

int vdecoder_decode_get_width(vdecoder_t *decoder)
{
    if (decoder == NULL || decoder->context == NULL) {
        LOGE("vdecoder_decode_get_width failed !");
        return -1;
    }
    return (decoder->output_width > 0) ? decoder->output_width : decoder->context->width;
}

int vdecoder_decode_get_height(vdecoder_t *decoder)
{
    if (decoder == NULL || decoder->context == NULL) {
        LOGE("vdecoder_decode_get_height failed !");
        return -1;
    }
    return (decoder->output_height > 0) ? decoder->output_height : decoder->context->height;
}

vdecode_result_t vdecoder_decode_buffer(vdecoder_t *decoder, unsigned char *dst, unsigned char *input, int size, int64_t pts)
{
    LOGI("vdecoder vdecoder_decode_buffer time = %lld", pts);
    vdecode_result_t result;
    result.success = 0;
    result.eof = 0;
    result.got_frame = 0;
    result.img_buffer = NULL;
    result.img_size = 0;

    if (decoder == NULL) {
        return result;
    }

    decoder->avpkt.data = input;
    decoder->avpkt.size = size;
    decoder->avpkt.pts = pts;

    int64_t rescale_pts = av_rescale_q(decoder->avpkt.pts, decoder->in_time_base, decoder->out_time_base);

    LOGI("vdecoder avcodec_send_packet time = %lld", pts);
    int ret = avcodec_send_packet(decoder->context, input == NULL ? NULL : &decoder->avpkt);
    if (ret != 0) {
        LOGE("vdecoder_decode_buffer avcodec_send_packet failed ! errorCode = %d", ret);
        return result;
    }

    while(ret >= 0) {
        ret = avcodec_receive_frame(decoder->context, decoder->avframe);

        LOGI("vdecoder avcodec_receive_frame")
        if (ret == AVERROR(EAGAIN)) {
            LOGI("vdecoder_decode_buffer output is not available right now !");
            break;
        } else if (ret == AVERROR_EOF) {
            LOGI("vdecoder_decode_buffer output reached eof !");
            result.success = 1;
            result.pts = 0;
            result.eof = 1;
            return result;
        } else if (ret < 0) {
            LOGE("vdecoder_decode_buffer error during decoding !");
            return result;
        }

        LOGI("vdecoder width = %d height = %d, pts = %lld", vdecoder_decode_get_width(decoder), vdecoder_decode_get_height(decoder), decoder->avframe->pts);
        if (vdecoder_sws_convert(decoder, vdecoder_decode_get_width(decoder), vdecoder_decode_get_height(decoder)) < 0) {
            return result;
        }
    }

    // int bytes_decoded = -1;
    // while (decoder->avpkt.size > 0) {
    //     bytes_decoded = avcodec_decode_video2(decoder->context, decoder->avframe, &result.got_frame, &decoder->avpkt);
    //     if (bytes_decoded < 0) {
    //         LOGE("vdecoder_decode avcodec_decode_video2 fail, size = %d, error = %d !", decoder->avpkt.size, bytes_decoded);
    //         return result;
    //     }
    //     if (decoder->avpkt.data) {
    //         decoder->avpkt.size -= bytes_decoded;
    //         decoder->avpkt.data += bytes_decoded;
    //     }
    //     LOGV("vdecoder_decode avcodec_decode_video2 success, bytes_decoded = %d!", bytes_decoded);
    // }

    // if (!result.got_frame) {
    //     return result;
    // }

    memcpy(dst, decoder->output_buffer, decoder->output_buffer_size);

    result.img_width  = vdecoder_decode_get_width(decoder);
    result.img_height = vdecoder_decode_get_height(decoder);
    result.img_buffer = decoder->output_buffer;
    result.img_size = decoder->output_buffer_size;
    result.pts = rescale_pts;
    result.success = 1;

    return result;
}

int vdecoder_decode_buffer_to_surface(vdecoder_t *decoder, unsigned char *input, int size, int output_width, int output_height) {
    if (decoder == NULL) {
        LOGE("vdecoder decoder == NULL !");
        return 0;
    }
    decoder->avpkt.data = input;
    decoder->avpkt.size = size;
    LOGI("vdecoder decode_buffer_to_surface %d %d", decoder->avpkt.data == NULL, decoder->avpkt.size == NULL);
    int result = -1;
    while (decoder->avpkt.size > 0) {
        result = avcodec_send_packet(decoder->context, &decoder->avpkt);
        LOGI("vdecoder avcodec_send_packet %d", result);
        if (result < 0) {
            LOGE("vdecoder_decode avcodec_send_packet failed !");
            return 0;
        }
        while (result >= 0) {
            result = avcodec_receive_frame(decoder->context, decoder->avframe);
            LOGI("vdecoder avcodec_receive_frame %d", result);
            if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
                LOGE("vdecoder_decode avcodec_receive_frame EAGAIN or EOF!\n");
                return 0;
            } else if (result < 0) {
                LOGE("vdecoder_decode avcodec_receive_frame Error during decoding!\n");
                return 0;
            }

            LOGI("vdecoder ANativeWindow_setBuffersGeometry %d", decoder->pANativeWindow == NULL);
            ANativeWindow_setBuffersGeometry(decoder->pANativeWindow, decoder->context->width, decoder->context->height, WINDOW_FORMAT_RGB_565);
            ANativeWindow_lock(decoder->pANativeWindow, &decoder->nativeWindow_buffer, NULL);

            if (vdecoder_sws_convert(decoder, output_width, output_height) < 0) {
                return 0;
            }
            LOGI("vdecoder --- first %d", decoder->nativeWindow_buffer.bits == NULL);
            uint8_t *dst = (uint8_t *) decoder->nativeWindow_buffer.bits;
            int destStride = decoder->nativeWindow_buffer.stride * 2;
            LOGI("vdecoder --- second %d", decoder->output_frame == NULL);
            uint8_t *src = decoder->output_frame->data[0];
            int srcStride = decoder->output_frame->linesize[0];
            LOGI("vdecoder --- third %d %d %d %d", decoder->context == NULL, decoder->context->height, dst == NULL, src == NULL);
            for (int i = 0; i < decoder->context->height; i++) {
                memcpy(dst + i * destStride, src + i * srcStride, srcStride);
            }
            LOGI("vdecoder --- ANativeWindow_unlockAndPost +");
            ANativeWindow_unlockAndPost(decoder->pANativeWindow);
            LOGI("vdecoder --- ANativeWindow_unlockAndPost -");
        }
        av_packet_unref(&decoder->avpkt);
        LOGI("vdecoder av_packet_unref");
    }
    return 1;
}

static int vdecoder_sws_convert(vdecoder_t * decoder, int output_width, int output_height)
{
    if (decoder->output_width != output_width || decoder->output_height != output_height) {
        if (decoder->sws_ctx) {
            sws_freeContext(decoder->sws_ctx);
            decoder->sws_ctx = NULL;
        }
        if (decoder->output_buffer) {
            free(decoder->output_buffer);
            decoder->output_buffer = NULL;
        }
        decoder->output_width = output_width;
        decoder->output_height = output_height;
    }

    output_width = vdecoder_decode_get_width(decoder);
    output_height = vdecoder_decode_get_height(decoder);

    if (decoder->output_buffer == NULL) {
        int picsize = avpicture_get_size(decoder->output_fmt, output_width, output_height);
        if (picsize < 0) {
            LOGE("vdecoder_sws_convert failed to calc output img size !");
            return -1;
        }
        decoder->output_buffer_size = picsize;
        decoder->output_buffer = (unsigned char *) malloc(decoder->output_buffer_size * sizeof(char));
        if (decoder->output_buffer == NULL) {
            LOGE("vdecoder_convert_yuv_to_rgb malloc output_buffer failed !");
            return -1;
        }
        LOGI("vdecoder_sws_convert malloc output_buffer success, %d x %d, size = %d",
             output_width, output_height, decoder->output_buffer_size);
    }

    if (decoder->sws_ctx == NULL) {
        decoder->sws_ctx = sws_getContext(decoder->context->width, decoder->context->height,
                                          decoder->context->pix_fmt, output_width, output_height,
                                          decoder->output_fmt, /*SWS_BICUBIC*/SWS_BILINEAR, NULL, NULL, NULL);
        LOGI("vdecoder_sws_convert sws_context created success !");
    }

    /*
     * assign the output buffer to avframe buffer
     */
    av_image_fill_arrays(decoder->output_frame->data, decoder->output_frame->linesize, decoder->output_buffer,
                         decoder->output_fmt, output_width, output_height, 1);
    // avpicture_fill((AVPicture *) decoder->output_frame,  decoder->output_buffer, decoder->output_fmt,
    //    output_width, output_height);

    LOGI("outframe_data : %p output_buffer : %p", decoder->output_frame->data, decoder->output_buffer);

    /*
     * do convert the image buffer
     */
    sws_scale(decoder->sws_ctx, (const uint8_t* const*) decoder->avframe->data, decoder->avframe->linesize, 0,
              decoder->context->height, decoder->output_frame->data, decoder->output_frame->linesize);

    LOGI("vdecoder_sws_convert convert success !");

    return 0;
}
