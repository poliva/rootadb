/*
rootadb
(c) 2013 Pau Oliva Fora <pof[at]eslack[dot]org>
 */
package org.eslack.rootadb;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import android.content.ComponentName;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.preference.PreferenceManager;

public class Utils {

	private Context mContext;

	public Utils(Context context) {
		mContext = context;
	}

	public String exec(String command) {
		// execute a shell command, returning output in a string
		try {
			Runtime rt = Runtime.getRuntime();
			Process process = rt.exec("sh");
			DataOutputStream os = new DataOutputStream(
					process.getOutputStream());
			os.writeBytes(command + "\n");
			os.flush();
			os.writeBytes("exit\n");
			os.flush();

			BufferedReader reader = new BufferedReader(new InputStreamReader(
					process.getInputStream()));
			int read;
			char[] buffer = new char[4096];
			StringBuffer output = new StringBuffer();
			while ((read = reader.read(buffer)) > 0) {
				output.append(buffer, 0, read);
			}
			reader.close();

			process.waitFor();

			return output.toString();
		} catch (IOException e) {
			throw new RuntimeException(e);
		} catch (InterruptedException e) {
			throw new RuntimeException(e);
		}
	}

	public void doTheMeat() {

		String filesPath = mContext.getFilesDir().getAbsolutePath();
		String output;

		SharedPreferences prefs = PreferenceManager
				.getDefaultSharedPreferences(mContext);
		boolean debuggable = prefs.getBoolean("debuggable", true);

		try {
			copyFromAssets(mContext, "setpropex", "setpropex");
			exec("chmod 700 " + filesPath + "/setpropex");
		} catch (Exception e) {
			e.printStackTrace();
		}

		output = exec("LD_LIBRARY_PATH=/system/lib getprop ro.secure");
		if (output.equals("0\n")) {
			exec("su -c 'stop adbd'");
			exec("su -c '" + filesPath + "/setpropex ro.secure 1'");
			if (debuggable)
				exec("su -c '" + filesPath + "/setpropex ro.debuggable 0'");
		} else {
			exec("su -c 'stop adbd'");
			exec("su -c '" + filesPath + "/setpropex ro.secure 0'");
			if (debuggable)
				exec("su -c '" + filesPath + "/setpropex ro.debuggable 1'");
		}

		exec("su -c 'start adbd'");
		exec("rm " + filesPath + "/setpropex");
	}

	public static final void copyFromAssets(Context context, String source,
			String destination) throws IOException {

		// read file from the apk
		InputStream is = context.getAssets().open(source);
		int size = is.available();
		byte[] buffer = new byte[size];
		is.read(buffer);
		is.close();

		// write files in app private storage
		FileOutputStream output = context.openFileOutput(destination,
				Context.MODE_PRIVATE);
		output.write(buffer);
		output.close();

	}

	/**
	 * Enables Receiver
	 * 
	 * @param ctx
	 * @param cls
	 *            of the Receiver
	 */
	public static void enableReceiver(final Context ctx, final Class<?> cls) {
		ComponentName receiver = new ComponentName(ctx, cls);
		PackageManager pm = ctx.getPackageManager();
		pm.setComponentEnabledSetting(receiver,
				PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
				PackageManager.DONT_KILL_APP);
	}

	/**
	 * Disables Receiver
	 * 
	 * @param ctx
	 * @param cls
	 *            of the Receiver
	 */
	public static void disableReceiver(final Context ctx, final Class<?> cls) {
		ComponentName receiver = new ComponentName(ctx, cls);
		PackageManager pm = ctx.getPackageManager();
		pm.setComponentEnabledSetting(receiver,
				PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
				PackageManager.DONT_KILL_APP);
	}

}
