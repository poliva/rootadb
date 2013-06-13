/*
rootadb
(c) 2013 Pau Oliva Fora <pof[at]eslack[dot]org>
 */
package org.eslack.rootadb;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.CheckBox;
import android.widget.TextView;

public class RootAdb extends Activity {

	private TextView outputView;
	private Handler handler = new Handler();
	private Utils mUtils;
	private CheckBox check;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		mUtils = new Utils(getApplicationContext());

		initViews();
	}

	@Override
	protected void onStart() {
		super.onStart();
		updateUi(isAdbRoot());
	}

	private void initViews() {
		outputView = (TextView) findViewById(R.id.outputView);

		check = (CheckBox) findViewById(R.id.checkBox1);

		check.setClickable(false);

		updateUi(isAdbRoot());

		check.setClickable(true);
	}

	private void updateUi(boolean rootadbEnabled) {
		check.setText(rootadbEnabled ? R.string.button_on : R.string.button_off);
		check.setChecked(rootadbEnabled);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.add(Menu.NONE, 0, 0, "Settings");
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case 0:
			startActivity(new Intent(this, SettingsActivity.class));
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	public void onToggleClicked(View view) {
		check.setClickable(false);

		//boolean on = ((CheckBox) view).isChecked();
		boolean on = isAdbRoot();

		if (on) {
			output("\nrestarting adb daemon with shell privileges\n");
		} else {
			output("\nrestarting adb daemon with root privileges\n");
		}

		outputView.setText("");

		Thread thread = new Thread(new Runnable() {
			public void run() {
				mUtils.doTheMeat();
				runOnUiThread(new Runnable() {
     	     	     	     	     public void run() {
					updateUi(isAdbRoot());
    	    	    	    	    }
				});
				check.setClickable(true);
			}
		});
		thread.start();
	}

	private void output(final String str) {
		Runnable proc = new Runnable() {
			public void run() {
				if (str != null)
					outputView.append(str);
			}
		};
		handler.post(proc);
	}

	private boolean isAdbRoot() {
		String output;
		output = mUtils.exec("LD_LIBRARY_PATH=/system/lib getprop ro.secure");
		if (output.equals("0\n")) {
			output("\nadbd is running as root.\n");
			return true;
		} else {
			output("\nadbd is running as shell.\n");
			return false;
		}
	}

}
