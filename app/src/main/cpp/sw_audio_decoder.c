//
// Created by Jingbo Liu on 2019-08-21.
//

#include <stdio.h>
#include <stdlib.h>
#include <adecoder.h>
#include <jni.h>
#include <logger.h>
#include <android/asset_manager_jni.h>
#include <unistd.h>

#ifndef _Included_com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder
#define _Included_com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder
#ifdef __cplusplus
extern "C" {
#endif

static jmethodID gDecodeCallbackMethodId;
static jmethodID gDecodeFormatCallbackMethodId;
static jboolean gDecodeFormatCallbackTriggered;

static jboolean set_audio_decoder(JNIEnv *env, jobject thiz, adecoder_t * decoder)
{
    jclass SWVideoDecoder = (*env)->GetObjectClass(env, thiz);
    jfieldID mVideoDecoderId = (*env)->GetFieldID(env, SWVideoDecoder, "mAudioDecoderId", "J");
    if (mVideoDecoderId == NULL) {
        return JNI_FALSE;
    }
    (*env)->SetLongField(env, thiz, mVideoDecoderId, (jlong) decoder);
    LOGE("adecoder set_audio_decoder success");
    return JNI_TRUE;
}

static adecoder_t * get_audio_decoder(JNIEnv *env, jobject thiz)
{
    jclass SWVideoDecoder = (*env)->GetObjectClass(env, thiz);
    jfieldID mVideoDecoderId = (*env)->GetFieldID(env, SWVideoDecoder, "mAudioDecoderId", "J");
    if (mVideoDecoderId == NULL) {
        return NULL;
    }
    LOGE("vdecoder get_video_decoder success");
    return (adecoder_t *) (*env)->GetLongField(env, thiz, mVideoDecoderId);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    JNIEnv *env = NULL;
    if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("JNI_OnLoad fail!");
        return -1;
    }
    adecoder_init();
    return JNI_VERSION_1_4;
}

/*
 * Class:     com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder
 * Method:    nativeInit
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder_nativeInit
  (JNIEnv * env, jobject thiz, jstring file_path) {
    char *path = (char *) (*env)->GetStringUTFChars(env, file_path, 0);
    LOGI("adecoder filepath = %s", path);

    adecoder_t *decoder = adecoder_open_file(path);
    if (decoder == NULL) {
        (*env)->ReleaseStringUTFChars(env, file_path, path);
        return JNI_FALSE;
    }
    (*env)->ReleaseStringUTFChars(env, file_path, path);

    jclass jClass = (*env)->GetObjectClass(env, thiz);
    gDecodeCallbackMethodId = (*env)->GetMethodID(env, jClass, "onAudioFrameDecoded", "(IJZ)V");
    gDecodeFormatCallbackMethodId = (*env)->GetMethodID(env, jClass, "onAudioDecodeFormatChanged", "()V");
    gDecodeFormatCallbackTriggered = JNI_FALSE;

    return set_audio_decoder(env, thiz, decoder);
}

JNIEXPORT jboolean JNICALL Java_com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder_nativeInitAsset
        (JNIEnv * env, jobject thiz, jobject assetManager, jstring assetName) {

    AAssetManager* manager = AAssetManager_fromJava(env, assetManager);
    const char *szAssetName = (*env)->GetStringUTFChars(env, assetName, NULL);
    AAsset* asset = AAssetManager_open(manager, szAssetName, AASSET_MODE_RANDOM);
    (*env)->ReleaseStringUTFChars(env, assetName, szAssetName);
    off_t offset, length;
    int fd = AAsset_openFileDescriptor(asset, &offset, &length);
    AAsset_close(asset);

    adecoder_t *decoder = adecoder_open_asset(fd, offset, length);
    if (decoder == NULL) {
        close(fd);
        return JNI_FALSE;
    }
    close(fd);

    jclass jClass = (*env)->GetObjectClass(env, thiz);
    gDecodeCallbackMethodId = (*env)->GetMethodID(env, jClass, "onAudioFrameDecoded", "(IJZ)V");
    gDecodeFormatCallbackMethodId = (*env)->GetMethodID(env, jClass, "onAudioDecodeFormatChanged", "()V");
    gDecodeFormatCallbackTriggered = JNI_FALSE;

    return set_audio_decoder(env, thiz, decoder);
}

/*
 * Class:     com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder
 * Method:    nativeRelease
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder_nativeRelease
  (JNIEnv * env, jobject thiz) {
    adecoder_t *decoder = get_audio_decoder(env, thiz);
    if (decoder == NULL) {
        return JNI_FALSE;
    }
    adecoder_close(decoder);
    return JNI_TRUE;
}

/*
 * Class:     com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder
 * Method:    nativeDecode
 * Signature: ([B)[B
 */
JNIEXPORT jboolean JNICALL Java_com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder_nativeDecode
  (JNIEnv *env, jobject thiz, jobject dst, jbyteArray src, jint size, jlong pts) {
    adecoder_t *decoder = get_audio_decoder(env, thiz);
    if (decoder == NULL) {
        LOGE("adecoder decoder == NULL");
        return JNI_FALSE;
    }

    unsigned char * p_dst_buffer = (*env)->GetDirectBufferAddress(env, dst);
    if (p_dst_buffer == NULL) {
        LOGE("adecoder dstBuffer == NULL");
        return JNI_FALSE;
    }

    unsigned char * p_src_buffer = NULL;
    if (src != NULL) {
        p_src_buffer = (unsigned char*) (*env)->GetByteArrayElements(env, src, NULL);
        if (p_src_buffer == NULL) {
            return JNI_FALSE;
        }
    }

    adecode_result_t result;
    LOGI("nativeDecode adecoder_decode_buffer %d", size);
    result = adecoder_decode_buffer(decoder, p_dst_buffer, p_src_buffer, size, pts);

    if (!result.success) {
        return JNI_FALSE;
    }

    if (!gDecodeFormatCallbackTriggered && decoder->extradata_size > 0) {
        gDecodeFormatCallbackTriggered = JNI_TRUE;
        (*env)->CallVoidMethod(env, thiz, gDecodeFormatCallbackMethodId);
    }

    (*env)->CallVoidMethod(env, thiz, gDecodeCallbackMethodId, result.decoded_size, result.pts, result.eof == 1);

    return JNI_TRUE;
}

/*
 * Class:     com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder
 * Method:    nativeGetSampleRate
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder_nativeGetSampleRate
  (JNIEnv *env, jobject thiz) {
    adecoder_t *decoder = get_audio_decoder(env, thiz);
    if (decoder == NULL) {
        return 0;
    }
    return adecoder_get_samplerate(decoder);
}

/*
 * Class:     com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder
 * Method:    nativeGetChannelCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_xiaobole_ffmpegdecoder_decode_SWAudioDecoder_nativeGetChannelCount
  (JNIEnv *env, jobject thiz) {
    adecoder_t *decoder = get_audio_decoder(env, thiz);
    if (decoder == NULL) {
        return 0;
    }
    return adecoder_get_channel_count(decoder);
}

#ifdef __cplusplus
}
#endif
#endif
