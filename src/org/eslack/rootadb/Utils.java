/*
rootadb
(c) 2013 Pau Oliva Fora <pof[at]eslack[dot]org>
*/
package org.eslack.rootadb;

import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.DataOutputStream;


import android.content.Context;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;


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
			DataOutputStream os = new DataOutputStream(process.getOutputStream()); 
			os.writeBytes(command + "\n");
			os.flush();
			os.writeBytes("exit\n");
			os.flush();

			BufferedReader reader = new BufferedReader(
			new InputStreamReader(process.getInputStream()));
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

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mContext);
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
			if (debuggable) exec("su -c '" + filesPath + "/setpropex ro.debuggable 0'");
		} else {
			exec("su -c 'stop adbd'");
			exec("su -c '" + filesPath + "/setpropex ro.secure 0'");
			if (debuggable) exec("su -c '" + filesPath + "/setpropex ro.debuggable 1'");
		}

		exec("su -c 'start adbd'");
		exec("rm " + filesPath + "/setpropex");
	}

	public static final void copyFromAssets(Context context, String source, String destination) throws IOException {

		// read file from the apk
		InputStream is = context.getAssets().open(source);
		int size = is.available();
		byte[] buffer = new byte[size];
		is.read(buffer);
		is.close();

		// write files in app private storage
		FileOutputStream output = context.openFileOutput(destination, Context.MODE_PRIVATE);
		output.write(buffer);
		output.close();

	}

}
