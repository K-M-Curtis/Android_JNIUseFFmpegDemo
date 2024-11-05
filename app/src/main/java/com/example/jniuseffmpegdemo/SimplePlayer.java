package com.example.jniuseffmpegdemo;

import android.view.Surface;

public class SimplePlayer {
    static {
        System.loadLibrary("native-lib");
    }

    public native void playVideo(String path, Surface surface);
}
