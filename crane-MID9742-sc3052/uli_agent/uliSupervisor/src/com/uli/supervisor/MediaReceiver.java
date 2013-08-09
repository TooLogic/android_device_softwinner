/* MediaReceiver.java
 * 
 * Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved. 
 *
 */

package com.uli.supervisor;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;


public class MediaReceiver extends BroadcastReceiver {
 
	    @Override
	    public void onReceive(Context context, Intent intent) {
	        Log.d("UPDATELOGIC", "intent=" + intent);
	        String action = intent.getAction();
            if (action.equals(Intent.ACTION_MEDIA_MOUNTED)){
            		context.startService(new Intent(context, ULIService.class).putExtra("action", Intent.ACTION_MEDIA_MOUNTED).putExtra("mediaPath", intent.getDataString()));
	        }    
	    }	 
	    
}
