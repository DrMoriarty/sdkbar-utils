package org.cocos2dx.javascript;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;
import android.app.NotificationManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.util.Log;

public class TimeNotification extends BroadcastReceiver {

	private static final int NOTIFY_ID = 101;
	@Override
	public void onReceive(Context context, Intent intent) {
		LocalNotification.show(context,
				intent.getIntExtra("id", NOTIFY_ID),
				intent.getStringExtra("ticker"),
				intent.getStringExtra("title"),
				intent.getStringExtra("text")
		);
	}
}
