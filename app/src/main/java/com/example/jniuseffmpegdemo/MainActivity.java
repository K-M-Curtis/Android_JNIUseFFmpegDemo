package com.example.jniuseffmpegdemo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.example.jniuseffmpegdemo.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'jniuseffmpegdemo' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private static final String TAG = "======MainActivity";
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private TextView textView;
    private Button playButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        textView = findViewById(R.id.sample_text);
        textView.setText(stringFromJNI());

        surfaceView = findViewById(R.id.surface_view);
        surfaceHolder = surfaceView.getHolder();

        playButton = findViewById(R.id.play_button);
        playButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String video_path = Environment.getExternalStorageDirectory() + "/bili.mp4";
                Log.d(TAG, "onClick: video_path: " + video_path);
                SimplePlayer simplePlayer = new SimplePlayer();
                simplePlayer.playVideo(video_path, surfaceHolder.getSurface());
            }
        });
    }

    /**
     * A native method that is implemented by the 'jniuseffmpegdemo' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}