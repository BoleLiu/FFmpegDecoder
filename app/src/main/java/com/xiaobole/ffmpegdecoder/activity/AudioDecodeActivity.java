package com.xiaobole.ffmpegdecoder.activity;

import android.app.Activity;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import com.xiaobole.ffmpegdecoder.R;
import com.xiaobole.ffmpegdecoder.decode.SWAudioDecoder;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class AudioDecodeActivity extends Activity implements SWAudioDecoder.OnDecodeStateChangedListener {
    private static final String TAG = "AudioDecodeActivity";

    private MediaExtractor mMediaExtractor;
    private SWAudioDecoder mAudioDecoder;

    private ByteBuffer mInputBuffer;
    private String mFilePath;
    private int mAudioIndex;
    private boolean mIsStop;

    FileOutputStream fos = null;
    DataOutputStream dos = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_decode);

        mFilePath = Environment.getExternalStorageDirectory() + File.separator + "QNShortVideo/test.mp4";
//        AssetFileDescriptor afd = null;
//        try {
//            afd = getAssets().openFd("test.mp3");
//        } catch (IOException e) {
//            e.printStackTrace();
//        }
        mInputBuffer = ByteBuffer.allocateDirect(1024 * 1024 * 2);
        mMediaExtractor = new MediaExtractor();
        try {
            mMediaExtractor.setDataSource(mFilePath);
//            if (afd != null) {
//                mMediaExtractor.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getDeclaredLength());
//            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        for (int i = 0; i < mMediaExtractor.getTrackCount(); i++) {
            String mimeType = mMediaExtractor.getTrackFormat(i).getString(MediaFormat.KEY_MIME);
            if (mimeType.startsWith("audio")) {
                mAudioIndex = i;
                mMediaExtractor.selectTrack(mAudioIndex);
            }
        }
        Log.i("liu", "adecoder audioIndex = " + mAudioIndex);

        mAudioDecoder = new SWAudioDecoder(this);
        mAudioDecoder.setOnDecodeStateChangedListener(this);
        mAudioDecoder.init(mFilePath);
//        mAudioDecoder.initAsset("test.mp3");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

    }

    public void onClickStart(View v) {
        try {
            fos = new FileOutputStream(Environment.getExternalStorageDirectory() + File.separator + "decodeAudio.pcm");
            dos = new DataOutputStream(fos);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                while (!mIsStop) {
                    int sampleSize = mMediaExtractor.readSampleData(mInputBuffer, 0);
                    if (sampleSize >= 0) {
                        Log.i("liu", "adecoder sample size = " + sampleSize);
                        mAudioDecoder.decode(mInputBuffer, sampleSize, mMediaExtractor.getSampleTime());
                        mMediaExtractor.advance();
                    } else {
                        Log.i("liu", "adecoder eof !");
                        mAudioDecoder.decode(null, 0, 0);
//                        mAudioDecoder.release();
//                        mAudioDecoder = null;
//
//                        mMediaExtractor.release();
//                        mMediaExtractor = null;
//                        try {
//                            fos.close();
//                            dos.close();
//                        } catch (IOException e) {
//                            e.printStackTrace();
//                        }
                        break;
                    }
                }
            }
        }).start();
    }

    public void onClickStop(View v) {
        mIsStop = true;
        mAudioDecoder.release();
        try {
            fos.close();
            dos.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onDecodeFormatChanged(MediaFormat format) {
        Log.i(TAG, "MediaFormat changed : " + format);
    }

    @Override
    public void onDecodeFrameAvailable(ByteBuffer buffer, int size, long pts, boolean eof) {
        if (eof) {
            mAudioDecoder.release();
            mAudioDecoder = null;

            mMediaExtractor.release();
            mMediaExtractor = null;
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(AudioDecodeActivity.this, "eof", Toast.LENGTH_SHORT).show();
                }
            });
            try {
                fos.close();
                dos.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else {
            try {
                byte[] data = new byte[size];
                buffer.get(data, 0, data.length);
                dos.write(data);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}
