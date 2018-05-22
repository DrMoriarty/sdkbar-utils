
package org.cocos2dx.javascript;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import org.cocos2dx.lib.Cocos2dxJavascriptJavaBridge;
import org.cocos2dx.lib.Cocos2dxHelper;
import android.util.Log;

public class NetworkChangeReceiver extends BroadcastReceiver {

	final static String TAG = "NetworkChangeReceiver";
	@Override
	public void onReceive(final Context context, final Intent intent) {
		Cocos2dxHelper.runOnGLThread(new Runnable() {
			@Override
			public void run() {
				try {
					// TODO: Отключено по пречине креша в новой версии
					//Cocos2dxJavascriptJavaBridge.evalString("if('undefined' !== typeof cc && cc.eventManager) { cc.eventManager.dispatchCustomEvent('changenetwork')}");
					//Cocos2dxJavascriptJavaBridge.evalString("cc.log('!!!! test NetworkChangeReceiver')");
					NetworkChangeReceiver.dispatchEvent();
				} catch (Exception ex) {
					ex.printStackTrace();
				}
			}
		});
	}

	public static native int dispatchEvent();
}
