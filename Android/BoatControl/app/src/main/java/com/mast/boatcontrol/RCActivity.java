package com.mast.boatcontrol;

import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.speech.tts.TextToSpeech;
import android.support.annotation.NonNull;
import android.support.design.widget.BottomNavigationView;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.mast.boatcontrol.models.XMessage;

import java.util.List;
import java.util.Locale;

import static android.app.PendingIntent.FLAG_UPDATE_CURRENT;

public class RCActivity extends AppCompatActivity {

    private TextView tvTransmitterError, tvWind, tvWindAngle, tvHeading, tvLastTrans, tvSpeed;
    TextToSpeech myTTS;
    private int iLastTransmission = 0;
    private long iLastSpeakDeg, iLastSpeakHead;
    private LinearLayout llTransmitterError;
    private MyApplication myApplication;
    OnIncomingTransmissionListener myIncomingTransmissionListener;
    OnUSBStateChangedListener myUSBStateChangedListener;

    private Handler mOfflineHandler = new Handler();
    private Runnable mOfflineRunnable;

    private Handler mLastHandler = new Handler();
    private Runnable mLastRunnable;

    private BottomNavigationView.OnNavigationItemSelectedListener mOnNavigationItemSelectedListener
            = new BottomNavigationView.OnNavigationItemSelectedListener() {

        @Override
        public boolean onNavigationItemSelected(@NonNull MenuItem item) {
            switch (item.getItemId()) {
                case R.id.navigation_home:
                    return true;
                case R.id.navigation_mode:
                    if (myApplication.isConnectedUSB()) {
                        AlertDialog.Builder builder = new AlertDialog.Builder(RCActivity.this);
                        builder.setTitle("Change Mode")
                                .setItems(R.array.modes_array, new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int which) {
                                        myApplication.sendTransmission(new XMessage(21, "" + which));
                                    }
                                });
                        builder.create();
                        builder.show();
                    } else {
                        Toast.makeText(RCActivity.this, "Connect transmitter to continue", Toast.LENGTH_SHORT).show();
                    }
                    return false;
                case R.id.navigation_autosail:
                    startActivity(new Intent(RCActivity.this, SailPlansActivity.class));
                    return false;
            }
            return false;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_rcavtivity);

        myApplication = (MyApplication) getApplication();
        tvWindAngle = (TextView) findViewById(R.id.tvWindDir);
        tvHeading = (TextView) findViewById(R.id.tvHeading);
        tvLastTrans = (TextView) findViewById(R.id.tvLastTrans);
        tvWind = (TextView) findViewById(R.id.tvWind);
        tvSpeed = (TextView) findViewById(R.id.tvSpeed);
        tvTransmitterError = (TextView) findViewById(R.id.tvTransmitterError);
        llTransmitterError = (LinearLayout) findViewById(R.id.llTransmitterError);
        BottomNavigationView navigation = (BottomNavigationView) findViewById(R.id.navigation);
        navigation.setOnNavigationItemSelectedListener(mOnNavigationItemSelectedListener);

        mOfflineRunnable = new Runnable() {
            @Override
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        //tvTransmitterError.setText("Boat Offline");
                        //llTransmitterError.setBackgroundColor(ContextCompat.getColor(RCActivity.this, R.color.colorAccent));
                        //llTransmitterError.setVisibility(View.VISIBLE);
                        myTTS.speak("Boat offline.", TextToSpeech.QUEUE_FLUSH, null);
                    }
                });
            }
        };

        mLastRunnable = new Runnable() {
            @Override
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        iLastTransmission++;
                        tvLastTrans.setText(iLastTransmission + "");
                        mLastHandler.postDelayed(mLastRunnable, 1000);
                    }
                });
            }
        };

        myIncomingTransmissionListener = new OnIncomingTransmissionListener() {
            @Override
            public void onIncomingTransmission(XMessage transmission) {
                final XMessage myTransmission = transmission;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        executeMessage(myTransmission);
                    }
                });
            }
        };
        myUSBStateChangedListener = new OnUSBStateChangedListener() {
            @Override
            public void onUSBStateChanged(boolean connected) {
                if (connected) {
                    tvTransmitterError.setText("Awaiting transmissions");
                    llTransmitterError.setBackgroundColor(ContextCompat.getColor(RCActivity.this, R.color.colorAccent));
                    llTransmitterError.setVisibility(View.VISIBLE);
                    //myRef.removeEventListener(velConnected);
                } else {
                    //myRef.addChildEventListener(celStatistics);
                    //connectedRef.addValueEventListener(velConnected);
                    llTransmitterError.setVisibility(View.VISIBLE);
                    tvTransmitterError.setText("Transmitter not connected");
                    llTransmitterError.setBackgroundColor(ContextCompat.getColor(RCActivity.this, R.color.colorError));
                    if (myApplication.isPermissionNeeded()) {
                        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
                        List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
                        if (!availableDrivers.isEmpty()) {
                            UsbSerialDriver driver = availableDrivers.get(0);
                            UsbDeviceConnection connection = manager.openDevice(driver.getDevice());
                            if (connection == null) {
                                // You probably need to call UsbManager.requestPermission(driver.getDevice(), ..)
                                manager.requestPermission(driver.getDevice(), PendingIntent.getActivity(RCActivity.this, 500, new Intent(RCActivity.this, RCActivity.class), FLAG_UPDATE_CURRENT));
                            }
                        }
                    }
                }
            }
        };
        myTTS = new TextToSpeech(this, new TextToSpeech.OnInitListener() {
            @Override
            public void onInit(int status) {
                if (status != TextToSpeech.ERROR) {
                    myTTS.setLanguage(Locale.UK);
                    mOfflineHandler.postDelayed(mOfflineRunnable, 2000);
                }

            }
        });

    }

    private void executeMessage(XMessage command) {
        switch (command.getData_code()) {
            case 12:
                String[] wind = command.getData().split(" ");
                if (wind.length == 2) {
                    tvWind.setText(wind[0]);
                    tvWindAngle.setText(wind[1]);
                    if (System.currentTimeMillis() - iLastSpeakDeg > 5000) {
                        try {
                            myTTS.speak(Math.round(Double.parseDouble(wind[1])) + " degrees.", TextToSpeech.QUEUE_FLUSH, null);
                            if (Math.round(Double.parseDouble(wind[1])) == 69)
                                myTTS.speak("yeehaw!", TextToSpeech.QUEUE_ADD, null);

                            iLastSpeakDeg = System.currentTimeMillis();
                        } catch (NumberFormatException e) {
                        }
                    }
                    if (System.currentTimeMillis() - iLastSpeakHead > 10000) {
                        try {
                            myTTS.speak(Math.round(Double.parseDouble(wind[0])) + " knots.", TextToSpeech.QUEUE_ADD, null);
                            iLastSpeakHead = System.currentTimeMillis();
                        } catch (NumberFormatException e) {

                        }
                    }
                }
                break;
            case 13:
                tvHeading.setText(command.getData());
                break;
            case 16:
                tvSpeed.setText(command.getData());
                break;
        }
        mOfflineHandler.removeCallbacks(mOfflineRunnable);
        mOfflineHandler.postDelayed(mOfflineRunnable, 2000);
        llTransmitterError.setVisibility(View.GONE);

        iLastTransmission = 0;
        tvLastTrans.setText("0");
        mLastHandler.removeCallbacks(mLastRunnable);
        mLastHandler.postDelayed(mLastRunnable, 1000);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mOfflineHandler.removeCallbacks(mOfflineRunnable);
        mLastHandler.removeCallbacks(mLastRunnable);
        myApplication.removeonIncomingTransmissionListener(myIncomingTransmissionListener);
        myApplication.removeOnUSBStateChangedListener(myUSBStateChangedListener);
    }

    @Override
    protected void onResume() {
        super.onResume();
        myApplication.refreshUSBDevice();
        myApplication.addOnIncomingTransmissionListener(myIncomingTransmissionListener);
        myApplication.addOnUSBStateChangedListener(myUSBStateChangedListener);
    }
}
