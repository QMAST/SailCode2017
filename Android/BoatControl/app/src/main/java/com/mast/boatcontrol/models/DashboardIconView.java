package com.mast.boatcontrol.models;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.v4.content.ContextCompat;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.mast.boatcontrol.R;

/**
 * Created by Angelo on 2/4/2017.
 */

public class DashboardIconView extends FrameLayout {
    public static final int ANIM_DURATION = 200;
    private String mLabel = ""; // TODO: use a default from R.string...
    private View mIconView;
    private int mainImage;
    private ImageView ivButton;
    private int sPayload = 0;
    private TextView tvLabel, tvShort;
    private RelativeLayout rlButton;

    public DashboardIconView(Context context) {
        super(context, null, 0);
        initialize(context, null, 0);
    }

    public DashboardIconView(Context context, AttributeSet attrs) {
        super(context, attrs, 0);
        initialize(context, attrs, 0);
    }

    public DashboardIconView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize(context, attrs, defStyle);
    }

    private void initialize(Context context, AttributeSet attrs, int defStyle) {
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mIconView = inflater.inflate(R.layout.dashboard_icon, this, true);
        tvLabel = (TextView) mIconView.findViewById(R.id.tvLabel);
        tvShort = (TextView) mIconView.findViewById(R.id.tvButton);
        rlButton = (RelativeLayout) mIconView.findViewById(R.id.rlButton);
        ivButton = (ImageView) mIconView.findViewById(R.id.ivButton);

        // Load attributes
        final TypedArray a = getContext().obtainStyledAttributes(
                attrs, R.styleable.DashboardIconView, defStyle, 0);

        mLabel = a.getString(
                R.styleable.DashboardIconView_cc_label);

        mainImage = a.getResourceId(R.styleable.DashboardIconView_cc_background,0);
        a.recycle();
        rlButton.setBackgroundColor(ContextCompat.getColor(getContext(), R.color.colorWhite));

        setClickable(true);
        TypedValue outValue = new TypedValue();
        getContext().getTheme().resolveAttribute(android.R.attr.selectableItemBackground, outValue, true);

        setForeground(ContextCompat.getDrawable(context, outValue.resourceId));

        // Update View
        invalidateProperties();
    }

    private void invalidateProperties() {
        tvLabel.setText(mLabel);
        if (mainImage != 0) {
            ivButton.setVisibility(VISIBLE);
            ivButton.setImageResource(mainImage);
            ivButton.setColorFilter(ContextCompat.getColor(getContext(), R.color.colorWhite));
            tvShort.setVisibility(GONE);
        }else if (mLabel != null && !mLabel.equals("")) {
            tvShort.setVisibility(VISIBLE);
            tvShort.setText(mLabel.substring(0, 1));
            ivButton.setVisibility(GONE);
        }
    }

    /**
     * Use sparingly.
     */

    public String getmLabel() {
        return mLabel;
    }

    public void setLabel(String mLabel) {
        this.mLabel = mLabel;
        invalidateProperties();
    }

    public void setImage(int ImageResID) {
        mainImage = ImageResID;
        invalidateProperties();
    }

    public int getPayload() {
        return sPayload;
    }

    public void setPayload(int sPayload) {
        this.sPayload = sPayload;
    }

    public void dismiss() {
        dismiss(false);
    }

    public void dismiss(boolean animate) {
        if (!animate) {
            setVisibility(View.GONE);
        } else {
            animate().scaleY(0.1f).alpha(0.1f).setDuration(ANIM_DURATION);
        }
    }

    public void show() {
        setVisibility(View.VISIBLE);
    }
}
