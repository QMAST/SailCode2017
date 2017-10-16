package com.mast.boatcontrol;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.google.android.gms.common.GooglePlayServicesNotAvailableException;
import com.google.android.gms.location.places.Place;
import com.google.android.gms.location.places.ui.PlacePicker;
import com.google.android.gms.maps.model.LatLng;
import com.mast.boatcontrol.models.Route;
import com.mast.boatcontrol.models.myLatLng;

public class SelectWPDialog extends AppCompatActivity {

    Button bDone, bSelectWP;
    EditText etLatitude, etLongitude;
    private static int PLACE_PICKER_REQUEST = 1;
    Intent intent;
    Context myContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_select_wpdialog);

        intent = this.getIntent();
        bDone = (Button) findViewById(R.id.bDone);
        bSelectWP = (Button) findViewById(R.id.bSelectWP);
        etLatitude = (EditText) findViewById(R.id.etLatitude);
        etLongitude = (EditText) findViewById(R.id.etLongitude);
        myContext  = this;

        bSelectWP.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    PlacePicker.IntentBuilder builder = new PlacePicker.IntentBuilder();
                    startActivityForResult(builder.build(SelectWPDialog.this), PLACE_PICKER_REQUEST);
                } catch (GooglePlayServicesNotAvailableException e) {
                    e.printStackTrace();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
        bDone.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!etLatitude.getText().toString().equals("") && !etLongitude.getText().toString().equals("")) {
                    double lat = Double.parseDouble(etLatitude.getText().toString());
                    double lon = Double.parseDouble(etLongitude.getText().toString());
                    if (lat <= 90 && lat >= -90 && lon <= 180 && lon >= -180) {
                        myLatLng myDestination = new myLatLng(lat, lon);
                        intent.putExtra("lat", lat);
                        intent.putExtra("lon", lon);
                        setResult(RESULT_OK, intent);
                        finish();
                    } else {
                        Toast.makeText(SelectWPDialog.this, "Please enter valid coordinates", Toast.LENGTH_SHORT).show();
                        return;
                    }
                } else {
                    Toast.makeText(SelectWPDialog.this, "Please enter valid coordinates", Toast.LENGTH_SHORT).show();
                    return;
                }
            }
        });
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
        }
    }
}
