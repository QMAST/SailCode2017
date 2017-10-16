package com.mast.boatcontrol;

import android.os.Build;
import android.os.Handler;
import android.speech.tts.TextToSpeech;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.SeekBar;
import android.widget.TextView;

import com.google.android.flexbox.FlexboxLayout;
import com.mast.boatcontrol.models.XMessage;

public class RemoteActivity extends AppCompatActivity {

    private static int TRANS_INTERVAL_MILLIS = 250;
    int iCurrentSails = 100;
    TextView tvSails;

    FlexboxLayout fbControls;

    private Handler transmissionHandler = new Handler();
    private Runnable transmissionRunnable;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (Build.VERSION.SDK_INT >= 21) {
            //getWindow().setStatusBarColor(getResources().getColor(R.color.colorNavRoute));
        }
        setContentView(R.layout.activity_remote);

        fbControls = (FlexboxLayout) findViewById(R.id.fbControls);

        transmissionRunnable = new Runnable() {
            @Override
            public void run() {
                ((MyApplication) getApplication()).sendTransmission(new XMessage(99,iCurrentSails+""));
                Log.e("TRANSMISSION", (new XMessage(99,iCurrentSails+"").toString()));
                transmissionHandler.postDelayed(transmissionRunnable, TRANS_INTERVAL_MILLIS);
            }
        };

        tvSails = (TextView) findViewById(R.id.tvSails);

        SeekBar sbSails = (SeekBar) findViewById(R.id.sbSails);
        sbSails.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                tvSails.setText(progress + "% wound");
                if(Math.abs(progress - iCurrentSails) >= 3){
                    iCurrentSails = progress;
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

    }

    @Override
    protected void onResume() {
        transmissionHandler.postDelayed(transmissionRunnable, TRANS_INTERVAL_MILLIS);
        super.onResume();
    }

    @Override
    protected void onPause() {
        transmissionHandler.removeCallbacks(transmissionRunnable);
        super.onPause();
    }
}
