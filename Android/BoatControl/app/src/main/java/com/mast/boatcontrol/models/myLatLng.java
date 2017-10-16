package com.mast.boatcontrol.models;

import com.google.android.gms.maps.model.LatLng;
import com.google.firebase.database.Exclude;

/**
 * Created by Angelo on 5/26/2017.
 */

public class myLatLng {
    public double latitude, longitude;
    private LatLng representation;

    public myLatLng() {
    }

    public myLatLng(double lat, double lon) {
        this.latitude = lat;
        this.longitude = lon;
    }

    public double getLatitude() {
        return latitude;
    }

    public void setLatitude(double lat) {
        this.latitude = lat;
    }

    public double getLongitude() {
        return longitude;
    }

    public void setLongitude(double lon) {
        this.longitude = lon;
    }

    @Exclude
    public LatLng getLatLng() {
        if (representation != null){
            if (latitude == 0 || longitude == 0) return null;
            if(representation.latitude == latitude && representation.longitude ==longitude){
                return representation;
            }else{
                representation = new LatLng(latitude,longitude);
            }
        }else{
            representation = new LatLng(latitude,longitude);representation = new LatLng(latitude,longitude);
        }
        return representation;
    }
}
