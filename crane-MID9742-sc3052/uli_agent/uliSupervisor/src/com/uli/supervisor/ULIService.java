/* ULIService.java
 * 
 * Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved. 
 *
 */

package com.uli.supervisor;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.Process;
import android.os.RecoverySystem;
import android.os.RemoteException;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;
import android.net.NetworkInfo;

import com.uli.supervisor.R;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

public class ULIService extends Service {
	private NotificationManager mNM;
	private Intent mInvokeIntent;
	private volatile Looper mServiceLooper;
	private volatile static ServiceHandler mServiceHandler;
	public static NetworkState mNetState;
	private static String[] uInfo = {"","","",""};
	public static String lastMediaUpdatePath;
	public static boolean isInLoopMode = false;
	public static boolean isMandatory = true;
	public static boolean isOOBEDone = false;
	public static boolean isUpdatePostponed = false;
    public static String buildVariant;
	Timer postponeUpdateTimer = null;
	private final IBinder mBinder = new LocalBinder();
	public static boolean ignoreAllEvents = false;
    public static boolean activityVisible = false;
    public static boolean dialogVisible = false;
    static final int MSG_ULI_RETURNED_INFO = 0;
	
	public enum NetworkState {
		UNKNOWN,
		CONNECTED,
		NOT_CONNECTED
	}

	private void checkConnectivity(){
		if (mNetState != NetworkState.CONNECTED)
		{
			ConnectivityManager cm = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
            checkConnectivity(cm);

		}
	}

    public static void checkConnectivity(ConnectivityManager cm){
        if(cm != null){
            if (mNetState != NetworkState.CONNECTED)
            {
                Log.i("UPDATELOGIC", "Network state is unknown");
                NetworkInfo netInfo = cm.getActiveNetworkInfo();
                if (netInfo != null && netInfo.isConnected()) {
                    Log.i("UPDATELOGIC", "Network state: CONNECTED");
                    mNetState = NetworkState.CONNECTED;
                    UtvProjectOnNetworkUp();	
                }
                else{
                    mNetState = NetworkState.NOT_CONNECTED;
                }
            }
        }
        else{
            Log.w("UPDATELOGIC", "ConnectivityManager == null");
            mNetState = NetworkState.UNKNOWN;
        }        
    }

	private final class ServiceHandler extends Handler {
		public ServiceHandler(Looper looper) {
			super(looper);
		}
        
        public void manageNotification(){
            if(!isOOBEDone){
                try {
                    isOOBEDone = (0 != Settings.Secure.getInt(getContentResolver(), Settings.Secure.DEVICE_PROVISIONED));
                } catch (SettingNotFoundException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            isMandatory = !UtvIsCurrentUpdateOptional();
            if(isMandatory){
                isUpdatePostponed = false;
            }
            if(isOOBEDone && !isUpdatePostponed){
                ignoreAllEvents = true;
                dialogVisible = true;
                isInLoopMode = UtvManifestGetLoopTestStatus();
                refreshUpdateInfo();
                showNotification(uInfo[1] + " " + uInfo[0]);
            }        
        }

		@Override
		public void handleMessage(Message msg) {
			Bundle arguments = (Bundle)msg.obj;
			boolean updateNotification = false;

            Log.i("UPDATELOGIC", "#### action = " + arguments.getString("action") + " ####");
            
			// placed before the ignoreAllEvent check because we should always reply to this request
            if(arguments.getString("action").contentEquals("ULI_ULIService_UtvPublicGetAgentInfo")){
				
				String[] wantedInfo = arguments.getStringArray("ULI_Wanted_Info");
				Messenger replyToMessenger = arguments.getParcelable("replyToMessenger");
				String[] infoReturned =	UtvPublicGetAgentInfo(wantedInfo);
				Bundle returnedArguments = new Bundle();
				returnedArguments.putStringArray("ULI_Returned_Info", infoReturned);
				if(replyToMessenger == null){
					Log.e("UPDATELOGIC","No replyToMessenger");
					return;
				}
                try {
                	Message replyMessage = Message.obtain(null,MSG_ULI_RETURNED_INFO, infoReturned.length, 0);
                	replyMessage.setData(returnedArguments);
                	replyToMessenger.send(replyMessage);
                } catch (RemoteException e) {
                	e.printStackTrace();
                }
                return;
			}


			if(arguments.getString("action").contentEquals("ULI_Start_RecoverySystem_Update")){
				hideNotification();
				File updateFile = new File(UtvGetUpdateInstallPath() + "update.zip");
				if (updateFile.exists()){
					try {
						ignoreAllEvents = true;
						UtvProjectOnShutdown();
						Log.i("UPDATELOGIC", "Shutdown complete, calling RecoverySystem.installPackage()" );
						RecoverySystem.installPackage(getApplicationContext(), updateFile); 
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace(); 
						ignoreAllEvents = false;
					}
				}
			}
            
            
            if(ignoreAllEvents){
                if(!activityVisible && !dialogVisible){ //bring the activity to front
                    Intent updaterActivity = new Intent(ULIService.this, ULIUpdateNotification.class);
                    Bundle bundle = new Bundle();
                    bundle.putString("param1", "-fromService");
                    updaterActivity.putExtras(bundle);
                    updaterActivity.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP );
                    startActivity(updaterActivity);
                }
                if(dialogVisible){
                    Log.i("UPDATELOGIC", "#### ignoreAllEvents and bring dialog to front ####");
                    manageNotification();
                }
                else{
                    Log.i("UPDATELOGIC", "#### ignoreAllEvents ####");
                }
                Log.i("UPDATELOGIC", "Done with #" + msg.arg1); 
                return;
                
            }

			if(arguments.getString("action").contentEquals(Intent.ACTION_BOOT_COMPLETED)){
                File updateFile = new File(UtvGetUpdateInstallPath() + "update.zip");
				if (updateFile.exists()){
                    updateFile.delete();
                }
				//UtvProjectOnBoot();
			}

			if(arguments.getString("action").contentEquals(Intent.ACTION_SHUTDOWN)){
				ignoreAllEvents = true;				
				UtvProjectOnShutdown();
			}

			if(arguments.getString("action").contentEquals(Intent.ACTION_MEDIA_MOUNTED)){
				Log.i("UPDATELOGIC", "Media Path =" + arguments.getString("mediaPath") );
                if(arguments.getString("mediaPath").contentEquals("file:///mnt/sdcard")){
                    Log.i("UPDATELOGIC", "ignoring mount event /mnt/sdcard" );
                } else
                {
				lastMediaUpdatePath = arguments.getString("mediaPath").replaceFirst("file://", "");
				updateNotification = UtvProjectMediaUpdateAvailable(lastMediaUpdatePath) > 0;
			}   
			}   

            if(arguments.getString("action").contentEquals("ULI_ULIService_UtvPublicInternetManageRegStore")){
				String[] regStoreStringArray = arguments.getStringArray("ULI_RegStore");
				UtvPublicInternetManageRegStore(regStoreStringArray);
			}

			if(arguments.getString("action").contentEquals(Intent.ACTION_USER_PRESENT)){
				checkConnectivity();
                if (mNetState == NetworkState.CONNECTED){
                    ignoreAllEvents = true;
                    updateNotification = UtvProjectOTAUpdateAvailable() > 0;
                    ignoreAllEvents = false;
			}
			}


			if(arguments.getString("action").contentEquals("ULI_RecoverySystem_Update_Postponed")){
				hideNotification();
				isUpdatePostponed = true;
				postponeUpdateTimer = new Timer("postponeUpdateTimer", true);
				postponeUpdateTimer.schedule(new TimerTask() {
					@Override
					public void run() {
						isUpdatePostponed = false;
					}

				}, 24*3600*1000);
			}

            if(arguments.getString("action").contentEquals("ULI_Bring_Update_Activity_To_Front")){
                if(!activityVisible){
                    Intent updaterActivity = new Intent(ULIService.this, ULIUpdateNotification.class);
                    Bundle bundle = new Bundle();
                    bundle.putString("param1", "-fromService");
                    updaterActivity.putExtras(bundle);
                    updaterActivity.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP );
                    startActivity(updaterActivity);
                }
            }

            if(arguments.getString("action").contentEquals("ULI_ULIService_CheckConnectivity")){
                checkConnectivity();
            }


			if(arguments.getString("action").contentEquals(android.net.ConnectivityManager.CONNECTIVITY_ACTION)){
				if (!arguments.getBoolean("noConnectivity"))
				{
					Log.i("UPDATELOGIC", "Network UP");
					mNetState = NetworkState.CONNECTED;
                    ignoreAllEvents = true;
					UtvProjectOnNetworkUp();
					updateNotification = UtvProjectOTAUpdateAvailable() > 0;
                    ignoreAllEvents = false;
				}
				else{
					mNetState = NetworkState.NOT_CONNECTED;
					Log.i("UPDATELOGIC", "Network DOWN");
				}

			}

			if(updateNotification){
                manageNotification();
			}

			Log.i("UPDATELOGIC", "Done with #" + msg.arg1); 
		}

	};


	@Override
	public void onCreate() {
		super.onCreate();
		// Check the lib version
		String versionString = UtvGetSharedLibVersion();
        String expectedVersionString = getString(R.string.expectedAgentVersionPrefix) + " " + getString(R.string.versionString);
        buildVariant = UtvGetBuildVariant();
        Log.i("UPDATELOGIC", "ULI Shared lib version = " + versionString);
        Log.i("UPDATELOGIC", "ULI Supervisor version = " + expectedVersionString);
        Log.i("UPDATELOGIC", "Build Variant = " + buildVariant);

		mNetState = NetworkState.UNKNOWN;
		mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

		//Toast.makeText(this, R.string.service_created, Toast.LENGTH_SHORT).show();


		mInvokeIntent = new Intent(this, ULIUpdateNotification.class);

		// Start up the thread running the service. 
		HandlerThread thread = new HandlerThread("ULIService",
				Process.THREAD_PRIORITY_BACKGROUND);
		thread.start();

		mServiceLooper = thread.getLooper();
		mServiceHandler = new ServiceHandler(mServiceLooper);

		// register receiver that handles media detection
		/*IntentFilter mediafilter = new IntentFilter(Intent.ACTION_MEDIA_MOUNTED);
		mediafilter.addDataScheme("file");

		BroadcastReceiver mMediaReceiver = new MediaReceiver();
		registerReceiver(mMediaReceiver, mediafilter);
*/

		// the following paths are already set in the native library. 
		// For convenience during development, you can override them with the following functions. 
		//UtvSetPeristentPath(getText(R.string.uli_dev_folder) + "persistent/"); 
		//UtvSetFactoryPath(getText(R.string.uli_dev_folder) + "factory/");
		//UtvSetReadOnlyPath(getText(R.string.uli_dev_folder) + "read-only/");
		//UtvSetUpdateInstallPath(getFilesDir() + "/");

		// Superceded by logic found in UtvCEMInternetGetULPKFileName
        //File UlpkZ = new File(UtvGetReadOnlyPath() +  "ulpk.dat");
        //if ((buildVariant.equals("eng") || buildVariant.equals("user")) && UlpkZ.exists()){
        //    UtvSetFactoryPath(UtvGetReadOnlyPath());
        //}

		// TODO: REMOVE BEFORE PRODUCTION, THE ULPK MUST BE INSERTED BY A FACTORY APPLICATION
		// The code below copies a clear ULPK into the secure store
		String clearULPKFilename = UtvGetFactoryPath() + "ulpk.transit";
		File clearULPKFile = new File(clearULPKFilename);
		if(clearULPKFile.exists())
		{
			Log.i("UPDATELOGIC", "Transit ULPK found: Simulate ULPK provisioning");
			FileInputStream fileinputstream = null; 
			try {
				fileinputstream = new FileInputStream(clearULPKFilename);
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			int numberBytes = 0;
			try {
				numberBytes = fileinputstream.available();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			byte bytearray[] = new byte[numberBytes];
			try {
				fileinputstream.read(bytearray);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			try {
				fileinputstream.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

			if (UtvPlatformInsertULPK(bytearray)== 0)
			{
				//Log.i("UPDATELOGIC", "UtvPlatformInsertULPK OK: deleting the clear ULPK");
				//clearULPKFile.delete();
			}

		}
	}


	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null) {
            return START_STICKY;
        }
		Log.i("UPDATELOGIC", "Starting #" + startId + ": " + intent.getExtras());

		Message msg = getServiceHandler().obtainMessage();
		msg.arg1 = startId;
		msg.arg2 = flags;
		msg.obj = intent.getExtras();
		getServiceHandler().sendMessage(msg);
		Log.i("UPDATELOGIC", "Sending: " + msg);

		return START_STICKY;
	}

	@Override
	public void onDestroy() {
		mServiceLooper.quit();

		hideNotification();

		// Tell the user we stopped.
		//Toast.makeText(ULIService.this, R.string.service_destroyed, Toast.LENGTH_SHORT).show();
	}


	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}




	private void showNotification(String text) {
		// Set the icon, scrolling text and timestamp
		Notification notification = new Notification(R.drawable.stat_sample, text,
				System.currentTimeMillis());

		mInvokeIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK );
		String arguments = "-updateAvailable -fromService";
		if(isInLoopMode){
			arguments += "-looptest";
		}
		if(isMandatory){
			arguments += "-mandatoryUpdate";
		}
		Bundle bundle = new Bundle();
		bundle.putString("param1", arguments);
		mInvokeIntent.putExtras(bundle);

		PendingIntent contentIntent = PendingIntent.getActivity(this, 0, mInvokeIntent, 0);

		notification.setLatestEventInfo(this, getText(R.string.update_notification_string),
				text, contentIntent);

		// Send the notification.
        if(mNM != null)
            mNM.notify(R.string.service_created, notification);


		Intent dialogActivity = new Intent(this, ULINotificationDialog.class);
		dialogActivity.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
		dialogActivity.putExtras(bundle);
		startActivity(dialogActivity);


	}

	private void hideNotification() {
        if(mNM != null)
            mNM.cancel(R.string.service_created);
	}

	static {
		try{
			System.loadLibrary("utv-agent");
		} catch (Throwable ex) {
			Log.e("UPDATELOGIC", "Unable to load libutv-agent.so", ex);
		}
	}

	public class LocalBinder extends Binder {
		ULIService getService() {
			return ULIService.this;
		}
	}

	public native static String	    UtvGetSharedLibVersion();
	public native static String	    UtvGetPrivateLibVersion();
    public native static String	    UtvGetBuildVariant();
	public native static int		UtvDebugSetThreadName(String jName); 
	public native static int		UtvProjectOnDebugInfo();

	public native static int		UtvProjectOnBoot();
	public native static int 		UtvProjectOnNetworkUp();
	public native static int 		UtvProjectOnShutdown();
	public native static int		UtvProjectOnAbort();

	public native static int		UtvProjectOnForceNOCContact();
	public native static String[]	UtvProjectGetUpdateInfo();	
	public native static int 		UtvProjectOTAUpdateAvailable();
	public native static int 		UtvProjectMediaUpdateAvailable(String jPath);	
	public native static boolean 	UtvIsUpdateReadyToInstall();
    public native static int     	UtvSetIsUpdateReadyToInstall(int value);
	public native static int		UtvDownloadScheduledUpdate();
	public native static int		UtvProjectDownloadPercentComplete();


	public native static int		UtvSetUpdateInstallPath(String jName);
	public native static String		UtvGetUpdateInstallPath();
	public native static int		UtvSetPeristentPath(String jName);
	public native static String		UtvGetPeristentPath();
	public native static int		UtvSetFactoryPath(String jName);
	public native static String		UtvGetFactoryPath();
	public native static int		UtvSetReadOnlyPath(String jName);
	public native static String		UtvGetReadOnlyPath();

	public native static int		UtvPublicInternetManageRegStore(String[] stringArray);
	public native static int 		UtvPlatformInsertULPK( byte[] ulpk );

	public native static int 		UtvProjectGetCurrentUpdateStatus();
	public native static String		UtvGetMessageString(int errorCode);
	public native static boolean 	UtvManifestGetLoopTestStatus();
	public native static String[]	UtvPublicGetAgentInfo(String[] stringArray);
	public native static boolean 	UtvIsCurrentUpdateOptional();


	public static ServiceHandler getServiceHandler() {
		return mServiceHandler;
	}

	public static void refreshUpdateInfo(){
		uInfo = UtvProjectGetUpdateInfo();
	}

	public static String getUpdateVersion() {
        if(uInfo[1].contentEquals(""))
			refreshUpdateInfo();
		return uInfo[1];
	}

	public static String getUpdateInfo() {
        if(uInfo[0].contentEquals(""))
			refreshUpdateInfo();
		return uInfo[0];
	}

	public static String getUpdateDecription() {
        if(uInfo[2].contentEquals(""))
			refreshUpdateInfo();
		return uInfo[2];
	}

	public static String getUpdateType() {
        if(uInfo[3].contentEquals(""))
			refreshUpdateInfo();   
		return uInfo[3];
	}
	
	public static String getUpdateNotificationText(Context context){
		//OTA Update available: <Version> <Info>
		return ULIService.getUpdateType() + " " + context.getText(R.string.update_notification_string) + ": " + ULIService.getUpdateVersion()+ " " + ULIService.getUpdateInfo();
	}

	public static String getUpdateNotificationQuestion(Context context){
		//OTA Update available: <Version> <Info> Do you want to download and install it now?
		return getUpdateNotificationText(context) + " " + context.getText(R.string.updateQuestion);
	}
	
	public static String getMandatoryUpdateDialogText(Context context){
		return 	ULIService.getUpdateNotificationText(context) + "\n\n" +
		context.getText(R.string.dialog_mandatory_update_notification_string)+ "\n\n" +
		getUpdateDecription();
	}
	
	public static String getOptionalUpdateDialogText(Context context){
		return 	getUpdateNotificationText(context) + "\n\n" +
				context.getText(R.string.dialog_optional_update_notification_string)+ "\n\n" +
				getUpdateDecription();
	}
	


	
	

}

