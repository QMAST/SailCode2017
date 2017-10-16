package com.mast.boatcontrol.models;

import android.content.Context;
import android.support.v4.content.ContextCompat;

import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.model.Circle;
import com.google.android.gms.maps.model.CircleOptions;
import com.google.android.gms.maps.model.Marker;
import com.google.android.gms.maps.model.MarkerOptions;
import com.google.android.gms.maps.model.Polyline;
import com.google.android.gms.maps.model.PolylineOptions;
import com.google.firebase.database.Exclude;
import com.mast.boatcontrol.R;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by mast on 21/05/17.
 */

public class Route {
    // Class that helps calculate different route types based on an origin and draws them on a map
    public static final int ROUTE_TYPE_SIMPLE = 0;
    public static final int ROUTE_TYPE_EXPANDINGSQUARE = 1;
    public static final int ROUTE_TYPE_STATIONKEEP = 2;

    private GoogleMap myGoogleMap;
    private Context context;
    private myLatLng origin;
    private Polyline myPolyLine;
    private List<myLatLng> route = new ArrayList<>();
    private int routeType;
    private int angle = 0;
    private List<Marker> myMarkers = new ArrayList<>();

    public Route() {

    }

    public Route(Context c, myLatLng point) {
        route = new ArrayList<>();
        route.add(point);
        origin = point;
        this.context = c;
        this.routeType = ROUTE_TYPE_SIMPLE;
    }

    public Route(Context c, myLatLng origin, int routeType) {
        this.origin = origin;
        this.context = c;
        this.routeType = routeType;
        calculateRoute();
    }

    private static List<myLatLng> calculateExpandingSquareWaypoints(myLatLng origin, int angle) {
        // Returns an array of LatLng, each corresponding to a waypoint in the search pattern
        // Earth’s radius, sphere
        double Ra = 6378137.0;

        // Offsets for each waypoint relative to the previous
        double[] latOffset = {0, 10.0, 0.0, -20.0, 0.0, 40.0, 0.0, -60.0, 0.0, 80.0, 0.0, -100.0, 0.0, 120.0, 0.0, -140.0, 0.0, 140.0};
        double[] lonOffset = {0, 0.0, 20.0, 0.0, -40.0, 0.0, 60.0, 0.0, -80.0, 0.0, 100.0, 0.0, -120.0, 0.0, 140.0, 0.0, -160.0, 0.0};

        List<myLatLng> waypoints = new ArrayList<>();
        waypoints.add(origin);
        myLatLng prevWP = origin;
        // Calculates the waypoints that make up this search pattern
        for (int i = 1; i < latOffset.length; i++) {
            // Apply rotation
            double angleRad = angle * Math.PI / 180;
            double hyp = Math.sqrt((latOffset[i] * latOffset[i] + lonOffset[i] * lonOffset[i]));
            if (latOffset[i] < 0) hyp = hyp * -1;
            if (lonOffset[i] < 0) hyp = hyp * -1;
            double rLat = (Math.sin(angleRad + Math.asin(latOffset[i] / hyp))) * hyp;
            if (latOffset[i] < 0) rLat = rLat * -1;
            double rLon = (Math.cos(angleRad + Math.acos(lonOffset[i] / hyp))) * hyp;
            if (latOffset[i] < 0) rLat = rLat * -1;

            //Coordinate offsets in radians
            double dLat = rLat / Ra;
            double dLon = rLon / (Ra * Math.cos(Math.PI * prevWP.latitude / 180));

            //OffsetPosition, decimal degrees
            double latO = prevWP.latitude + dLat * 180 / Math.PI;
            double lonO = prevWP.longitude + dLon * 180 / Math.PI;

            prevWP = new myLatLng(latO, lonO);
            waypoints.add(prevWP);
        }
        return waypoints;
    }

    public int getAngle() {
        return angle;
    }

    public void setAngle(int angle) {
        this.angle = angle;
        calculateRoute();
    }

    public void calculateRoute() {
        switch (routeType) {
            case (ROUTE_TYPE_EXPANDINGSQUARE):
                route = calculateExpandingSquareWaypoints(origin, angle);
                break;
            case (ROUTE_TYPE_STATIONKEEP):
                break;
        }
    }

    public void drawMap(GoogleMap myGoogleMap) {
        this.myGoogleMap = myGoogleMap;
        myGoogleMap.clear();
        switch (routeType) {
            case (ROUTE_TYPE_EXPANDINGSQUARE):
                myGoogleMap.addMarker(new MarkerOptions().position(origin.getLatLng()));
                myGoogleMap.animateCamera(CameraUpdateFactory.newLatLngZoom(origin.getLatLng(), 17));
                Circle circle = myGoogleMap.addCircle(new CircleOptions()
                        .center(origin.getLatLng())
                        .radius(100)
                        .strokeColor(R.color.colorNavOutline)
                        .fillColor(ContextCompat.getColor(context, R.color.colorNavBG)));
                break;
            case (ROUTE_TYPE_STATIONKEEP):
                break;
            case (ROUTE_TYPE_SIMPLE):
                break;
        }

        redrawMap();
    }

    public void redrawMap() {
        if (myGoogleMap != null) {
            if (myPolyLine != null) myPolyLine.remove();
            PolylineOptions myPolyOptions = new PolylineOptions();
            myPolyOptions.color(ContextCompat.getColor(context, R.color.colorPrimary));
            switch (routeType) {
                case (ROUTE_TYPE_EXPANDINGSQUARE):
                    for (myLatLng waypoint : route) {
                        myPolyOptions.add(waypoint.getLatLng());
                    }
                    myPolyLine = myGoogleMap.addPolyline(myPolyOptions);
                    break;
                case (ROUTE_TYPE_STATIONKEEP):
                    break;
                case (ROUTE_TYPE_SIMPLE):
                    for (Marker myMarker : myMarkers) {
                        myMarkers.remove(myMarker);
                        myMarker.remove();
                    }
                    boolean first = true;
                    for (myLatLng waypoint : route) {
                        if (first) {
                            myGoogleMap.animateCamera(CameraUpdateFactory.newLatLngZoom(waypoint.getLatLng(), 17));
                            first = false;
                        }
                        myPolyOptions.add(waypoint.getLatLng());
                        myMarkers.add(myGoogleMap.addMarker(new MarkerOptions()
                                .position(waypoint.getLatLng())));
                    }
                    myPolyLine = myGoogleMap.addPolyline(myPolyOptions);
                    break;
            }
        } else {
            throw new NullPointerException("drawMap(GoogleMap myGoogleMap) must be called before redrawMap()");
        }
    }

    public myLatLng getOrigin() {
        return origin;
    }

    protected void setOrigin(myLatLng origin) {
        this.origin = origin;
    }


    public int getRouteType() {
        return routeType;
    }

    public void setRouteType(int routeType) {
        this.routeType = routeType;
    }

    @Exclude
    public List<myLatLng> getRoute() {
        return route;
    }
}
