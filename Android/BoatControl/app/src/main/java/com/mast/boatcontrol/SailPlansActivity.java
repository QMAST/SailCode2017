package com.mast.boatcontrol;

import android.content.Intent;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.view.View;

import com.google.firebase.database.ChildEventListener;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.mast.boatcontrol.models.Route;

import java.util.ArrayList;
import java.util.List;

public class SailPlansActivity extends AppCompatActivity {

    private static final String TAG = "SailPlansActivity.java";
    private static final double SIG_FIGS = 1000000.0;
    RecyclerView rvSailPlans;
    private RecyclerView.Adapter mAdapter;
    private RecyclerView.LayoutManager mLayoutManager;
    List<SailPlan> mySailPlans = new ArrayList<>();
    FirebaseDatabase database;
    ChildEventListener celSailPlans;
    DatabaseReference myRef;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sail_plans);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                //Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG).setAction("Action", null).show();
                Intent in = new Intent(SailPlansActivity.this, NewSailPlanActivity.class);
                startActivity(in);
            }
        });
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        rvSailPlans = (RecyclerView) findViewById(R.id.rvSailPlans);
        mLayoutManager = new LinearLayoutManager(this);
        rvSailPlans.setLayoutManager(mLayoutManager);

        // specify an adapter (see also next example)
        mAdapter = new SailPlanAdapter(mySailPlans, new SailPlanAdapter.OnItemClickListener() {
            @Override
            public void onItemClick(String key) {
                Intent in = new Intent(SailPlansActivity.this, NewSailPlanActivity.class);
                in.putExtra("id", key);
                startActivity(in);
                finish();
            }
        });
        rvSailPlans.setAdapter(mAdapter);

        database = FirebaseDatabase.getInstance();
        myRef = database.getReference("sailplans");

        celSailPlans = new ChildEventListener() {
            @Override
            public void onChildAdded(DataSnapshot dataSnapshot, String s) {
                Route child = dataSnapshot.getValue(Route.class);
                String title, message;
                switch (child.getRouteType()) {
                    case Route.ROUTE_TYPE_EXPANDINGSQUARE:
                        title = "Sailbot 2017 - Search";
                        message = "Origin: " + Math.round (child.getOrigin().latitude * SIG_FIGS) / SIG_FIGS + ", " + Math.round (child.getOrigin().longitude * SIG_FIGS) / SIG_FIGS + "\nRotation: "+ child.getAngle() + " degrees";
                        break;
                    case Route.ROUTE_TYPE_SIMPLE:
                        title = "Simple Navigation";
                        message = "Destination: " + Math.round (child.getOrigin().latitude * SIG_FIGS) / SIG_FIGS + ", " + Math.round (child.getOrigin().longitude * SIG_FIGS) / SIG_FIGS;
                        break;

                    case Route.ROUTE_TYPE_STATIONKEEP:
                        title = "Sailbot 2017 - Station Keeping";
                        message = "Not Implemented";
                        break;
                    default:
                        title = "Unknown route type";
                        message = Math.round (child.getOrigin().latitude * SIG_FIGS) / SIG_FIGS + ", " + Math.round (child.getOrigin().longitude * SIG_FIGS) / SIG_FIGS;
                        break;
                }
                SailPlan toAdd = new SailPlan(title, message, dataSnapshot.getKey());
                mySailPlans.add(toAdd);
                mAdapter.notifyDataSetChanged();
            }

            @Override
            public void onChildChanged(DataSnapshot dataSnapshot, String s) {

                mAdapter.notifyDataSetChanged();
            }

            @Override
            public void onChildRemoved(DataSnapshot dataSnapshot) {

                mAdapter.notifyDataSetChanged();
            }

            @Override
            public void onChildMoved(DataSnapshot dataSnapshot, String s) {

            }

            @Override
            public void onCancelled(DatabaseError databaseError) {

            }
        };
        myRef.addChildEventListener(celSailPlans);
    }

    @Override
    public boolean onSupportNavigateUp() {
        finish();
        return true;
    }

    public class SailPlan {
        String title, text, key;

        public SailPlan() {
        }

        public SailPlan(String title, String text, String key) {
            this.title = title;
            this.text = text;
            this.key = key;
        }

        public String getTitle() {
            return title;
        }

        public void setTitle(String title) {
            this.title = title;
        }

        public String getText() {
            return text;
        }

        public void setText(String text) {
            this.text = text;
        }

        public String getKey() {
            return key;
        }

        public void setKey(String key) {
            this.key = key;
        }
    }
}
