package com.xiaobole.ffmpegdecoder.activity;

import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.xiaobole.ffmpegdecoder.decode.FFDecoder;
import com.xiaobole.ffmpegdecoder.R;

import java.io.File;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    private SurfaceView mPlayerView;

    // Used to load the 'native-lib' library on application startup.
//    static {
//        System.loadLibrary("ffmpeg-decode");
//    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
//        TextView tv = findViewById(R.id.sample_text);
//        tv.setText(stringFromJNI());
        mPlayerView = findViewById(R.id.surfaceView);
        mPlayerView.getHolder().setFormat(PixelFormat.RGBA_8888);
        mPlayerView.getHolder().addCallback(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i("liu", "surfaceCreated");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void onClickStart(View v) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                FFDecoder.start(Environment.getExternalStorageDirectory() + File.separator + "DCIM/Camera/test.mp4", mPlayerView.getHolder().getSurface());
            }
        }).start();
    }

    public void onClickDecodeAudio(View v) {
        Intent intent = new Intent(MainActivity.this, AudioDecodeActivity.class);
        startActivity(intent);
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
//    public native String stringFromJNI();
}
