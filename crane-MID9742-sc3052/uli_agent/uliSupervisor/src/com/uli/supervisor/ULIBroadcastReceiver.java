/* ULIBroadcastReceiver.java
 * 
 * Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved. 
 *
 */

package com.uli.supervisor;

//import java.io.File;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.util.Log;

//import com.uli.supervisor.R;



public class ULIBroadcastReceiver extends BroadcastReceiver {
	
    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d("UPDATELOGIC", "intent=" + intent);
        
        // FOR DEVELOPMENT ONLY:  don't start the service if the persistent directory doesn't exist. 
        //File updateFile = new File((String) context.getText(R.string.uli_dev_folder));
		//if (!updateFile.exists()){
		//	Log.d("ULIBroadcastReceiver", "WARNING:" + context.getText(R.string.uli_dev_folder) + "doesn't exist");
		//	return;
		//}

        String action = intent.getAction();
        if (action.equals(Intent.ACTION_BOOT_COMPLETED)){
        	//Context.startService();
        	context.startService(new Intent(context, ULIService.class).putExtra("action", Intent.ACTION_BOOT_COMPLETED));
        }
        if (action.equals(Intent.ACTION_SHUTDOWN)){
        	//Context.stopService();
        	context.startService(new Intent(context, ULIService.class).putExtra("action", Intent.ACTION_SHUTDOWN));
        }
        if (action.equals(Intent.ACTION_USER_PRESENT)){        	
        	context.startService(new Intent(context, ULIService.class).putExtra("action", Intent.ACTION_USER_PRESENT));
        }
        if (action.equals(android.net.ConnectivityManager.CONNECTIVITY_ACTION)){
            boolean noConnectivity = intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, false);
        	context.startService(new Intent(context, ULIService.class).putExtra("action", android.net.ConnectivityManager.CONNECTIVITY_ACTION).putExtra("noConnectivity", noConnectivity));
        
        }
        if (action.equals(Intent.ACTION_MEDIA_MOUNTED)){
            context.startService(new Intent(context, ULIService.class).putExtra("action", Intent.ACTION_MEDIA_MOUNTED).putExtra("mediaPath", intent.getDataString()));
	    }          

    }

}
