package org.cocos2dx.javascript;

import org.cocos2dx.lib.Cocos2dxActivity;
import org.cocos2dx.lib.Cocos2dxGLSurfaceView;
import android.os.Bundle;

//import android.support.v4.content.LocalBroadcastManager;
import android.content.IntentFilter;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
//import android.widget.Toast;
import android.util.Log;

import io.fabric.sdk.android.Fabric;
import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.ndk.CrashlyticsNdk;
import com.crashlytics.android.answers.Answers;
import com.crashlytics.android.answers.AnswersEvent;
import com.crashlytics.android.answers.LoginEvent;
import com.crashlytics.android.answers.CustomEvent;
import com.crashlytics.android.answers.PurchaseEvent;
import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;
import java.util.Map;
import java.util.Iterator;
import java.util.Currency;
import java.math.BigDecimal;

public class FabricBridge {
    final static String TAG = "FabricBridge";
    private static Cocos2dxActivity activity;

    public FabricBridge(Cocos2dxActivity activity) {
        FabricBridge.activity = activity;
        //Fabric.with(activity.getApplicationContext(), new Crashlytics(), new Answers(), new CrashlyticsNdk());
        final Fabric fabric = new Fabric.Builder(activity.getApplicationContext())
            .kits(new Crashlytics(), new CrashlyticsNdk(), new Answers())
            .debuggable(true)
            .build();
        Fabric.with(fabric);
    }

    static public void CrashlyticsLog(final String text) {
        Log.i(TAG, "Crashlytics log:"+text);
        Crashlytics.log(text);
    }

    static public void CrashlyticsSetString(final String key, final String value) {
        Log.i(TAG, "Crashlytics set string:"+key);
        Crashlytics.setString(key, value);
    }

    static public void CrashlyticsSetBool(final String key, boolean value) {
        Log.i(TAG, "Crashlytics set bool:"+key);
        Crashlytics.setBool(key, value);
    }

    static public void CrashlyticsSetInt(final String key, int value) {
        Log.i(TAG, "Crashlytics set int:"+key);
        Crashlytics.setInt(key, value);
    }

    static public void CrashlyticsSetFloat(final String key, float value) {
        Log.i(TAG, "Crashlytics set float:"+key);
        Crashlytics.setFloat(key, value);
    }

    static public void AnswersSendLogIn(final String params) throws JSONException {
        final JSONArray data = new JSONArray(params);
        FabricBridge.activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                LoginEvent evt = new LoginEvent();

                evt.putMethod(data.optString(0, "Direct"));
                evt.putSuccess(data.optBoolean(1, true));

                if (!data.isNull(2)) {
                    populateCustomAttributes(evt, data.optJSONObject(2));
                }

                Answers.getInstance().logLogin(evt);
            }
        });
    }

    static public void AnswersSendCustomEvent(final String event, final String params) {
        FabricBridge.activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                CustomEvent evt = new CustomEvent(event);

                try {
                    if (params != null) {
                        populateCustomAttributes(evt, new JSONObject(params));
                    }
                } catch (JSONException e) {
                    Log.e(TAG, e.toString());
                }

                Answers.getInstance().logCustom(evt);
            }
        });
    }

    static public void AnswersSendPurchase(final String params) throws JSONException {
        final JSONArray data = new JSONArray(params);
        FabricBridge.activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                PurchaseEvent evt = new PurchaseEvent();

                if (!data.isNull(0)) {
                    evt.putItemPrice(new BigDecimal(data.optDouble(0, 0)));
                }

                try {
                    evt.putCurrency(Currency.getInstance(data.optString(1)));
                }
                catch (Exception ex) {
                    Log.w(TAG, "Unable to parse currency: " + ex.getMessage());
                }

                evt.putSuccess(data.optBoolean(2, true));
                evt.putItemName(data.optString(3));
                evt.putItemType(data.optString(4));
                evt.putItemId(data.optString(5));

                if (!data.isNull(6)) {
                    populateCustomAttributes(evt, data.optJSONObject(5));
                }

                Answers.getInstance().logPurchase(evt);
            }
        });
    }

    static private void populateCustomAttributes(AnswersEvent evt, JSONObject attributes) {

        if (attributes == null || evt == null) {
            return;
        }

        try {
            Iterator<String> keys = attributes.keys();
            while (keys.hasNext()) {
                String key = keys.next();

                try {
                    evt.putCustomAttribute(key, attributes.getString(key));
                }
                catch (Exception e) {
                    Log.w(TAG, "Error while populating custom attribute with key '" + key + "': " + e.getMessage());
                }
            }
        }
        catch (Exception ex) {
            Log.w(TAG, "Error while populating custom attributes: " + ex.getMessage());
        }
    }
}
