/*
rootadb
(c) 2013 Pau Oliva Fora <pof[at]eslack[dot]org>
*/
package org.eslack.rootadb;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.*;
import android.content.Context;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;

public class RootAdb extends Activity {
	
	private TextView outputView;
	private Handler handler = new Handler();
	private Button buttonPm;
	
	private Context context;

	Utils utils = new Utils();

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		outputView = (TextView)findViewById(R.id.outputView);

		buttonPm = (Button)findViewById(R.id.buttonPm);
		buttonPm.setOnClickListener(onButtonPmClick);
		buttonPm.setClickable(false);

		String output;

		output = utils.exec("LD_LIBRARY_PATH=/system/lib getprop ro.secure");
		if (output.equals("0\n")) {
			output("\nadb is already runing as root.\n");
		} else {
			output("\nClick 'ENABLE' to run adb daemon with root privileges.\n");
		}

		buttonPm.setClickable(true);

	}

	private OnClickListener onButtonPmClick = new OnClickListener() {
		public void onClick(View v) {
			// disable button click if it has been clicked once
			buttonPm.setClickable(false);
			outputView.setText("");

			Thread thread = new Thread(new Runnable() {
				public void run() {

					String filesPath = getFilesDir().getAbsolutePath();
					String output;

					try {
						copyFromAssets("setpropex", "setpropex");
						utils.exec("chmod 700 " + filesPath + "/setpropex");
					} catch (Exception e) {
						e.printStackTrace();
					}

					output("restarting adb daemon with root privileges\n");
					utils.exec("su -c 'stop adbd'");
					utils.exec("su -c '" + filesPath + "/setpropex ro.secure 0'");
					utils.exec("su -c 'start adbd'");
					utils.exec("rm " + filesPath + "/setpropex");

					output = utils.exec("LD_LIBRARY_PATH=/system/lib getprop ro.secure");
					if (output.equals("0\n")) {
						output("\nadb daemon is now runing as root.\n");
					} else {
						output("\nsomething failed... :(\ndo you have root?");
					}

					buttonPm.setClickable(true);
				}
			});
			thread.start();
		}
	};

	private void output(final String str) {
		Runnable proc = new Runnable() {
			public void run() {
				if (str!=null) outputView.append(str);
			}
		};
		handler.post(proc);
	}

	private void copyFromAssets(String source, String destination) throws IOException {

		// read file from the apk
		InputStream is = getAssets().open(source);
		int size = is.available();
		byte[] buffer = new byte[size];
		is.read(buffer);
		is.close();

		// write files in app private storage
		FileOutputStream output = openFileOutput(destination, Context.MODE_PRIVATE);
		output.write(buffer);
		output.close();

	}
}
