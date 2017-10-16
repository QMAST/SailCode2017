package com.mast.boatcontrol;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.gms.common.GooglePlayServicesNotAvailableException;
import com.google.android.gms.location.places.Place;
import com.google.android.gms.location.places.ui.PlacePicker;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.LatLng;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;
import com.mast.boatcontrol.models.Route;
import com.mast.boatcontrol.models.XMessage;
import com.mast.boatcontrol.models.myLatLng;

import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.List;

public class NewSailPlanActivity extends AppCompatActivity implements OnMapReadyCallback {

    private static String TAG = "NewSailPlanActivity.java";
    private static int PLACE_PICKER_REQUEST = 1;
    private MyApplication myApplication;
    private int step, plan_type;
    private TextView tvMain, tvHeading, tvHeading2;
    private Spinner spType;
    private Button bNext, bBack, bSelectWP, bAddWaypoints;
    private ListView lvWaypoints;
    private SeekBar sbAngle;
    private EditText etLatitude, etLongitude;
    private GoogleMap myGoogleMap;
    private View googleMapView;
    private RelativeLayout rlWaypoint;
    private Handler mLastHandler = new Handler();
    private Runnable mLastRunnable;
    private Route myRoute;
    private int searchAngle = 0;
    private List<XMessage> transmissionQueue;
    private List<myLatLng> selectedWaypoints = new ArrayList<>();
    Boolean transmitting = false;
    String sailPlanID;
    FirebaseDatabase database;
    OnIncomingTransmissionListener myIncomingListener;
    OnUSBStateChangedListener myUSBStateListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_new_sail_plan);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        myApplication = (MyApplication) getApplication();

        spType = (Spinner) findViewById(R.id.spType);
        tvMain = (TextView) findViewById(R.id.tvMain);
        tvHeading = (TextView) findViewById(R.id.tvHeading);
        tvHeading2 = (TextView) findViewById(R.id.tvHeading2);
        bNext = (Button) findViewById(R.id.bNext);
        bBack = (Button) findViewById(R.id.bBack);
        bSelectWP = (Button) findViewById(R.id.bSelectWP);
        rlWaypoint = (RelativeLayout) findViewById(R.id.rlWaypoint);
        etLatitude = (EditText) findViewById(R.id.etLatitude);
        etLongitude = (EditText) findViewById(R.id.etLongitude);
        googleMapView = findViewById(R.id.map);
        sbAngle = (SeekBar) findViewById(R.id.sbAngle);
        bAddWaypoints = (Button) findViewById(R.id.bAddWaypoints);


        lvWaypoints = (ListView) findViewById(R.id.lvWaypoints);


        transmissionQueue = new ArrayList<>();


        mLastRunnable = new Runnable() {
            @Override
            public void run() {
                boolean sent = false;
                for (int j = 0; j < transmissionQueue.size(); j++) {
                    if (!transmissionQueue.get(j).verified) {
                        myApplication.sendTransmission(transmissionQueue.get(j));
                        sent = true;
                        break;
                    }
                }
                if (sent) {
                    mLastHandler.removeCallbacks(mLastRunnable);
                    mLastHandler.postDelayed(mLastRunnable, 250);
                }
            }
        };

        spType.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                // Update the plan type
                plan_type = spType.getSelectedItemPosition();
                refreshPlan();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });
        bBack.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onBackPressed();
            }
        });
        bNext.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                loadStep(step + 1);
            }
        });
        bSelectWP.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    PlacePicker.IntentBuilder builder = new PlacePicker.IntentBuilder();
                    startActivityForResult(builder.build(NewSailPlanActivity.this), PLACE_PICKER_REQUEST);
                } catch (GooglePlayServicesNotAvailableException e) {
                    e.printStackTrace();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
        sbAngle.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (progress % 5 == 0) {
                    if (myRoute != null) {
                        searchAngle = progress;
                        myRoute.setAngle(searchAngle);
                        myRoute.redrawMap();
                    }
                    if (step == 2)
                        tvHeading2.setText("Adjust Angle (" + progress + ")");
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        bAddWaypoints.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivityForResult(new Intent(NewSailPlanActivity.this, SelectWPDialog.class), 10);
            }
        });

        // Set up map
        SupportMapFragment mapFragment = (SupportMapFragment) getSupportFragmentManager()
                .findFragmentById(R.id.map);
        mapFragment.getMapAsync(this);
        loadStep(1);

        database = FirebaseDatabase.getInstance();
        sailPlanID = getIntent().getStringExtra("id");
        if (sailPlanID != null) {
            final DatabaseReference myRef = database.getReference("sailplans").child(sailPlanID);
            myRef.addListenerForSingleValueEvent(new ValueEventListener() {
                @Override
                public void onDataChange(DataSnapshot dataSnapshot) {
                    if (dataSnapshot.exists()) {
                        Route myRoute = dataSnapshot.getValue(Route.class);
                        if (myRoute.getRouteType() == Route.ROUTE_TYPE_EXPANDINGSQUARE) {
                            searchAngle = myRoute.getAngle();
                            sbAngle.setProgress(myRoute.getAngle());
                        }
                        if (myRoute.getOrigin() != null) {
                            etLatitude.setText(myRoute.getOrigin().latitude + "");
                            etLongitude.setText(myRoute.getOrigin().longitude + "");
                        }
                        switch (myRoute.getRouteType()) {
                            case Route.ROUTE_TYPE_EXPANDINGSQUARE:
                                spType.setSelection(3);
                                break;
                            case Route.ROUTE_TYPE_SIMPLE:
                                spType.setSelection(0);
                                break;
                            case Route.ROUTE_TYPE_STATIONKEEP:
                                spType.setSelection(2);
                                break;
                        }
                    }
                }

                @Override
                public void onCancelled(DatabaseError databaseError) {

                }
            });
        } else {
            DatabaseReference myRef = database.getReference("sailplans");
            sailPlanID = myRef.push().getKey();
        }

        myUSBStateListener = new OnUSBStateChangedListener() {
            @Override
            public void onUSBStateChanged(boolean connected) {
                if (step == 3 || step == 4) {
                    setUSBState(connected, step);
                }
            }
        };
        myIncomingListener = new OnIncomingTransmissionListener() {
            @Override
            public void onIncomingTransmission(XMessage transmission) {
                transmissionRecieved(transmission);
            }
        };
    }

    private void setUSBState(boolean connected, int curStep) {
        if (connected) {
            if (curStep == 3) {
                tvMain.setText("Plan saved. Ready to upload plan to boat.");
            } else if (curStep == 4) {
                tvMain.setText("Plan uploaded. Ready to engage autosail mode.");
            }
            bNext.setEnabled(true);
        } else {
            if (curStep == 3) {
                tvMain.setText("Plan saved. Ready to upload plan to boat. Insert USB transmitter to continue.");
            } else if (curStep == 4) {
                tvMain.setText("Plan uploaded. Ready to engage autosail mode. Insert USB transmitter to continue.");
            }
            bNext.setEnabled(false);
        }
    }

    private void refreshPlan() {
        // Refresh the labels on screen to more accurately reflect the mode selected in the drop down
        String[] modeDescriptions = getResources().getStringArray(R.array.list_autosail_descriptions);
        tvMain.setText(modeDescriptions[plan_type]);
        if (plan_type == 0) {
            tvHeading2.setText("Destination");
            rlWaypoint.setVisibility(View.VISIBLE);
        } else if (plan_type == 1) {
            tvHeading2.setText("Waypoints");
            rlWaypoint.setVisibility(View.GONE);
        } else if (plan_type == 2) {
            tvHeading2.setText("Box corner points");
            rlWaypoint.setVisibility(View.GONE);
        } else if (plan_type == 3) {
            tvHeading2.setText("Search origin");
            rlWaypoint.setVisibility(View.VISIBLE);
        }
    }

    private void addTransmissionToQueue(XMessage message) {
        transmissionQueue.add(message);
    }

    private void startQueuedTransmission() {
        myApplication.sendTransmission(transmissionQueue.get(0));
        transmitting = true;
        mLastHandler.removeCallbacks(mLastRunnable);
        mLastHandler.postDelayed(mLastRunnable, 250);
    }

    private void transmissionRecieved(XMessage message) {
        int incomingCode = message.getData_code();
        String incomingData = message.getData();

        for (int i = 0; i < transmissionQueue.size(); i++) {
            if (!transmissionQueue.get(i).verified) {
                if (message.getResponse_code() == 200 && incomingCode == transmissionQueue.get(i).getData_code() && incomingData.equals(transmissionQueue.get(i).getData())) {
                    transmissionQueue.get(i).verified = true;
                    //Log.i(TAG, "Verified: " + transmissionQueue.get(i).toString());
                    boolean sent = false;
                    for (int j = 0; j < transmissionQueue.size(); j++) {
                        if (!transmissionQueue.get(j).verified) {
                            myApplication.sendTransmission(transmissionQueue.get(j));
                            sent = true;
                            break;
                        }
                    }
                    if (!sent) {
                        // Done transmitting everything in the queue!!
                        Log.i(TAG, "Everything has been sent and verified");
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                bNext.setText("Engage Autosail");
                                bNext.setEnabled(true);
                                setUSBState(myApplication.isConnectedUSB(), 4);
                            }
                        });
                    }
                    break;
                }
            }
        }

    }

    private void loadStep(int step) {
        switch (step) {
            case 1:
                // Select plan type and enter coordinates
                spType.setVisibility(View.VISIBLE);
                tvMain.setVisibility(View.VISIBLE);
                tvHeading.setVisibility(View.VISIBLE);
                rlWaypoint.setVisibility(View.GONE);
                sbAngle.setVisibility(View.GONE);
                googleMapView.setVisibility(View.GONE);
                tvHeading.setText("Plan type");
                bNext.setText("Next");
                refreshPlan();
                break;
            case 2:
                // Calculate route and confirm on map
                tvHeading2.setVisibility(View.GONE);
                if (plan_type == 0) {
                    tvMain.setText("Please confirm destination");
                    if (!etLatitude.getText().toString().equals("") && !etLongitude.getText().toString().equals("")) {
                        double lat = Double.parseDouble(etLatitude.getText().toString());
                        double lon = Double.parseDouble(etLongitude.getText().toString());
                        if (lat <= 90 && lat >= -90 && lon <= 180 && lon >= -180) {
                            myLatLng myDestination = new myLatLng(lat, lon);

                            myRoute = new Route(NewSailPlanActivity.this, myDestination);
                            myRoute.drawMap(myGoogleMap);
                        } else {
                            Toast.makeText(NewSailPlanActivity.this, "Please enter valid coordinates", Toast.LENGTH_SHORT).show();
                            return;
                        }
                    } else {
                        Toast.makeText(NewSailPlanActivity.this, "Please enter valid coordinates", Toast.LENGTH_SHORT).show();
                        return;
                    }
                } else if (plan_type == 1) {
                    tvMain.setText("Please confirm waypoints");
                } else if (plan_type == 2) {
                    tvMain.setText("Please confirm waypoints");
                } else if (plan_type == 3) {
                    tvMain.setText("Please confirm search route");
                    if (!etLatitude.getText().toString().equals("") && !etLongitude.getText().toString().equals("")) {
                        double lat = Double.parseDouble(etLatitude.getText().toString());
                        double lon = Double.parseDouble(etLongitude.getText().toString());
                        if (lat <= 90 && lat >= -90 && lon <= 180 && lon >= -180) {
                            // Waypoint entered is valid, continue with confirmation
                            final myLatLng myOrigin = new myLatLng(lat, lon);
                            myRoute = new Route(NewSailPlanActivity.this, myOrigin, Route.ROUTE_TYPE_EXPANDINGSQUARE);
                            if (searchAngle != 0) myRoute.setAngle(searchAngle);
                            myRoute.drawMap(myGoogleMap);

                            sbAngle.setVisibility(View.VISIBLE);
                            tvHeading2.setText("Adjust Angle (" + searchAngle + ")");
                            tvHeading2.setVisibility(View.VISIBLE);
                        } else {
                            Toast.makeText(NewSailPlanActivity.this, "Please enter valid coordinates", Toast.LENGTH_SHORT).show();
                            return;
                        }
                    } else {
                        Toast.makeText(NewSailPlanActivity.this, "Please enter valid coordinates", Toast.LENGTH_SHORT).show();
                        return;
                    }
                }
                tvHeading.setVisibility(View.GONE);
                spType.setVisibility(View.GONE);
                rlWaypoint.setVisibility(View.GONE);
                googleMapView.setVisibility(View.VISIBLE);
                bNext.setText("Save");
                break;
            case 3:
                // Save route and prepare for transmission
                DatabaseReference myRef = database.getReference("sailplans").child(sailPlanID);
                myRef.setValue(myRoute);
                sbAngle.setVisibility(View.GONE);
                tvHeading2.setVisibility(View.GONE);
                bNext.setText("Upload");
                setUSBState(myApplication.isConnectedUSB(), 3);
                break;
            case 4:
                // Transmit route and prepare for autosail engage
                bNext.setEnabled(false);
                bNext.setText("Uploading...");
                tvMain.setText("Uploading and verifying sail plan...");
                addTransmissionToQueue(new XMessage(21, "0"));
                addTransmissionToQueue(new XMessage(40, "" + myRoute.getRouteType()));
                List<myLatLng> route = myRoute.getRoute();
                for (int i = 0; i < route.size(); i++) {
                    String data = i + " " + String.format("%.6f", route.get(i).latitude) + " " + String.format("%.6f", route.get(i).latitude);
                    data = data.replace(".", "");
                    addTransmissionToQueue(new XMessage(20, data));
                }
                addTransmissionToQueue(new XMessage(23, "0"));
                addTransmissionToQueue(new XMessage(24, "" + (route.size() - 1)));
                startQueuedTransmission();
                break;
            case 5:
                // Engage autosail
                myApplication.sendTransmission(new XMessage(21, "2"));
                finish();
                break;
            default:
                return;
        }
        this.step = step;
    }


    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == PLACE_PICKER_REQUEST) {
            // Take the place picker coordinates and put them in the text box
            if (resultCode == RESULT_OK) {
                Place place = PlacePicker.getPlace(data, this);
                LatLng selectedPlace = place.getLatLng();
                etLatitude.setText(selectedPlace.latitude + "");
                etLongitude.setText(selectedPlace.longitude + "");
            }
        } else if (requestCode == 10 && resultCode == RESULT_OK) {
            Bundle extras = data.getExtras();
            int position = extras.getInt("position");
            if (!extras.getString("lat").equals("") && !extras.getString("lon").equals("")) {
                double lat = Double.parseDouble(extras.getString("lat"));
                double lon = Double.parseDouble(extras.getString("lon"));
                if (lat <= 90 && lat >= -90 && lon <= 180 && lon >= -180) {
                    myLatLng myDestination = new myLatLng(lat, lon);
                    selectedWaypoints.add(myDestination);
                    // TODO: get stuff and do stuff
                } else {
                    Toast.makeText(NewSailPlanActivity.this, "Please enter valid coordinates", Toast.LENGTH_SHORT).show();
                    return;
                }
            } else {
                Toast.makeText(NewSailPlanActivity.this, "Please enter valid coordinates", Toast.LENGTH_SHORT).show();
                return;
            }
        }


    }

    @Override
    public void onMapReady(GoogleMap googleMap) {
        myGoogleMap = googleMap;
        googleMap.setMapType(GoogleMap.MAP_TYPE_SATELLITE);
    }

    @Override
    public void onBackPressed() {
        // Load th previously step or exit activity
        if (step == 1 || step == 4 || step == 5) {
            super.onBackPressed();
        } else {
            loadStep(step - 1);
        }

    }

    @Override
    protected void onPause() {
        myApplication.removeonIncomingTransmissionListener(myIncomingListener);
        myApplication.removeOnUSBStateChangedListener(myUSBStateListener);
        super.onPause();
    }

    @Override
    protected void onResume() {
        myApplication.addOnIncomingTransmissionListener(myIncomingListener);
        myApplication.addOnUSBStateChangedListener(myUSBStateListener);
        super.onResume();
    }

    /*class MyArrayAdapter extends ArrayAdapter<myLatLng> {
        private final Context context;
        private final List<myLatLng> values;

        public MyArrayAdapter(Context context, int textViewResourceId) {
            super(context, textViewResourceId);
        }

        public MyArrayAdapter(Context context, int resource, List<myLatLng> items) {
            super(context, resource, items);
        }


        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            LayoutInflater inflater = (LayoutInflater) context
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            View rowView = inflater.inflate(R.layout.list_item, parent, false);

            View v = convertView;

            if (v == null) {
                LayoutInflater vi;
                vi = LayoutInflater.from(getContext());
                v = vi.inflate(R.layout.list_item, null);
            }

            myLatLng p = values.get(position);

            if (p != null) {
                TextView tvTitle = (TextView) v.findViewById(R.id.tvTitle);
                TextView tvText = (TextView)  v.findViewById(R.id.tvText);

                tvTitle.setText(p.getLatitude() + ", " + p.getLongitude());
            }

            return v;

        }
    }*/
}
