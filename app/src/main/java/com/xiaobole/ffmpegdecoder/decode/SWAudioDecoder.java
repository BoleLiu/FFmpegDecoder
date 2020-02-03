package com.xiaobole.ffmpegdecoder.decode;

import android.content.Context;
import android.content.res.AssetManager;
import android.media.MediaFormat;
import android.util.Log;

import java.nio.ByteBuffer;

public class SWAudioDecoder {
    private static final String TAG = "SWAudioDecoder";

    private static final int BUFFER_SIZE = 1024 * 1024 * 2;

    static {
        System.loadLibrary("ffmpeg-decode");
    }

    private native boolean nativeInit(String filePath);
    private native boolean nativeInitAsset(AssetManager assetManager, String fileName);
    private native boolean nativeRelease();
    private native boolean nativeDecode(ByteBuffer dst, byte[] src, int size, long pts);
    private native int nativeGetSampleRate();
    private native int nativeGetChannelCount();

    private Context mContext;
    private ByteBuffer mDecodedBuffer;

    private OnDecodeStateChangedListener mOnDecodeStateChangedListener;

    public interface OnDecodeStateChangedListener {
        void onDecodeFormatChanged(MediaFormat format);
        void onDecodeFrameAvailable(ByteBuffer buffer, int size, long pts, boolean eof);
    }

    public SWAudioDecoder(Context context) {
        mContext = context;
    }

    /**
     * Access by native code
     */
    private long mAudioDecoderId;

    private void onAudioDecodeFormatChanged() {
        MediaFormat format = createMediaFormat();
        if (mOnDecodeStateChangedListener != null) {
            mOnDecodeStateChangedListener.onDecodeFormatChanged(format);
        }
    }

    private void onAudioFrameDecoded(int size, long timestampUs, boolean eof) {
        Log.i(TAG, "size = " + size + " pts = " + timestampUs + " eof = " + eof);
        if (mOnDecodeStateChangedListener != null) {
            mOnDecodeStateChangedListener.onDecodeFrameAvailable(mDecodedBuffer, size, timestampUs, eof);
        }
    }

    public boolean init(String filePath) {
        return nativeInit(filePath);
    }

    public boolean initAsset(String fileName) {
        return nativeInitAsset(mContext.getAssets(), fileName);
    }

    public boolean release() {
        return nativeRelease();
    }

    public void decode(ByteBuffer src, int size, long pts) {
        if (mDecodedBuffer == null) {
            mDecodedBuffer = ByteBuffer.allocateDirect(BUFFER_SIZE);
        }
        mDecodedBuffer.rewind();

        // end of file
        byte[] data = null;
        if (src != null) {
            data = new byte[src.remaining()];
            src.get(data, 0, data.length);
        }

        nativeDecode(mDecodedBuffer, data, size, pts);
    }

    public int getSampleRate() {
        return nativeGetSampleRate();
    }

    public int getChannelCount() {
        return nativeGetChannelCount();
    }

    public void setOnDecodeStateChangedListener(OnDecodeStateChangedListener listener) {
        mOnDecodeStateChangedListener = listener;
    }

    private MediaFormat createMediaFormat() {
        return MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, getSampleRate(), getChannelCount());
    }
}
