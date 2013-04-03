/*
rootadb
(c) 2013 Pau Oliva Fora <pof[at]eslack[dot]org>
*/
package org.eslack.rootadb;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import org.eslack.rootadb.Utils;

public class BootReceiver extends BroadcastReceiver{

	private Context context;
	private Utils mUtils;

	@Override
	public void onReceive(Context context, Intent intent) {
		mUtils = new Utils(context);
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
		boolean start_onboot = prefs.getBoolean("start_onboot", false);
		if (start_onboot) {
			mUtils.doTheMeat();
		}
	}
}
