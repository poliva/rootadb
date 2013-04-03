/*
rootadb
(c) 2013 Pau Oliva Fora <pof[at]eslack[dot]org>
*/
package org.eslack.rootadb;

import org.eslack.rootadb.Utils;

import android.os.Bundle;
import android.preference.PreferenceActivity;

public class SettingsActivity extends PreferenceActivity {
    
    @Override
    public void onCreate(Bundle savedInstanceState) {        
        super.onCreate(savedInstanceState);        
        addPreferencesFromResource(R.xml.preferences);        
    }
    
}

