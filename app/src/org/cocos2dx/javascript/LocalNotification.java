package org.cocos2dx.javascript;

import android.content.Intent;
import android.content.Context;
import android.app.NotificationManager;
import android.app.Notification;
import android.content.res.Resources;
import android.graphics.BitmapFactory;
import android.app.PendingIntent;
import android.app.AlarmManager;
import android.app.PendingIntent;
import java.util.Calendar;

public class LocalNotification {
	static public void cancel(Context context, final int id) {
		AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
		Intent intent = new Intent(context, org.cocos2dx.javascript.TimeNotification.class);
		PendingIntent pendingIntent = PendingIntent.getBroadcast(context, id, intent, PendingIntent.FLAG_CANCEL_CURRENT);
// На случай, если мы ранее запускали активити, а потом поменяли время,
// откажемся от уведомления
		am.cancel(pendingIntent);
	}
	static public void add(Context context, final int id, final String ticker, final String title, final String text, final int seconds) {
		AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
		Intent intent = new Intent(context, org.cocos2dx.javascript.TimeNotification.class);
		intent.putExtra("id", id);
		intent.putExtra("ticker", ticker);
		intent.putExtra("title", title);
		intent.putExtra("text", text);

		PendingIntent pendingIntent = PendingIntent.getBroadcast(context, id, intent, PendingIntent.FLAG_CANCEL_CURRENT);
// На случай, если мы ранее запускали активити, а потом поменяли время,
// откажемся от уведомления
		am.cancel(pendingIntent);
// Устанавливаем разовое напоминание

		Calendar cal = Calendar.getInstance();
		cal.setTimeInMillis(System.currentTimeMillis());
		cal.add(Calendar.SECOND, seconds);

		am.set(AlarmManager.RTC_WAKEUP, cal.getTimeInMillis(), pendingIntent);
	}

	static public void show(Context context, int id, final String ticker, final String title, final String text) {
		Intent notificationIntent = new Intent(context, AppActivity.class);
		PendingIntent contentIntent = PendingIntent.getActivity(context,
				0, notificationIntent,
				PendingIntent.FLAG_CANCEL_CURRENT);

		Resources res = context.getResources();

		Notification.Builder builder = new Notification.Builder(context);

		builder.setContentIntent(contentIntent)
				.setSmallIcon(Util.getResByName(context, "smallicon", "drawable"))
				.setLargeIcon(BitmapFactory.decodeResource(res, Util.getResByName(context, "icon", "drawable")))						// большая картинка
				//.setLargeIcon(BitmapFactory.decodeResource(res, R.drawable.hungrycat))
				.setTicker(ticker) // текст в строке состояния
				.setWhen(System.currentTimeMillis())
				.setAutoCancel(true)
				.setContentTitle(title)
				.setContentText(text); // Текст уведомления

		Notification notification = builder.getNotification(); // до API 16
		//Notification notification = builder.build();

		NotificationManager notificationManager = (NotificationManager) context
				.getSystemService(Context.NOTIFICATION_SERVICE);
		notificationManager.notify(id, notification);
	}
}