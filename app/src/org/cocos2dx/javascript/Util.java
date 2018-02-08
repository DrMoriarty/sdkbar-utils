package org.cocos2dx.javascript;

import android.provider.Settings.Secure;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.content.Context;
import android.app.AlertDialog;
import android.content.DialogInterface;
import org.cocos2dx.lib.Cocos2dxJavascriptJavaBridge;
import org.cocos2dx.lib.Cocos2dxHelper;
import android.widget.Toast;
import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import android.app.PendingIntent;
import android.app.AlarmManager;

import android.content.ClipData;
import android.content.ClipboardManager;

import android.content.Intent;
import android.util.Log;

public class Util {

	final static String TAG = "Util";

	public static int TYPE_WIFI = 1;
	public static int TYPE_MOBILE = 2;
	public static int TYPE_NOT_CONNECTED = 0;

	public static AppActivity app;

	public static void init(AppActivity _app) {
		app = _app;
	}

	public static String GetId() {
		String deviceId = Secure.getString(app.getContentResolver(), Secure.ANDROID_ID);

		return deviceId;
	}

	public static String GetPackageName() {
		Context context = app.getApplicationContext();
		String PackageName = context.getPackageName();
		return PackageName;
	}

	public static int getResByName(Context ctx, String aString, String type) {
		return ctx.getResources().getIdentifier(aString, type, ctx.getPackageName());
	}

	public static String getStringResByName(String aString) {
		int resId = app.getResources().getIdentifier(aString, "string", app.getPackageName());
		try {
			return app.getString(resId);
		} catch (Exception e) {
			return "";
		}
	}

	public static Integer getIntResByName(Context ctx, String aString) {
		int resId = ctx.getResources().getIdentifier(aString, "integer", ctx.getPackageName());
		try {
			return ctx.getResources().getInteger(resId);
		} catch (Exception e) {
			return 0;
		}
	}
	
	public static int isNetworkAvailable() {
		ConnectivityManager connectivityManager
				= (ConnectivityManager) app.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo activeNetwork = connectivityManager.getActiveNetworkInfo();

		if (null != activeNetwork) {
			if (activeNetwork.getType() == ConnectivityManager.TYPE_WIFI) {
				return TYPE_WIFI;
			}

			if (activeNetwork.getType() == ConnectivityManager.TYPE_MOBILE) {
				return TYPE_MOBILE;
			}
		}
		return TYPE_NOT_CONNECTED;
	}

	public static void showAlertDialog(final String title, final String message) {
		Cocos2dxJavascriptJavaBridge.evalString("cc.log(\"!!!test!\")");

		//we must use runOnUiThread here
		app.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				AlertDialog alertDialog = new AlertDialog.Builder(app).create();
				alertDialog.setTitle(title);
				alertDialog.setMessage(message);
//				alertDialog.setIcon(R.drawable.icon);
				alertDialog.setButton("OK", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						//we must use runOnGLThread here
						Cocos2dxHelper.runOnGLThread(new Runnable() {
							@Override
							public void run() {
								Cocos2dxJavascriptJavaBridge.evalString("cc.log(\"!!!test2!\")");
							}
						});
					}
				});
				alertDialog.show();
			}
		});
	}

	public static void toast(final String title) {
		Toast toast = Toast.makeText(app.getApplicationContext(),
				title, Toast.LENGTH_SHORT);
		toast.show();
	}
	public static void LocalNotificationCancel(final int id) {
		app.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				LocalNotification.cancel(app.getApplicationContext(), id);
			}
		});
	}

	public static void LocalNotificationAdd(final int id, final String ticker, final String title, final String message, final int seconds) {
		//showAlertDialog(title, message);
		app.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				LocalNotification.add(app.getApplicationContext(), id, ticker, title, message, seconds);
			}
		});
	}

    public static Map<String, Object> jsonToMap(JSONObject json) throws JSONException {
        Map<String, Object> retMap = new HashMap<String, Object>();

        if(json != JSONObject.NULL) {
            retMap = toMap(json);
        }
        return retMap;
    }

    public static Map<String, Object> toMap(JSONObject object) throws JSONException {
        Map<String, Object> map = new HashMap<String, Object>();

        Iterator<String> keysItr = object.keys();
        while(keysItr.hasNext()) {
            String key = keysItr.next();
            Object value = object.get(key);

            if(value instanceof JSONArray) {
                value = toList((JSONArray) value);
            }

            else if(value instanceof JSONObject) {
                value = toMap((JSONObject) value);
            }
            map.put(key, value);
        }
        return map;
    }

    public static List<Object> toList(JSONArray array) throws JSONException {
        List<Object> list = new ArrayList<Object>();
        for(int i = 0; i < array.length(); i++) {
            Object value = array.get(i);
            if(value instanceof JSONArray) {
                value = toList((JSONArray) value);
            }

            else if(value instanceof JSONObject) {
                value = toMap((JSONObject) value);
            }
            list.add(value);
        }
        return list;
	}

	public static void restart() {
		try {
			Context context = app.getApplicationContext();
			Log.i(TAG, "restarting app " + context.getPackageName());
			Intent restartIntent = context.getPackageManager()
					.getLaunchIntentForPackage(context.getPackageName());
			PendingIntent intent = PendingIntent.getActivity(
					context, 0,
					restartIntent, Intent.FLAG_ACTIVITY_CLEAR_TOP);
			AlarmManager manager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
			manager.set(AlarmManager.RTC, System.currentTimeMillis() + 100, intent);
			app.finish();
			System.exit(0);
		} catch (Exception e) {
			e.printStackTrace();
			//Log.e(TAG, )
		}
	}

	/**
	 * Поместить в буфер обмена
	 *
	 * @param text
	 */
	public static void clipboard(final String text) {
		//we must use runOnUiThread here
		app.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				ClipboardManager clipboardManager = (ClipboardManager) app.getSystemService(Context.CLIPBOARD_SERVICE);
				clipboardManager.setPrimaryClip(ClipData.newPlainText("text", text));
			}
		});
	}
}
