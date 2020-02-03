#include <jni.h>
#include <string>

extern "C"
{

#include <libavutil/frame.h>
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif

#include "android/native_window_jni.h"
#include "../ffmpeg/include/libavcodec/avcodec.h"
#include "../ffmpeg/include/libavformat/avformat.h"
#include "../ffmpeg/include/libavutil/imgutils.h"
#include "../ffmpeg/include/libswscale/swscale.h"

JNIEXPORT jstring JNICALL
Java_com_xiaobole_ffmpegdecoder_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT void JNICALL
Java_com_xiaobole_ffmpegdecoder_decode_FFDecoder_start(JNIEnv *env, jclass obj, jstring url_,
                                                jobject surface) {
    const char *url = env->GetStringUTFChars(url_, 0);
    AVCodec *pCodec;
    AVCodecContext *pCodecCtx;
    AVPacket *avPacket;
    AVFrame *avFrame;
    AVFrame *rgbFrame;

    // register
    av_register_all();

    AVFormatContext *avFormatContext = avformat_alloc_context();
    // 这里第一个参数需要看下，没看懂指针这儿
    int ret = avformat_open_input(&avFormatContext, url, NULL, NULL);
    if (ret < 0) {
        LOGE("Could't open input stream! errorCode = %d\n", ret);
        return;
    }

    ret = avformat_find_stream_info(avFormatContext, NULL);
    if (ret < 0) {
        LOGE("Could't find stream info! errorCode = %d\n", ret);
        return;
    }

    // find video index
    int video_index = -1;
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        }
    }
    if (video_index == -1) {
        LOGE("Couldn't find a video stream!\n");
        return;
    }

    // find decoder
    pCodec = avcodec_find_decoder(avFormatContext->streams[video_index]->codecpar->codec_id);
    if (pCodec == NULL) {
        LOGE("Couldn't find codec!\n");
        return;
    }

    // alloc codec context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        LOGE("avcodec_alloc_context3 failed!\n");
        return;
    }
    avcodec_parameters_to_context(pCodecCtx, avFormatContext->streams[video_index]->codecpar);

    pCodecCtx->framerate = av_guess_frame_rate(avFormatContext, avFormatContext->streams[video_index], NULL);

//    LOGI("liu extradata = %x, size = %d pix_fmt = %d framerate = %lf", pCodecCtx->extradata, pCodecCtx->extradata_size, pCodecCtx->pix_fmt, pCodecCtx->framerate);

//    // find decoder
//    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
//    if (pCodec == NULL) {
//        LOGE("Couldn't find codec!\n");
//        return;
//    }

    LOGI("liu extradata = %x, size = %d pix_fmt = %d framerate = %d %d stream_time_base = %lf",
            pCodecCtx->extradata,
            pCodecCtx->extradata_size,
            pCodecCtx->pix_fmt,
            pCodecCtx->framerate.num,
            pCodecCtx->framerate.den,
            av_q2d(avFormatContext->streams[video_index]->time_base));

    // open codec
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (ret < 0) {
        LOGE("couldn't open codec! errorCode = %d\n", ret);
        return;
    }

    avPacket = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(avPacket);

    avFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height, 1) *
            sizeof(uint8_t));
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, out_buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx->width, pCodecCtx->height, 1);

    ANativeWindow *pANativeWindow = ANativeWindow_fromSurface(env, surface);
    if (pANativeWindow == NULL) {
        LOGE("couldn't get native window!\n");
        return;
    }
    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA,
                                            SWS_BICUBIC, NULL, NULL, NULL);

    // 视频缓冲区
    ANativeWindow_Buffer nativeWindow_buffer;
    int frameCount;
    while (av_read_frame(avFormatContext, avPacket) >= 0) {
        if (avPacket->stream_index == video_index) {
            LOGI("avPacket pts = %lld, dts = %lld", avPacket->pts, avPacket->dts);
            int ret = avcodec_send_packet(pCodecCtx, avPacket);
            if (ret < 0) {
                LOGE("Send video packet failed!\n");
                continue;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, avFrame);
                // -11 means there is no data to receive, need more packet data to decode
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    LOGE("EAGAIN or EOF!\n");
                    break;
                } else if (ret < 0) {
                    LOGE("Error during decoding!\n");
                    break;
                }
                LOGI("liu avFrame.pts = %lld", av_frame_get_best_effort_timestamp(avFrame));
                ANativeWindow_setBuffersGeometry(pANativeWindow, pCodecCtx->width,
                                                 pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
                ANativeWindow_lock(pANativeWindow, &nativeWindow_buffer, NULL);
                // 转换 rgb 格式
                sws_scale(swsContext, (const uint8_t *const *) avFrame->data, avFrame->linesize, 0,
                          avFrame->height, rgbFrame->data, rgbFrame->linesize);
                uint8_t *dst = (uint8_t *) nativeWindow_buffer.bits;
                LOGI("liu nativeWindow_buffer.bits = %d", nativeWindow_buffer.bits == NULL);
                int destStride = nativeWindow_buffer.stride * 4;
                uint8_t *src = rgbFrame->data[0];
                int srcStride = rgbFrame->linesize[0];
                for (int i = 0; i < pCodecCtx->height; i++) {
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(pANativeWindow);
            }
        }
        av_packet_unref(avPacket);
    }

    ANativeWindow_release(pANativeWindow);
    av_frame_free(&avFrame);
    av_frame_free(&rgbFrame);
    avcodec_close(pCodecCtx);
    avformat_free_context(avFormatContext);

    env->ReleaseStringUTFChars(url_, url);
}
}