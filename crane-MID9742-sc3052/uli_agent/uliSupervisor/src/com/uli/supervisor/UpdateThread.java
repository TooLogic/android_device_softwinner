package com.uli.supervisor;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.os.PowerManager;
import android.os.SystemClock;
import android.util.Log;

public class UpdateThread extends Thread {

	Context context;
	Context storedContext;
	Application app = null;
	ULIUpdateNotification updateActivity;
	public static String TAG = "UPDATELOGIC";
	boolean updateReadyToBeInstalled;
	int errorCode;
	
	boolean running = false;
	int endedWithError = 0;
	
	String userMessage;
	private Object lock = new Object();
	
	PowerManager pm;
	PowerManager.WakeLock wl;
	private boolean downloadComplete = false;
	
	// this must be called by the active activity
	public void setActivity(ULIUpdateNotification activity) {
		synchronized (lock) {
			updateActivity = activity;
			context = activity;			
			if (activity != null){
				storedContext = context;
				app = activity.getApplication();
		}
		}
	}
	
	public boolean isRunning() {
		return running;
	}
	
	public int endedWithError() {
		return endedWithError;
	}

	// is there a successful download that happened, perhaps during orientation change
	public boolean isDownloadComplete() {
		return downloadComplete;
	}
	
	@Override
	public void run() {
		int mProgressStatus;
		running = true;
		endedWithError = 0;
		pm = (PowerManager) updateActivity.getSystemService(Context.POWER_SERVICE);
        if(pm != null){
            wl = pm.newWakeLock(  PowerManager.SCREEN_BRIGHT_WAKE_LOCK
                    | PowerManager.ON_AFTER_RELEASE
                    | PowerManager.ACQUIRE_CAUSES_WAKEUP, "ULIUpdateNotification");
            wl.acquire();
        }
        else{
            Log.w(TAG,"Unable to retrieve POWER_SERVICE");
        }
		
		try {
			Log.d(TAG,"onClick upgradeThread run");

			ULIService.UtvDownloadScheduledUpdate();
	
			while ( !updateReadyToBeInstalled && (errorCode == 0) ) {
				SystemClock.sleep(200);
				errorCode = ULIService.UtvProjectGetCurrentUpdateStatus();
				mProgressStatus = ULIService.UtvProjectDownloadPercentComplete();
				//Log.d(TAG, "in thread loop, mProgressStatus ="+mProgressStatus);
	
				sendProgress(mProgressStatus); // send to UI if it exists
				
				updateReadyToBeInstalled = ULIService.UtvIsUpdateReadyToInstall();
				if(updateReadyToBeInstalled){
					sendProgress(100);
				}
			}
	
			/* check if download complete */
			if ( !updateReadyToBeInstalled && (errorCode != 114) ) {
				endedWithError = errorCode;
				reportErrorToActivity(endedWithError);
			}
			else{
				downloadComplete = true;
				sendDownloadComplete();
					}
				Log.d(TAG,"onClick upgradeThread leaving");
			
			ULIService.ignoreAllEvents = false;
			if (updateActivity == null) {
				if (app != null){
					storedContext.startService(new Intent(storedContext, ULIService.class).putExtra("action", "ULI_Bring_Update_Activity_To_Front"));
				}
			}
		} finally {
            if(pm != null){
                wl.release();
            }
			running = false;
		}
	}

	private void sendDownloadComplete() {
		synchronized (lock) {
			if (updateActivity != null) {
				updateActivity.downloadComplete();
			}	
		}
	}

	private void reportErrorToActivity(int error) {
		synchronized (lock) {
			if (updateActivity != null) {
				updateActivity.setInfoText(error);
			}	
		}
	}

	private void sendProgress(int ps) {
		synchronized (lock) {
			if (updateActivity != null) {
				updateActivity.setProgressBar(ps);
			}
			
		}
	}
}


