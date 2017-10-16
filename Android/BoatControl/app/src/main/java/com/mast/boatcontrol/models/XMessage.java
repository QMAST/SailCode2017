package com.mast.boatcontrol.models;

import android.util.Log;

/**
 * Created by Angelo on 4/1/2017.
 * Class for wrapping data messages to and from the boat
 */

public class XMessage {
    public static final int OUTBOUND = 0;
    public static final int INBOUND = 1;
    private int type; // 0 = outbound transmission, 1 = inbound transmission
    private String data; // Contains message portion of transmission
    private int data_code; // Indicates subject of transmission
    private int response_code; // Indicates success/error for inbound transmissions in response to an outgoing transmission
    private String TAG = "XMessage.java";
    public boolean verified;

    // Constructor for outbound transmissions
    public XMessage(int data_code, String data) {
        if (data != null) {
            this.data = data;
        } else {
            this.data = "";
        }
        this.data_code = data_code;
        type = OUTBOUND;
    }

    // Constructor for inbound transmissions
    public XMessage(String transmission_string) {
        String sTransArray[] = transmission_string.replace(";", "").split(",");
        if (sTransArray.length > 1) {
            try {
                data_code = Integer.parseInt(sTransArray[0]);
                response_code = Integer.parseInt(sTransArray[1]);
                if (sTransArray.length > 3) {
                    data = sTransArray[2];
                    for (int i = 3; i < sTransArray.length; i++) {
                        data = data + "," + sTransArray[i];
                    }
                } else if (sTransArray.length == 3) {
                    data = sTransArray[2];
                } else if (sTransArray.length == 2) {
                    data = "";
                }
            } catch (NumberFormatException e) {
                Log.e(TAG, "Transmission corrupted. Number Format Exception. Received: " + transmission_string);
                data_code = 501;
            }
        } else if (transmission_string.equals("501")) {
            data_code = 501;
        } else {
            Log.e(TAG, "Transmission corrupted. Received: " + transmission_string);
        }
        type = INBOUND;
    }

    @Override
    public String toString() {
        if (type == OUTBOUND) {
            return data_code + "," + data + ";";
        } else if (type == INBOUND) {
            return data_code + "," + response_code + "," + data + ";";
        } else {
            return null;
        }
    }

    public String getData() {
        return data;
    }

    public int getData_code() {
        return data_code;
    }

    public int getResponse_code() {
        return response_code;
    }
}
