package com.mast.boatcontrol;

import android.Manifest;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.BitmapDescriptorFactory;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.Marker;
import com.google.android.gms.maps.model.MarkerOptions;
import com.google.firebase.database.ChildEventListener;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.mast.boatcontrol.models.DashboardIconView;
import com.mast.boatcontrol.models.ListItem;
import com.mast.boatcontrol.models.XMessage;
import com.sothree.slidinguppanel.SlidingUpPanelLayout;

import java.text.DateFormat;
import java.util.Date;
import java.util.List;

import static android.app.PendingIntent.FLAG_UPDATE_CURRENT;

public class MainActivity extends AppCompatActivity implements OnMapReadyCallback {

    private static String TAG = "MainActivity.java";

    // View item declarations
    View[] myListItems;
    Boolean menuShowing = true;
    Boolean motorLocked = false;
    MyApplication myApplication;
    SlidingUpPanelLayout suplMain;
    LinearLayout llMain, llMenu, llTransmitterError;
    DashboardIconView divMotorLock;
    TextView tvTransmitterError;

    AlertDialog.Builder startBuilder, calibrateBuilder;
    AlertDialog startDialog, calibrateDialog;
    String startUpTerminal = "";
    String lastTransmissionTime;
    GoogleMap myGoogleMap;
    Marker boatLocation, targetLocation;


    OnIncomingTransmissionListener myIncomingTransmissionListener;
    OnUSBStateChangedListener myUSBStateChangedListener;
    int rcCalibrationStage = 0;
    int wCalibrationStage = 0;
    ChildEventListener celStatistics;
    ValueEventListener velConnected;
    DatabaseReference myRef, connectedRef;

    private Handler mOfflineHandler = new Handler();
    private Runnable mOfflineRunnable;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //getSupportActionBar().setElevation(0);
        //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT);
        setTheme(R.style.AppTheme);
        myApplication = (MyApplication) getApplication();

        // Set up sliding panel
        suplMain = (SlidingUpPanelLayout) findViewById(R.id.activity_main);
        suplMain.setPanelState(SlidingUpPanelLayout.PanelState.EXPANDED);
        suplMain.setTouchEnabled(false);

        // Set up interface
        llMain = (LinearLayout) findViewById(R.id.llMain);
        llMenu = (LinearLayout) findViewById(R.id.llMenu);
        llTransmitterError = (LinearLayout) findViewById(R.id.llTransmitterError);
        tvTransmitterError = (TextView) findViewById(R.id.tvTransmitterError);

        // Set up startup dialog
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            startBuilder = new AlertDialog.Builder(this, android.R.style.Theme_Material_Light_Dialog_NoActionBar);
        } else {
            startBuilder = new AlertDialog.Builder(this);
        }
        startBuilder.setTitle("Boat Startup")
                .setPositiveButton("Close", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        // continue with delete
                    }
                })
                .setMessage("")
                .setIcon(R.drawable.ic_power);
        startDialog = startBuilder.create();
        startDialog.setCanceledOnTouchOutside(false);

        // Set up calibrate dialog
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            calibrateBuilder = new AlertDialog.Builder(this, android.R.style.Theme_Material_Light_Dialog_NoActionBar);
        } else {
            calibrateBuilder = new AlertDialog.Builder(this);
        }
        calibrateBuilder.setTitle("Calibrate Device")
                .setPositiveButton("Done", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (rcCalibrationStage == 1) {
                            myApplication.sendTransmission(new XMessage(37, "2"));
                        } else if (rcCalibrationStage == 2) {
                            myApplication.sendTransmission(new XMessage(37, "3"));
                        }
                        if (wCalibrationStage == 1) {
                            myApplication.sendTransmission(new XMessage(56, "2"));
                        }
                        wCalibrationStage = rcCalibrationStage = 0;
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        wCalibrationStage = 0;
                        rcCalibrationStage = 0;
                        // Ignore
                    }
                })
                .setMessage("Waiting for boat to respond...")
                .setIcon(R.drawable.ic_remote);
        calibrateDialog = calibrateBuilder.create();
        calibrateDialog.setCanceledOnTouchOutside(false);

        // Set up telemetry list
        Resources res = getResources();
        // Each item title, subtitle and type is stored in values/strings.xml
        // The type determines if it is a list item or a subheading
        String[] ItemTitles = res.getStringArray(R.array.list_titles);
        String[] ItemSubTitles = res.getStringArray(R.array.list_subtitles);
        String[] ItemTags = res.getStringArray(R.array.list_tags);
        int[] ItemTypes = res.getIntArray(R.array.list_types);
        myListItems = new View[ItemTypes.length];
        // LinearLayout Params for how the list of items and headings should behave
        LinearLayout.LayoutParams SubtitleParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        SubtitleParams.setMargins(0, (int) res.getDimension(R.dimen.activity_vertical_margin), 0, (int) res.getDimension(R.dimen.listitem_vertical_margin));
        LinearLayout.LayoutParams ListItemParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        // Inflate all the items
        for (int i = 0; i < ItemTypes.length; i++) {
            if (ItemTypes[i] == 0) {
                myListItems[i] = getLayoutInflater().inflate(R.layout.tvheader_template, null);
                ((TextView) myListItems[i]).setText(ItemTitles[i]);
                myListItems[i].setLayoutParams(SubtitleParams);
            } else {
                myListItems[i] = new ListItem(this);
                myListItems[i].setLayoutParams(ListItemParams);
                ((ListItem) myListItems[i]).setTitle(ItemTitles[i]);
                if (!ItemSubTitles[i].equals(""))
                    ((ListItem) myListItems[i]).setText(ItemSubTitles[i]);
                ((ListItem) myListItems[i]).TAG = ItemTags[i];
            }
            llMain.addView(myListItems[i]);
        }

        // Set up map
        SupportMapFragment mapFragment = (SupportMapFragment) getSupportFragmentManager()
                .findFragmentById(R.id.map);
        mapFragment.getMapAsync(this);


        // Set up menu
        DashboardIconView divDash = (DashboardIconView) findViewById(R.id.divDash);
        divMotorLock = (DashboardIconView) findViewById(R.id.divMotorLock);
        DashboardIconView divAuto = (DashboardIconView) findViewById(R.id.divAuto);
        DashboardIconView divLEDs = (DashboardIconView) findViewById(R.id.divLEDs);
        DashboardIconView divCalibrate = (DashboardIconView) findViewById(R.id.divCalibrate);
        DashboardIconView divEmergStop = (DashboardIconView) findViewById(R.id.divEmergStop);
        divDash.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                suplMain.setPanelState(SlidingUpPanelLayout.PanelState.COLLAPSED);
                suplMain.setTouchEnabled(true);
                llMain.setVisibility(View.VISIBLE);
                llMenu.setVisibility(View.GONE);
                menuShowing = false;
            }
        });
        // Start the sail plans class when autosail menu button is pressed
        divAuto.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(MainActivity.this, SailPlansActivity.class));
            }
        });
        // Locks motors, enters cli mode
        divEmergStop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                myApplication.sendTransmission(new XMessage(21, "0"));
                myApplication.sendTransmission(new XMessage(30, "0"));
            }
        });
        // Trigger LED lights on boat when LED menu button is pressed
        divLEDs.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                XMessage toggleLights = new XMessage(62, "1");
                myApplication.sendTransmission(toggleLights);
            }
        });
        // Calibrate RC
        divCalibrate.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                XMessage toggleLights = new XMessage(37, "1");
                myApplication.sendTransmission(toggleLights);
                calibrateDialog.show();
            }
        });
        // Lock or unlock the motors
        divMotorLock.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (motorLocked) {
                    // Unlock motors
                    myApplication.sendTransmission(new XMessage(31, ""));
                } else {
                    // Lock motors
                    myApplication.sendTransmission(new XMessage(30, ""));
                }
            }
        });

        // Check Permissions
        if (ContextCompat.checkSelfPermission(MainActivity.this,
                Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(MainActivity.this,
                    new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                    500);

        }

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
                    myRef.removeEventListener(celStatistics);
                    tvTransmitterError.setText("Awaiting transmissions");
                    llTransmitterError.setBackgroundColor(ContextCompat.getColor(MainActivity.this, R.color.colorAccent));
                    llTransmitterError.setVisibility(View.VISIBLE);
                    //myRef.removeEventListener(velConnected);
                } else {
                    //myRef.addChildEventListener(celStatistics);
                    //connectedRef.addValueEventListener(velConnected);
                    llTransmitterError.setVisibility(View.VISIBLE);
                    tvTransmitterError.setText("Transmitter not connected");
                    llTransmitterError.setBackgroundColor(ContextCompat.getColor(MainActivity.this, R.color.colorError));
                    if (myApplication.isPermissionNeeded()) {
                        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
                        List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
                        if (!availableDrivers.isEmpty()) {
                            UsbSerialDriver driver = availableDrivers.get(0);
                            UsbDeviceConnection connection = manager.openDevice(driver.getDevice());
                            if (connection == null) {
                                // You probably need to call UsbManager.requestPermission(driver.getDevice(), ..)
                                llTransmitterError.setBackgroundColor(ContextCompat.getColor(MainActivity.this, R.color.colorAccent));
                                manager.requestPermission(driver.getDevice(), PendingIntent.getActivity(MainActivity.this, 500, new Intent(MainActivity.this, MainActivity.class), FLAG_UPDATE_CURRENT));
                            }
                        }
                    }
                }
            }
        };

        // Sync data if usb transmitter not connected
        celStatistics = new ChildEventListener() {
            @Override
            public void onChildAdded(DataSnapshot dataSnapshot, String s) {
                ListItem temp = findListItembyTag(dataSnapshot.getKey());
                if (temp != null) {
                    temp.setText(dataSnapshot.getValue(String.class));
                }
            }

            @Override
            public void onChildChanged(DataSnapshot dataSnapshot, String s) {
                ListItem temp = findListItembyTag(dataSnapshot.getKey());
                if (temp != null) {
                    temp.setText(dataSnapshot.getValue(String.class));
                }
            }

            @Override
            public void onChildRemoved(DataSnapshot dataSnapshot) {
                ListItem temp = findListItembyTag(dataSnapshot.getKey());
                if (temp != null) {
                    temp.setText("Unknown");
                }
            }

            @Override
            public void onChildMoved(DataSnapshot dataSnapshot, String s) {

            }

            @Override
            public void onCancelled(DatabaseError databaseError) {

            }
        };

        velConnected = new ValueEventListener() {
            @Override
            public void onDataChange(DataSnapshot dataSnapshot) {
                if (dataSnapshot.getValue(Boolean.class)) {
                    llTransmitterError.setVisibility(View.VISIBLE);
                    //tvTransmitterError.setText("Transmitter not connected, telemetry available");
                    //llTransmitterError.setBackgroundColor(ContextCompat.getColor(MainActivity.this, R.color.colorAccent));
                } else {
                    llTransmitterError.setVisibility(View.VISIBLE);
                    tvTransmitterError.setText("Transmitter not connected");
                    llTransmitterError.setBackgroundColor(ContextCompat.getColor(MainActivity.this, R.color.colorError));
                }
            }

            @Override
            public void onCancelled(DatabaseError databaseError) {

            }
        };

        myRef = FirebaseDatabase.getInstance().getReference().child("stats");
        connectedRef = FirebaseDatabase.getInstance().getReference().child("connected");

        findListItembyTag("MODE").setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (myApplication.isConnectedUSB()) {
                    AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
                    builder.setTitle("Change Mode")
                            .setItems(R.array.modes_array, new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    myApplication.sendTransmission(new XMessage(21, "" + which));
                                }
                            });
                    builder.create();
                    builder.show();
                } else {
                    Toast.makeText(MainActivity.this, "Connect transmitter to continue", Toast.LENGTH_SHORT).show();
                }
            }
        });

        findListItembyTag("ENCODER").setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                myApplication.sendTransmission(new XMessage(56, "1"));
                calibrateDialog.show();
            }
        });

        mOfflineRunnable = new Runnable() {
            @Override
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        tvTransmitterError.setText("Boat Offline");
                        llTransmitterError.setBackgroundColor(ContextCompat.getColor(MainActivity.this, R.color.colorAccent));
                        llTransmitterError.setVisibility(View.VISIBLE);
                    }
                });
            }
        };
        findListItembyTag("STATUS").setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(MainActivity.this, RCActivity.class));
            }
        });
        findListItembyTag("HEADING").setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(MainActivity.this, RemoteActivity.class));
            }
        });
    }


    void updateListItem(String tag, String text) {
        ListItem temp = findListItembyTag(tag);
        if (temp != null) {
            temp.setText(text);
        }
        //myRef.child(tag).setValue(text);
    }

    @Override
    public void onMapReady(GoogleMap googleMap) {
        myGoogleMap = googleMap;
        myGoogleMap.setMapType(GoogleMap.MAP_TYPE_SATELLITE);
        // Center the map on Queen's University by default
        LatLng queens = new LatLng(38.982878, -76.478180);
        //myGoogleMap.addMarker(new MarkerOptions().position(queens));
        myGoogleMap.moveCamera(CameraUpdateFactory.newLatLng(queens));

    }

    @Override
    protected void onPause() {
        super.onPause();
        if (myApplication.isConnectedUSB()) {
            connectedRef.setValue(false);
        }
        myApplication.removeonIncomingTransmissionListener(myIncomingTransmissionListener);
        myApplication.removeOnUSBStateChangedListener(myUSBStateChangedListener);
        myRef.removeEventListener(celStatistics);
        connectedRef.removeEventListener(velConnected);
    }

    @Override
    protected void onResume() {
        super.onResume();
        Bundle myBundle = getIntent().getExtras();
        //if (myBundle != null && myBundle.getBoolean(EXTRA_PERMISSION_GRANTED)) {
        myApplication.refreshUSBDevice();
        //}
        myApplication.addOnIncomingTransmissionListener(myIncomingTransmissionListener);
        myApplication.addOnUSBStateChangedListener(myUSBStateChangedListener);
    }

    private void executeMessage(XMessage command) {
        switch (command.getData_code()) {
            case (501):
                break;
            case (10):
                if (command.getResponse_code() == 200) {
                    updateListItem("STATUS", command.getData());
                } else if (command.getResponse_code() == 201) {
                    Log.e(TAG, "Showing alert");
                    AlertDialog.Builder builder;
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                        builder = new AlertDialog.Builder(this, android.R.style.Theme_Material_Light_Dialog_NoActionBar);
                    } else {
                        builder = new AlertDialog.Builder(this);
                    }
                    builder.setTitle("Alert")
                            .setMessage(command.getData())
                            .setPositiveButton("Okay", new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    // continue with delete
                                }
                            })
                            .setIcon(R.drawable.ic_alert)
                            .show();
                } else if (command.getResponse_code() == 204) {
                    if (!startDialog.isShowing()) startDialog.show();
                    if (command.getData().equals("GO")) {
                        startUpTerminal = "";
                        startDialog.setMessage("Received power on notice");
                    } else {
                        startUpTerminal = startUpTerminal + (command.getData() + "\n");
                        startDialog.setMessage(startUpTerminal);
                    }
                }
                break;
            case (11):
                if (command.getData().equals("")) {
                    updateListItem("LOCATION", "No GPS Fix");
                    if (boatLocation != null) boatLocation.remove();
                    if (targetLocation != null) targetLocation.remove();
                } else {
                    String[] location = command.getData().split(" ");
                    if (location.length == 2) {

                        try {
                            if (boatLocation != null) boatLocation.remove();
                            LatLng boat = new LatLng(Double.parseDouble(location[0]) / 1000000, Double.parseDouble(location[1]) / 1000000);
                            boatLocation = myGoogleMap.addMarker(new MarkerOptions().position(boat));
                            myGoogleMap.animateCamera(CameraUpdateFactory.newLatLng(boat));
                            updateListItem("LOCATION", boat.latitude + ", " + boat.longitude);
                        } catch (NumberFormatException e) {
                            updateListItem("LOCATION", "GPS fix error");
                        }
                    }
                }
                break;
            case (12):
                String[] wind = command.getData().split(" ");
                if (wind.length == 2) {
                    updateListItem("WIND", wind[0] + " knots at " + wind[1] + " degrees");
                }
                break;
            case (13):
                updateListItem("HEADING", command.getData() + " degrees");
                break;
            case (14):
                updateListItem("MEGA_UPTIME", command.getData() + " seconds");
                break;
            case (15):
                updateListItem("MEGA_MEMORY", command.getData() + " bytes remaining");
                break;
            case (21):
                int mode = 0;
                try {
                    mode = Integer.parseInt(command.getData());
                } catch (NumberFormatException e) {

                }
                String modeString = getResources().getStringArray(R.array.modes_array)[mode] + ". Tap to change modes";
                String notPresent = "Not available in " + getResources().getStringArray(R.array.modes_array)[mode];
                String waiting = "Waiting for data";
                updateListItem("MODE", modeString);
                switch (mode) {
                    case (0):
                        updateListItem("TARGET", notPresent);
                        updateListItem("AUTO_STATUS", notPresent);
                        updateListItem("RSX", notPresent);
                        updateListItem("RSY", notPresent);
                        if (targetLocation != null) targetLocation.remove();
                        break;
                    case (1):
                        updateListItem("TARGET", notPresent);
                        updateListItem("AUTO_STATUS", notPresent);
                        if (targetLocation != null) targetLocation.remove();
                        break;
                    case (2):

                        break;
                }
                break;
            case (30):
                if (command.getResponse_code() == 200) {
                    divMotorLock.setLabel("Unlock Winch");
                    divMotorLock.setImage(R.drawable.ic_unlock);
                    motorLocked = true;
                }
                break;
            case (31):
                if (command.getResponse_code() == 200) {
                    divMotorLock.setLabel("Lock Winch");
                    divMotorLock.setImage(R.drawable.ic_lock);
                    motorLocked = false;
                }
                break;
            case (37):
                if (!calibrateDialog.isShowing()) calibrateDialog.show();
                if (command.getResponse_code() == 501) {
                    calibrateDialog.setMessage("Calibration error.");
                } else {
                    if (command.getData().equals("1")) {
                        calibrateDialog.setMessage("Set both sticks to high and then press done");
                        rcCalibrationStage = 1;
                    } else if (command.getData().equals("2")) {
                        calibrateDialog.setMessage("Set both sticks to low and then press done");
                        rcCalibrationStage = 2;
                    } else if (command.getData().equals("3")) {
                        calibrateDialog.setMessage("Calibration complete");
                        rcCalibrationStage = 0;
                    }
                }
                break;
            case (44):
                updateListItem("TARGET_ANGLE", command.getData());
                break;
            case (45):
                updateListItem("AUTO_STATUS", command.getData());
                break;
            case (46):
                String[] location = command.getData().split(" ");
                if (location.length == 2) {
                    try {
                        if (targetLocation != null) targetLocation.remove();
                        LatLng boat = new LatLng(Double.parseDouble(location[0]) / 1000000, Double.parseDouble(location[1]) / 1000000);
                        targetLocation = myGoogleMap.addMarker(new MarkerOptions().position(boat).icon(BitmapDescriptorFactory.defaultMarker(BitmapDescriptorFactory.HUE_BLUE)));
                        updateListItem("TARGET", boat.latitude + ", " + boat.longitude);
                    } catch (NumberFormatException e) {
                        updateListItem("TARGET", "Waypoint parse error");
                    }
                }
                break;
            case (50):
                updateListItem("TEMPERATURE", command.getData() + "°C");
                break;
            case (55):
                updateListItem("ENCODER", (Math.round(Double.parseDouble(command.getData()) - 610) / 225 * 100 * 10.0) / 10.0 + "%");
                break;
            case (56):
                if (!calibrateDialog.isShowing()) calibrateDialog.show();
                if (command.getResponse_code() == 501) {
                    calibrateDialog.setMessage("Calibration error.");
                } else {
                    if (command.getData().equals("1")) {
                        calibrateDialog.setMessage("Move winch to fully wound position and press done");
                        wCalibrationStage = 1;
                    } else if (command.getData().equals("2")) {
                        calibrateDialog.setMessage("Calibration complete");
                        wCalibrationStage = 0;
                    }
                }
            case (60):
                if (command.getResponse_code() == 200) {
                    updateListItem("SERVO_PIXY", command.getData() + " degrees");
                }
                break;
            case (61):
                if (command.getResponse_code() == 200) {
                    updateListItem("PIXY", "Buoy detected at " + command.getData() + " degrees");
                } else if (command.getResponse_code() == 204) {
                    updateListItem("PIXY", "No buoy detected");
                }
                break;
            case (63):
                updateListItem("RSX", command.getData());
                break;
            case (64):
                updateListItem("RSY", command.getData());
                break;
        }
        String transmissionTime = DateFormat.getDateTimeInstance().format(new Date());
        if (!transmissionTime.equals(lastTransmissionTime)) {
            lastTransmissionTime = DateFormat.getDateTimeInstance().format(new Date());
            updateListItem("LAST_TRANSMISSION", lastTransmissionTime);
        }
        mOfflineHandler.removeCallbacks(mOfflineRunnable);
        mOfflineHandler.postDelayed(mOfflineRunnable, 2000);
        llTransmitterError.setVisibility(View.GONE);
    }

    private ListItem findListItembyTag(String tag) {
        for (View someView : myListItems) {
            if (someView instanceof ListItem && ((ListItem) someView).TAG.equals(tag)) {
                return ((ListItem) someView);
            }
        }
        return null;
    }

    @Override
    public void onBackPressed() {
        // If the menu is not showing, show the menu
        // If the menu is already showing, close the app
        if (!menuShowing) {
            suplMain.setPanelState(SlidingUpPanelLayout.PanelState.EXPANDED);
            suplMain.setTouchEnabled(false);
            llMain.setVisibility(View.GONE);
            llMenu.setVisibility(View.VISIBLE);
            menuShowing = true;
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case 500: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {

                    // permission was granted, yay! Do the
                    // contacts-related task you need to do.

                } else {

                    // permission denied, boo! Disable the
                    // functionality that depends on this permission.
                }
                return;
            }
        }
    }
}
