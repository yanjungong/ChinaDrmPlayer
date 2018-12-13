package com.chinadrm.demo;

import android.content.Intent;
import android.media.MediaPlayer;
import android.os.Environment;
import android.os.SystemClock;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.TextView;

import java.io.IOException;

import static android.webkit.ConsoleMessage.MessageLevel.LOG;

public class MainActivity extends AppCompatActivity implements
        SurfaceHolder.Callback,
        MediaPlayer.OnInfoListener,
        MediaPlayer.OnPreparedListener,
        MediaPlayer.OnCompletionListener,
        MediaPlayer.OnErrorListener,
        MediaPlayer.OnVideoSizeChangedListener {

    private static final String TAG = "ChinaDrmParser";
    public static final String IntentTag_Url = "URL";
    public static final String IntentTag_Description = "Description";

    private String mUrl ;
    private String mDescription;

    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;
    private TextView mHubView;
    private MediaPlayer mPlayer;

    private long mStartTime;
    private long mPreparedTime;
    private long mRenderStartTime;
    private boolean mRenderStartReported;   //有些设备onInfo(3)可能上报多次，只记录第一次上报


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        String sdcardPath =  Environment.getExternalStorageDirectory().getPath();
        ChinaDrmDecrypt drm  = new ChinaDrmDecrypt();

        drm.setSdcardPath(sdcardPath);
        String outputFile = drm.decrypt("", "", "");

        Intent intent = getIntent();
        mUrl = intent.getStringExtra(IntentTag_Url);
        mUrl =  outputFile;
        mDescription = intent.getStringExtra(IntentTag_Description);
        setTitle(mDescription);

        mSurfaceView = findViewById(R.id.video_view);
        mSurfaceView.setSecure(true);
        mSurfaceHolder = mSurfaceView.getHolder();
        mSurfaceHolder.addCallback(this);

        mHubView = findViewById(R.id.hud_view);
    }

    public void startPlay(String url) {
        Log.d(TAG, "startPlay: url=" + url);
        if (mPlayer != null) {
            mPlayer.release();
            mPlayer = null;
        }
        mHubView.setTextColor(0xFFFF0000);
        mHubView.setText("Start Playing......");
        mStartTime = SystemClock.elapsedRealtime();
        try {
            mRenderStartReported = false;
            mPlayer = new MediaPlayer();
            mPlayer.setOnPreparedListener(this);
            mPlayer.setOnCompletionListener(this);
            mPlayer.setOnInfoListener(this);
            mPlayer.setOnVideoSizeChangedListener(this);
            mPlayer.setOnErrorListener(this);
            mPlayer.setDataSource(url);
            mPlayer.prepareAsync();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalStateException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    public void stopPlay() {
        Log.d(TAG, "stopPlayer");
        if (null != mPlayer) {
            mPlayer.stop();
            mPlayer.release();
            mPlayer = null;
        }
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        MainActivity.this.finish();
        Log.d(TAG, "onCompletion()");
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra) {
        Log.e(TAG, "onError: what=" + what + ", extra=" + extra);
        return false;
    }

    @Override
    public boolean onInfo(MediaPlayer mp, int what, int extra) {
        if (!mRenderStartReported && what == MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START) {
            Log.d(TAG, "First frame is render");
            mRenderStartReported = true;
            mRenderStartTime = SystemClock.elapsedRealtime() - mStartTime;
            String hubText = "PreparedTime=" + mPreparedTime + "(ms), RenderStartTime=" + mRenderStartTime + "(ms)";
            mHubView.setTextColor(0xFF00FF00);
            mHubView.setText(hubText);
        }
        return false;
    }

    @Override
    public void onPrepared(MediaPlayer mp) {
        mPreparedTime = SystemClock.elapsedRealtime() - mStartTime;
        Log.d(TAG, "onPrepared, time=" + mPreparedTime + ", mp=" + mp);
        if (mp != null) {
            if (mSurfaceHolder != null && mSurfaceHolder.getSurface().isValid()) {
                mp.setDisplay(mSurfaceHolder);
            }
            mp.start();
        }
    }

    @Override
    public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
        Log.d(TAG, "onVideoSizeChanged, width=" + width + " height=" + height);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated");
        mSurfaceHolder = holder;
        if (mPlayer != null) {
            mPlayer.setDisplay(mSurfaceHolder);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "surfaceChanged, width=" + width + " height=" + height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed");
        mSurfaceHolder = null;
        if (mPlayer != null) {
            mPlayer.setSurface(null);
        }
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause()");
        super.onPause();
        stopPlay();
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "onStop()");
        super.onStop();
    }

    @Override
    protected void onStart() {
        Log.d(TAG, "onStart()");
        super.onStart();
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume()");
        startPlay(mUrl);
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy()");
        super.onDestroy();
    }

}
