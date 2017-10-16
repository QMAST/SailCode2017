package com.mast.boatcontrol;

import android.support.v7.widget.CardView;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.List;

/**
 * Created by Angelo on 5/26/2017.
 */

public class SailPlanAdapter extends RecyclerView.Adapter<SailPlanAdapter.ViewHolder> {
    private static final String TAG = "SailPlanAdapter.java";
    private List<SailPlansActivity.SailPlan> sailPlans;
    private final OnItemClickListener listener;
    // Provide a reference to the views for each data item
    // Complex data items may need more than one view per item, and
    // you provide access to all the views for a data item in a view holder
    public static class ViewHolder extends RecyclerView.ViewHolder{
        // each data item is just a string in this case
        public CardView mCard;
        public  TextView mTitle, mText;
        public ViewHolder(View myView) {
            super(myView);
            mCard = (CardView) myView.findViewById(R.id.card_view);
            mTitle = (TextView) myView.findViewById(R.id.tvTitle);
            mText = (TextView) myView.findViewById(R.id.tvText);
        }

    }

    // Provide a suitable constructor (depends on the kind of dataset)
    public SailPlanAdapter(List<SailPlansActivity.SailPlan> sailPlans, OnItemClickListener listener) {
        this.sailPlans = sailPlans;
        this.listener = listener;
    }

    // Create new views (invoked by the layout manager)
    @Override
    public SailPlanAdapter.ViewHolder onCreateViewHolder(ViewGroup parent,
                                                   int viewType) {
        // create a new view
        Log.e(TAG, "Inflating");
        View myView = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.card_sail_plans, parent, false);
        // set the view's size, margins, paddings and layout parameters
        ViewHolder vh = new ViewHolder(myView);
        return vh;
    }


    public interface OnItemClickListener {
        void onItemClick(String key);
    }

    // Replace the contents of a view (invoked by the layout manager)
    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        // - get element from your dataset at this position
        // - replace the contents of the view with that element
        holder.mTitle.setText(sailPlans.get(position).getTitle());
        holder.mText.setText(sailPlans.get(position).getText());
        final int pos = position;
        holder.mCard.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                listener.onItemClick(sailPlans.get(pos).getKey());
            }
        });

    }

    // Return the size of your dataset (invoked by the layout manager)
    @Override
    public int getItemCount() {
        return sailPlans.size();
    }
}