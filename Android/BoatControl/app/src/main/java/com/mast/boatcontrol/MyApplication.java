package com.mast.boatcontrol;

import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.util.Log;

import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;
import com.mast.boatcontrol.models.XMessage;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

interface OnIncomingTransmissionListener {
    void onIncomingTransmission(XMessage transmission);
}

interface OnUSBStateChangedListener {
    void onUSBStateChanged(boolean connected);
}

/**
 * Created by mast on 21/05/17.
 */


public class MyApplication extends Application {
    private static String XBEE_AES_KEY = "8F71B07756A0B0901B19ED2EB0EC17F1";

    private static String TAG = "MyApplication.java";
    // Serial port declarations
    private final ExecutorService mExecutor = Executors.newSingleThreadExecutor();
    BroadcastReceiver mUsbReceiver;
    StringBuilder sBuffer = new StringBuilder();
    private List<OnIncomingTransmissionListener> onIncomingTransmissionListeners = new ArrayList<OnIncomingTransmissionListener>();
    private final SerialInputOutputManager.Listener mySerialInputOutputManagerListener =
            new SerialInputOutputManager.Listener() {

                @Override
                public void onRunError(Exception e) {
                    Log.d("Main", "Runner stopped.");
                }

                @Override
                public void onNewData(final byte[] data) {
                    // Processes incoming data from XBee
                    sBuffer.append(new String(data));
                    if (sBuffer.toString().contains(";")) {
                        // if the buffer contains a ; that means a full command has been received
                        sBuffer.append(" "); // append a space so that if the buffer contains a complete command, it will be split into an array with two element. This is necessary as the for loop executes length - 1 times. Not removing a space will cause delay
                        String sCommands[] = sBuffer.toString().split(";");
                        for (int i = 0; i < sCommands.length - 1; i++) {
                            Log.i(TAG, "Received: " + sCommands[i]);
                            XMessage command = new XMessage(sCommands[i]);
                            // Notify listeners
                            for (OnIncomingTransmissionListener listener : onIncomingTransmissionListeners)
                                listener.onIncomingTransmission(command);
                        }
                        sBuffer.setLength(0);
                        sBuffer.append(sCommands[sCommands.length - 1].replace(" ", "")); // put the unexecuted fragments back into the buffer
                    }
                }
            };
    private List<OnUSBStateChangedListener> onUSBStateChangedListeners = new ArrayList<OnUSBStateChangedListener>();
    private boolean connectedUSB = false;
    private boolean permissionNeeded = false;
    private UsbSerialPort sPort = null;
    private SerialInputOutputManager mSerialIoManager;
    DatabaseReference connectedRef;

    @Override
    public void onCreate() {
        super.onCreate();
        // Set up USB transmitter detach listeners
        mUsbReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                    UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                    if (device != null) {
                        connectedUSB = false;
                        onDeviceStateChange();
                    }
                }
            }
        };
        IntentFilter deviceDetachFilter = new IntentFilter();
        deviceDetachFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        registerReceiver(mUsbReceiver, deviceDetachFilter);

        FirebaseDatabase.getInstance().setPersistenceEnabled(true);
        DatabaseReference scoresRef = FirebaseDatabase.getInstance().getReference("sailplans");
        scoresRef.keepSynced(true);
        //DatabaseReference statsRef = FirebaseDatabase.getInstance().getReference("stats");
        //statsRef.keepSynced(true);
        //connectedRef = FirebaseDatabase.getInstance().getReference().child("connected");
        //connectedRef.keepSynced(false);
    }

    public void refreshUSBDevice() {
        Log.e(TAG, "Refreshing USB");
        // Set up USB
        connectedUSB = false;
        permissionNeeded = false;
        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
        if (!availableDrivers.isEmpty()) {
            // Open a connection to the first available driver.
            UsbSerialDriver driver = availableDrivers.get(0);
            UsbDeviceConnection connection = manager.openDevice(driver.getDevice());
            if (connection == null) {
                // You probably need to call UsbManager.requestPermission(driver.getDevice(), ..)
                permissionNeeded = true;
                Log.e(TAG, "Need Permissions");
                return;
            }

            sPort = driver.getPorts().get(0);
            //Log.d(TAG, "Resumed, port=" + sPort);
            if (sPort != null) {
                try {
                    sPort.open(connection);
                    sPort.setParameters(57600, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
                    connectedUSB = true;
                    Log.e(TAG, "Device connected" + sPort.getClass().getSimpleName());
                    onDeviceStateChange();
                } catch (IOException e) {
                    //Log.e(TAG, "Error setting up device: " + e.getMessage(), e);
                    //((ListItem) myListItems[10]).setText("Error opening device: " + e.getMessage());
                    Log.e(TAG, "Error 1");
                    connectedUSB = false;
                    try {
                        sPort.close();
                    } catch (IOException e2) {
                        // Ignore.
                    }
                    sPort = null;
                    return;
                }
                //((ListItem) myListItems[10]).setText(sPort.getClass().getSimpleName() + " connected");
            }
        } else {
            Log.e(TAG, "Device not detected");
        }
    }

    private void onDeviceStateChange() {
        Log.e(TAG, "Device state changed");
        for (OnUSBStateChangedListener listener : onUSBStateChangedListeners)
            listener.onUSBStateChanged(connectedUSB);
        // Stop IO manager
        if (mSerialIoManager != null) {
            Log.e(TAG, "Stopping io manager ..");
            mSerialIoManager.stop();
            mSerialIoManager = null;
        }
        // Start IO manager
        if (sPort != null) {
            Log.e(TAG, "Starting io manager ..");
            mSerialIoManager = new SerialInputOutputManager(sPort, mySerialInputOutputManagerListener);
            mExecutor.submit(mSerialIoManager);
        }
        //connectedRef.setValue(connectedUSB);
    }

    public void addOnIncomingTransmissionListener(OnIncomingTransmissionListener toAdd) {
        onIncomingTransmissionListeners.add(toAdd);
    }

    public void removeonIncomingTransmissionListener(OnIncomingTransmissionListener toRemove) {
        onIncomingTransmissionListeners.remove(toRemove);
    }

    public void addOnUSBStateChangedListener(OnUSBStateChangedListener toAdd) {
        toAdd.onUSBStateChanged(connectedUSB);
        onUSBStateChangedListeners.add(toAdd);
    }

    public void removeOnUSBStateChangedListener(OnUSBStateChangedListener toRemove) {
        onUSBStateChangedListeners.remove(toRemove);
    }

    public boolean isConnectedUSB() {
        return connectedUSB;
    }

    public boolean isPermissionNeeded() {
        return permissionNeeded;
    }

    public boolean sendTransmission(XMessage message) {
        // Takes a XMessage and sends it over serial
        if (sPort != null) {
            try {
                sPort.write(message.toString().getBytes(), 500);
                Log.i(TAG, "Sending "+ message.toString() );
                return true;
            } catch (IOException e) {
                // Deal with error.
                Log.e(TAG, "Something went wrong");
            } catch (NullPointerException e) {
                // Ignore
                Log.e(TAG, "sPort is null");
            }
        }
        return false;
    }

    @Override
    public void onTerminate() {
        // Stop the IO manager
        if (mSerialIoManager != null) {
            Log.i(TAG, "Stopping io manager ..");
            mSerialIoManager.stop();
            mSerialIoManager = null;
        }
        if (sPort != null) {
            try {
                sPort.close();
            } catch (IOException e) {
                // Ignore.
            }
            sPort = null;
        }
        if (mUsbReceiver != null) {
            unregisterReceiver(mUsbReceiver);
            mUsbReceiver = null;
        }
        super.onTerminate();
    }
}

