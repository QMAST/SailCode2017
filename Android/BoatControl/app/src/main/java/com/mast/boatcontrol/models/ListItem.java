package com.mast.boatcontrol.models;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.mast.boatcontrol.R;

/**
 * TODO: document your custom view class.
 */
public class ListItem extends RelativeLayout {
    private String mTitle = "";
    private String mText = "Unknown";
    public String TAG;
    private View mListItem;
    private TextView tvTitle, tvText;
    private RelativeLayout rlMain;
    public static final int ANIM_DURATION = 200;

    public ListItem(Context context) {
        super(context, null, 0);
        initialize(context, null, 0);
    }

    public ListItem(Context context, AttributeSet attrs) {
        super(context, attrs, 0);
        initialize(context, attrs, 0);
    }

    public ListItem(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initialize(context, attrs, defStyle);
    }

    private void initialize(Context context, AttributeSet attrs, int defStyle) {
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        mListItem = inflater.inflate(R.layout.list_item, this, true);
        tvTitle = (TextView) mListItem.findViewById(R.id.tvTitle);
        tvText = (TextView) mListItem.findViewById(R.id.tvText);
        rlMain = (RelativeLayout) mListItem.findViewById(R.id.rlMain);

        // Load attributes
        final TypedArray a = getContext().obtainStyledAttributes(
                attrs, R.styleable.ListItem, defStyle, 0);

        mTitle = a.getString(
                R.styleable.ListItem_itemTitle);
        if (a.getString(R.styleable.ListItem_itemText) != null) {
            mText = a.getString(
                    R.styleable.ListItem_itemText);
        }

        a.recycle();

        setClickable(true);

        TypedValue outValue = new TypedValue();
        getContext().getTheme().resolveAttribute(android.R.attr.selectableItemBackground, outValue, true);

        // Update View
        invalidateProperties();
    }

    private void invalidateProperties() {
        tvTitle.setText(mTitle);
        tvText.setText(mText);
    }

    /**
     * Use sparingly.
     */
    public void setTitle(String mTitle) {
        this.mTitle = mTitle;
        invalidateProperties();
    }

    public void setText(String mText) {
        this.mText = mText;
        invalidateProperties();
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
