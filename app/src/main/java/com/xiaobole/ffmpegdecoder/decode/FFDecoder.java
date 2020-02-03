package com.xiaobole.ffmpegdecoder.decode;

import android.view.Surface;

public class FFDecoder {
    static {
        System.loadLibrary("ffmpeg-decode");
    }

    public static native void start(String url, Surface surface);


//    public static native void stop();
//
//    public static native void release();
}
