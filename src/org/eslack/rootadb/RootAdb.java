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
import android.content.Intent;
import android.view.Menu;
import android.view.MenuItem;


public class RootAdb extends Activity {
	
	private TextView outputView;
	private Handler handler = new Handler();
	private Button buttonPm;
	
        private Context context;
        private Utils mUtils;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		mUtils = new Utils(getApplicationContext());

		outputView = (TextView)findViewById(R.id.outputView);

		buttonPm = (Button)findViewById(R.id.buttonPm);
		buttonPm.setOnClickListener(onButtonPmClick);
		buttonPm.setClickable(false);

		checkAdb();

		buttonPm.setClickable(true);

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
                return false;
        }

	private OnClickListener onButtonPmClick = new OnClickListener() {
		public void onClick(View v) {
			// disable button click if it has been clicked once
			buttonPm.setClickable(false);
			outputView.setText("");

			Thread thread = new Thread(new Runnable() {
				public void run() {
					mUtils.doTheMeat();
					checkAdb();
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

	private void checkAdb() {
		String output;
		output = mUtils.exec("LD_LIBRARY_PATH=/system/lib getprop ro.secure");
		if (output.equals("0\n")) {
			output("\nadbd is running as root.\nClick the button to restart it with shell privileges.\n");
		} else {
			output("\nadbd is running as shell.\nClick the button to restart it with root privileges.\n");
		}
	}

}
