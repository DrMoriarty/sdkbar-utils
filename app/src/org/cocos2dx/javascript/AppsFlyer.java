package org.cocos2dx.javascript;

import org.cocos2dx.lib.Cocos2dxActivity;
import android.os.Bundle;

import android.content.IntentFilter;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.appsflyer.AppsFlyerLib;
import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;
import java.util.Map;
import java.util.Iterator;
import java.util.Currency;
import java.math.BigDecimal;

public class AppsFlyer extends Cocos2dxActivity {

    final static String TAG = "AppsFlyer";
    private static Cocos2dxActivity activity;

	public AppsFlyer(Cocos2dxActivity activity, String key) {
        AppsFlyer.activity = activity;
		AppsFlyerLib.getInstance().startTracking(activity.getApplication(), key); // Нужно в настройки
    }

    static public void AppsFlyerAppUserId(String userId) {
        AppsFlyerLib.getInstance().setCustomerUserId(userId);
    }

    static public void AppsFlyerTrackEvent(String event, String params) {
        try {
            JSONObject json = new JSONObject(params);
            Map<String, Object> eventValue = Util.jsonToMap(json);
            AppsFlyerLib.getInstance().trackEvent(activity.getApplicationContext(), event, eventValue);
        } catch (JSONException e) {
            Log.e(TAG, e.toString());
        }
    }
}
