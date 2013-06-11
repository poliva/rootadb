/*
rootadb
(c) 2013 Pau Oliva Fora <pof[at]eslack[dot]org>
 */
package org.eslack.rootadb;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;

public class SettingsActivity extends PreferenceActivity {

	private CheckBoxPreference mStartOnBootPreference;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.preferences);

		mStartOnBootPreference = (CheckBoxPreference) getPreferenceScreen()
				.findPreference("start_onboot");

		// toggle BootReceiver on/off
		mStartOnBootPreference
				.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {

					@Override
					public boolean onPreferenceChange(Preference preference,
							Object newValue) {

						if (newValue instanceof Boolean) {
							if ((Boolean) newValue) {
								Utils.enableReceiver(getApplicationContext(),
										BootReceiver.class);
							} else {
								Utils.disableReceiver(getApplicationContext(),
										BootReceiver.class);
							}
							return true;
						}
						return false;
					}
				});

	}

}
