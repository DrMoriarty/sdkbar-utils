
package org.cocos2dx.javascript;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import org.cocos2dx.lib.Cocos2dxJavascriptJavaBridge;
import org.cocos2dx.lib.Cocos2dxHelper;

public class NetworkChangeReceiver extends BroadcastReceiver {
	@Override
	public void onReceive(final Context context, final Intent intent) {
		Cocos2dxHelper.runOnGLThread(new Runnable() {
			@Override
			public void run() {
				try {
					Cocos2dxJavascriptJavaBridge.evalString("if('undefined' !== typeof cc && cc.eventManager) { cc.eventManager.dispatchCustomEvent('changenetwork')}");
				} catch (Exception ex) {
					ex.printStackTrace();
				}
			}
		});
	}
}
