/* ULIUpdateNotification.java
 * 
 * Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved. 
 *
 */

package com.uli.supervisor;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.PowerManager;
import android.os.StatFs;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.view.WindowManager;
import android.net.ConnectivityManager;
import android.app.Dialog;
import java.io.*;


public class ULIUpdateNotification extends Activity {
	private ProgressBar mDownloadProgress;
	private Button buttonNo;
	private Button buttonYes;
	private Button buttonCancel;
	private TextView mProgressTextView;
	private TextView mInfoTextView;
	private TextView mDescriptionTextView;
	private static boolean updateAvailable = false;
	private static boolean isMandatory = false;
	private static boolean isLoopTest = false;
    private static boolean isUpdateAvailableDialogDisplayed = false;
    private boolean startedFromMenu = false;
	private String param1 = "";
	private ViewGroup progressFrame;
	private static AsyncTask<String, Integer, Integer> checkTask = null; 
	public static String TAG = ULIUpdateNotification.class.getSimpleName();
	private static UpdateThread updateThread;
	private static int errorCode = 0;
	static final int DIALOG_DOWNLOAD_ENDED_ID = 0;
	static final int DIALOG_DISPLAY_INFO_ID = 1;
	static final int DIALOG_UPDATE_AVAILABLE_ID = 2;
	static final int DIALOG_UPDATE_DISAPPEARED_ID = 3;

	@Override
	protected Dialog onCreateDialog(int id) {
		Dialog dialog;
		AlertDialog.Builder builder = new AlertDialog.Builder(ULIUpdateNotification.this);
        //builder.setUseZephyrStyle(true);
		switch(id) {
		case DIALOG_DOWNLOAD_ENDED_ID:
			builder.setMessage(getText(R.string.before_restart_text))
			.setCancelable(false)
			.setPositiveButton(getText(R.string.ok), new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
					ULIService.ignoreAllEvents = false;
					File updateFile = new File(ULIService.UtvGetUpdateInstallPath() + "update.zip");
					if (!updateFile.exists()){
						dialog.cancel();
						displayNoUpdateZipDialog();
					}
					else{
						startService(new Intent(ULIUpdateNotification.this, ULIService.class).putExtra("action", "ULI_Start_RecoverySystem_Update"));
						dialog.cancel();
					}
				}
			});
			dialog = builder.create();  
			break;
		case DIALOG_DISPLAY_INFO_ID:
			builder.setMessage(getFullDeviceInfo())
			.setCancelable(false)
			.setPositiveButton(getText(R.string.ok), new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
					dialog.cancel();
				}
			});
			dialog = builder.create(); 
			break;
		case DIALOG_UPDATE_AVAILABLE_ID:
            isUpdateAvailableDialogDisplayed = true;
			builder.setCancelable(false);
			DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
                    isUpdateAvailableDialogDisplayed = false;
					dialog.cancel();
					displayUpdateInfo();
					buttonYes.setVisibility(View.VISIBLE);
					buttonNo.setVisibility(View.VISIBLE);
					//if (isMandatory || isLoopTest){
						buttonYes.performClick();
					//}
					}
			};

			DialogInterface.OnClickListener askMeLaterListener = new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
                    isUpdateAvailableDialogDisplayed = false;
                    ULIService.ignoreAllEvents = false;
					startService(new Intent(ULIUpdateNotification.this, ULIService.class).putExtra("action", "ULI_RecoverySystem_Update_Postponed"));
					dialog.cancel();
					finish();
				}
			};
			
			String dialogMessage;
			if(isMandatory){
				dialogMessage = ULIService.getMandatoryUpdateDialogText(this);
				builder.setPositiveButton(getText(R.string.ok), okListener);	
			}
			else{
				dialogMessage = ULIService.getOptionalUpdateDialogText(this);
				builder.setPositiveButton(getText(R.string.update_now), okListener);
				builder.setNegativeButton(getText(R.string.update_later), askMeLaterListener);
			}
			builder.setMessage(dialogMessage);
			
			dialog = builder.create(); 
			break;
		case DIALOG_UPDATE_DISAPPEARED_ID:
			builder.setMessage(getText(R.string.update_zip_disappeared_text))
			.setCancelable(false)
			.setPositiveButton(getText(R.string.ok), new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
					ULIService.UtvSetIsUpdateReadyToInstall(0);
					updateThread = null;
					finish();
				}
			});
			dialog = builder.create();  
			break;
		default:
			dialog = null;
		}
		return dialog;
	}
	
	@Override
	protected void onPrepareDialog(final int id, final Dialog dialog) {
		AlertDialog ad = ((AlertDialog) dialog);
		switch (id) {
		case DIALOG_UPDATE_AVAILABLE_ID:
            isUpdateAvailableDialogDisplayed = true;
			DialogInterface.OnClickListener okListener = new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
                    isUpdateAvailableDialogDisplayed = false;
					dialog.cancel();
					displayUpdateInfo();
					buttonYes.setVisibility(View.VISIBLE);
					buttonNo.setVisibility(View.VISIBLE);
					//if (isMandatory || isLoopTest){
						buttonYes.performClick();
					//}
				}
			};

			DialogInterface.OnClickListener askMeLaterListener = new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int id) {
                    isUpdateAvailableDialogDisplayed = false;
                    ULIService.ignoreAllEvents = false;
					startService(new Intent(ULIUpdateNotification.this, ULIService.class).putExtra("action", "ULI_RecoverySystem_Update_Postponed"));
					dialog.cancel();
					finish();
				}
			};
			
			String dialogMessage;
			if(isMandatory){
				dialogMessage = ULIService.getMandatoryUpdateDialogText(this);
				ad.setButton(DialogInterface.BUTTON_POSITIVE,getText(R.string.ok), okListener);
			}
			else{
				dialogMessage = ULIService.getOptionalUpdateDialogText(this);
				ad.setButton(DialogInterface.BUTTON_POSITIVE,getText(R.string.update_now), okListener);
				ad.setButton(DialogInterface.BUTTON_NEGATIVE,getText(R.string.update_later), askMeLaterListener);
			}
			ad.setMessage(dialogMessage);
			break;
		case DIALOG_DISPLAY_INFO_ID:
			ad.setMessage(getFullDeviceInfo());
			break;
		}
	}

	private String getFullDeviceInfo(){
		String[] infoWanted = {"ESN","UID","TSR","QH","VER","INF","OUI","MG","HM","SV","MV","NP","REG","LP","EC","LE","LES","LET"};
		String[] infoReturned =	ULIService.UtvPublicGetAgentInfo(infoWanted);
		String infoText = "";
		infoText += "Service : " + getString(R.string.versionString) + "\n";
		infoText += "Library : " + ULIService.UtvGetSharedLibVersion() + "\n";
        infoText += "Private Library : " + ULIService.UtvGetPrivateLibVersion() + "\n";
		infoText += "Build Variant : " + ULIService.UtvGetBuildVariant()+ "\n";
		for (int i=0;i<infoWanted.length;i++)
		{
			infoText += infoWanted[i] + " : " + infoReturned[i] + "\n";
		}
		// no longer useful for displaying with ULPK path is active because it's 
		// context specific.
		// infoText += "Factory path : " + ULIService.UtvGetFactoryPath()+ "\n";
		// infoText += "Persistent path : " + ULIService.UtvGetPeristentPath()+ "\n";
		// infoText += "Read-only path : " + ULIService.UtvGetReadOnlyPath()+ "\n";
		// infoText += "Install path : " + ULIService.UtvGetUpdateInstallPath()+ "\n";
		return infoText;
	}

	@Override
	protected void onDestroy() {
		Log.d("UPDATELOGIC",TAG + "onDestroy");
		super.onDestroy();
	}
	
	private void displayUpdateInfo() {
		mInfoTextView.setText(ULIService.getUpdateNotificationQuestion(ULIUpdateNotification.this));
		if(isMandatory)
			mDescriptionTextView.setText(getText(R.string.while_downloading_mandatory_info_text));
		else
			mDescriptionTextView.setText(getText(R.string.while_downloading_optional_info_text));
			
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.d("UPDATELOGIC",TAG + "onCreate");
		//setTheme(com.android.internal.R.style.Theme_ZephyrScreen);
		super.onCreate(savedInstanceState);

		setContentView(R.layout.uli_update_notification);
		
		Bundle bundle = this.getIntent().getExtras();
		if(bundle != null){
			param1 = bundle.getString("param1");
		}

        // if this parameter is not present we know that the activity was started from the menu
        startedFromMenu = !param1.contains("-fromService");

        if(param1.contains("-updateAvailable")){
            updateAvailable = true;
            if(param1.contains("-mandatoryUpdate"))
                isMandatory = true;
            if(param1.contains("-looptest"))
                isLoopTest = true;
		}
	}
	
    @Override
    protected void onStart() {
        ULIService.activityVisible = true;
        super.onStart();
        Log.d("UPDATELOGIC",TAG + "onStart");
		initControls();

		State state = (State) getLastNonConfigurationInstance(); // on Orientation change, grab any inflight background task
		if (state != null) {
			updateThread = state.updateThread;
			checkTask = state.checkTask;
			errorCode = state.errorCode;
			isMandatory = state.isMandatory;
            updateAvailable = state.updateAvailable;
            isUpdateAvailableDialogDisplayed = state.isUpdateAvailableDialogDisplayed;
		}
        
        // Even if ther's an update available we recheck when the activity is started from the menu
        if(startedFromMenu){
            updateAvailable = false;
		}
		
		if (updateThread != null) {
			// still downloading, we will receive an event when it is done downloading

			if (updateThread.isDownloadComplete()) {
				showDownloadProgressView();
				setProgressBar(100);
				downloadComplete();
				return; // bail and show the dialog to the user for download complete
			} else if (updateThread.isRunning()) {
				updateThread.setActivity(this); // register to receive progress updates from a running background download
				// we should show progress in this case, we don't need to check, we are already downloading something
				displayUpdateInfo();
				showDownloadProgressView();
				return; // make sure progress view is up, show updates from the download process
			} else {
				errorCode = updateThread.endedWithError();
                if(errorCode == 9){//aborted by the user
                    errorCode = 0;
                }
				// TODO:  Add error checking after orientation change by using values from errCode in the update thread and display in this case
				updateThread = null; // no need to hold this reference, it must have failed while the user was not looking at the UI
			}
		}
		
		if((errorCode != 0))
		{
			setInfoText(errorCode);
			return;
		}
		
        if(ULIService.mNetState != ULIService.NetworkState.CONNECTED){
				ConnectivityManager cm = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
				ULIService.checkConnectivity(cm);
        }
		if((ULIService.mNetState != ULIService.NetworkState.CONNECTED) && !updateAvailable){
        	mInfoTextView.setText(getText(R.string.connection_unavailable));
			buttonNo.setText(getText(R.string.ok));
			buttonNo.setVisibility(View.VISIBLE);
            buttonNo.requestFocus();
            return;
        }
		

		
		if(updateAvailable){
			displayUpdateInfo();
            if(isUpdateAvailableDialogDisplayed){
                showDialog(DIALOG_UPDATE_AVAILABLE_ID);
            }
            else
            {
			buttonYes.setVisibility(View.VISIBLE);
			buttonNo.setVisibility(View.VISIBLE);

			//if (isLoopTest|| isMandatory){
				buttonYes.performClick();
			//}
			}
            
		} else {
            if(startedFromMenu){
                // add the "-fromService" paramter to avoid to have "checkForUpdateInBackground" during orientation change
                Bundle bundle = new Bundle();
                bundle.putString("param1", "-fromService");
                this.getIntent().putExtras(bundle);
			checkForUpdateInBackground();
		}
    }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (updateThread != null) 
            updateThread.setActivity(ULIUpdateNotification.this);
        Log.d("UPDATELOGIC",TAG + "onResume");
	}
	
	private void initControls() {
		// Watch for button clicks.
		buttonYes = (Button)findViewById(R.id.yes);
		buttonYes.setOnClickListener(mYesListener);
		buttonYes.setVisibility(View.GONE);

		buttonNo = (Button)findViewById(R.id.no);
		buttonNo.setOnClickListener(mNoListener);
		buttonNo.setVisibility(View.GONE);

		buttonCancel = (Button)findViewById(R.id.cancel);
		buttonCancel.setOnClickListener(mCancelListener);

		buttonCancel.setVisibility(View.GONE);

		progressFrame = (ViewGroup) findViewById(R.id.progressFrame);
		progressFrame.setVisibility(View.GONE);

		mDownloadProgress = (ProgressBar) findViewById(R.id.DownloadProgressBar);
		mDownloadProgress.setVisibility(View.GONE);

		mProgressTextView = (TextView) findViewById(R.id.ProgressTextView);

		mInfoTextView = (TextView) findViewById(R.id.InfoTextView);
		mDescriptionTextView = (TextView) findViewById(R.id.DescriptionTextView);

	}
	
	// background task that checks for a ULI update
	private void checkForUpdateInBackground() {
		
		AsyncTask<String, Integer, Integer> task = new AsyncTask<String, Integer, Integer>() {
			@Override
			protected void onPreExecute() {
				ULIService.ignoreAllEvents = true;
				mInfoTextView.setText(getText(R.string.checking_for_update_text));
				mInfoTextView.invalidate();
				mProgressTextView.setVisibility(View.GONE);
				progressFrame.setVisibility(View.VISIBLE);
				mDownloadProgress.setVisibility(View.VISIBLE);
				mDownloadProgress.setIndeterminate(true);
				mDownloadProgress.invalidate();
                buttonCancel.setVisibility(View.VISIBLE);
			}

			@Override
			protected Integer doInBackground(String... params) {
				ULIService.UtvProjectOnForceNOCContact();
				updateAvailable = ULIService.UtvProjectOTAUpdateAvailable() > 0;
				isMandatory = !ULIService.UtvIsCurrentUpdateOptional();
				isLoopTest = ULIService.UtvManifestGetLoopTestStatus();
				return null;
			}
			
			// runs in UIThread for us
			@Override
			protected void onPostExecute(Integer result) {
				ULIService.ignoreAllEvents = false;
				checkTask = null;
				progressFrame.setVisibility(View.GONE);
				mDownloadProgress.setVisibility(View.GONE);
				buttonCancel.setVisibility(View.GONE);
				mDownloadProgress.setIndeterminate(false);
				mProgressTextView.setVisibility(View.VISIBLE);
				if (updateAvailable){
                    ULIService.ignoreAllEvents = true;
					ULIService.refreshUpdateInfo();
					if(isLoopTest){
                        displayUpdateInfo();
                            buttonYes.performClick();
                        }
					else{
					showDialog(DIALOG_UPDATE_AVAILABLE_ID);
					}
				} else {
					int returnedCode = ULIService.UtvProjectGetCurrentUpdateStatus();
                    setInfoText(returnedCode);
					//String returnedMessage = ULIService.UtvGetMessageString(returnedCode);
					//mInfoTextView.setText(returnedMessage);
					//buttonNo.setText(getText(R.string.ok));
					//buttonNo.setVisibility(View.VISIBLE);
				}
				
			}
		};
		
		task.execute(new String[0]);
		checkTask = task; // record state
	}
	

	@Override
	public Object onRetainNonConfigurationInstance() {
		// pass the active updateThread to another Activity instance during orientation changes
		State state = new State();
		state.updateThread = updateThread;
		state.checkTask = checkTask;
		state.errorCode = errorCode;
		state.isMandatory = isMandatory;
        state.updateAvailable = updateAvailable;
        state.isUpdateAvailableDialogDisplayed = isUpdateAvailableDialogDisplayed;
		return state;
	}

	@Override
    protected void onPause() {
        Log.d("UPDATELOGIC",TAG + "onPause");
        ULIService.activityVisible = false;
        super.onPause();
		if (updateThread != null) {
			updateThread.setActivity(null); // make sure it doesn't hold a references to this activity
		}
	}

	@Override
	protected void onStop() {
        Log.d("UPDATELOGIC",TAG + "onStop");
		super.onStop();
	}

	private OnClickListener mYesListener = new OnClickListener() {
		public void onClick(View v) {
            if(updateAvailable){
			ULIService.ignoreAllEvents = true;
			int currentUpdateTotalSize;
            
			//clean the cache if needed
			String[] infoWanted = {"currentUpdateTotalSize"};
			String[] infoReturned =	ULIService.UtvPublicGetAgentInfo(infoWanted);
            try {
                currentUpdateTotalSize = Integer.parseInt(infoReturned[0]);
            } catch (NumberFormatException e) {
                // This should not happen if the lib is up to date: set a default size just in case the lib doesn't return the size
                currentUpdateTotalSize = 252551744;
            }
			Log.i("UPDATELOGIC","Cache Free Size " + GetCacheFreeSize());
			Log.i("UPDATELOGIC","Update Size " + infoReturned[0] + " in value = " + currentUpdateTotalSize);
			int fileAge = 24;//hours
			while ((GetCacheFreeSize() < (currentUpdateTotalSize + 1024)) &&( fileAge >= 0)){
				ClearOldFilesFromCache(fileAge*3600*1000,false);
				Log.i("UPDATELOGIC","Deleted " + fileAge + " hours old files from cache");
				Log.i("UPDATELOGIC","Cache Free Size " + GetCacheFreeSize());
				fileAge--;
			}
            
            if(GetCacheFreeSize() < (currentUpdateTotalSize + 1024)){
                Log.w("UPDATELOGIC","The Cache partition is too small. Using /mnt/sdcard/ instead");
                ULIService.UtvSetUpdateInstallPath("/mnt/sdcard/");
            }
			else{
                // check if the cache sub dir exists and create it if it's not the case
                File cacheInstallFolder = new File(ULIService.UtvGetUpdateInstallPath());           
                if(!cacheInstallFolder.exists())
                {
                    cacheInstallFolder.mkdir();
                }
                //just to avoid having an empty directory.
                File noEmptyDir = new File(cacheInstallFolder.getPath() + "/uli.txt");
                if(!noEmptyDir.exists())
                {
                    try { 
                        FileWriter outFile = new FileWriter(noEmptyDir.getPath());
                        PrintWriter out = new PrintWriter(outFile);
                        out.println("Please don't erase this file!");
                        out.close();
                    } catch (Exception e){
                        e.printStackTrace();
                    }
                }     
            }
            displayUpdateInfo();
			errorCode = 0;
			showDownloadProgressView();
			startUpdateThread();
		}
            else{
                buttonYes.setVisibility(View.GONE);
                buttonNo.setVisibility(View.GONE);
                mDescriptionTextView.setText("");
                errorCode = 0;
                checkForUpdateInBackground();
            }
            
		}
	}; 
	
	
	private void showDownloadProgressView() {
		buttonNo.setEnabled(false);
		buttonYes.setEnabled(false);
		progressFrame.setVisibility(View.VISIBLE);
		mDownloadProgress.setVisibility(View.VISIBLE);
		if(!isMandatory)
            buttonCancel.setVisibility(View.VISIBLE);
		buttonNo.setVisibility(View.GONE);
		buttonYes.setVisibility(View.GONE);
		mInfoTextView.setText(getText(R.string.downloading) + " " + ULIService.getUpdateVersion() );
		int mProgressStatus = 0;
		mDownloadProgress.setProgress(mProgressStatus);
		String per = "" + mProgressStatus + " %";
		mProgressTextView.setText(per);
	}
	
	
	

	private void startUpdateThread() {
		if (updateThread == null || !updateThread.isRunning()) {
			updateThread = new UpdateThread();
			updateThread.setActivity(ULIUpdateNotification.this);
			updateThread.start();
		}
	}
	
	private OnClickListener mNoListener = new OnClickListener() {
		public void onClick(View v) {
            updateThread = null;
			errorCode = 0;
            ULIService.ignoreAllEvents = false;
			//startService(new Intent(ULIUpdateNotification.this, ULIService.class).putExtra("action", "ULI_RecoverySystem_Update_Postponed"));
			finish();
		}
	};

	private OnClickListener mCancelListener = new OnClickListener() {
		public void onClick(View v) {

				if(checkTask != null){
					checkTask.cancel(true);
					mDownloadProgress.setIndeterminate(false);
					finish();
				}
			else{	
				ULIService.UtvProjectOnAbort();				
			}
		}
	};

	@Override
	public boolean onCreateOptionsMenu(Menu menu){
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.main_menu, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected (MenuItem item){
		switch (item.getItemId()){ 
		case R.id.itemDisplayInfo :
			showDialog(DIALOG_DISPLAY_INFO_ID);
			return true;
		}
		return false;
	}	

    @Override
    public void onBackPressed (){
        // this is to deactivate the back button
    }

	// called by UIUpdate Thread who is updating progress on the download
	public void setProgressBar(final int ps) {
		runOnUiThread(new Runnable() {
			public void run() {
				if(mDownloadProgress.getProgress() == 100){
                    mInfoTextView.setText(getText(R.string.validating) + " "+ ULIService.getUpdateVersion() );
                    mInfoTextView.invalidate();
                    mProgressTextView.setVisibility(View.GONE);
                    mDownloadProgress.setIndeterminate(true);
                    mDownloadProgress.invalidate();
                }
				mDownloadProgress.setProgress(ps);
				String per = "" + ps + " %";
				//Log.d(TAG,"Updating progress text ="+per);
				mProgressTextView.setText(per);
			}
		});
	}

	
	public void setInfoText(final int error) {
		runOnUiThread(new Runnable() {
			public void run() {
                ULIService.ignoreAllEvents = true;
				errorCode = error;
                Log.d("UPDATELOGIC",TAG + "#### errorCode = " + errorCode);
                progressFrame.setVisibility(View.GONE);
				buttonCancel.setVisibility(View.GONE);
				String userMessage = "";
				switch(errorCode) {
				// network related errors
				case 155://UTV_INTERNET_PERS_FAILURE
				case 156://UTV_INTERNET_SRVR_FAILURE
				case 163://UTV_INTERNET_UNKNOWN
				case 167://UTV_INTERNET_DNS_FAILS
				case 168://UTV_INTERNET_CNCT_TIMEOUT
				case 169://UTV_INTERNET_READ_TIMEOUT
				case 170://UTV_INTERNET_WRITE_TIMEOUT
				case 171://UTV_INTERNET_CONN_CLOSED
                case 243://UTV_INTERNET_CRYPT_CNCT_TIMEOUT
                case 244://UTV_INTERNET_CRYPT_READ_TIMEOUT
                case 245://UTV_INTERNET_CRYPT_WRITE_TIMEOUT
					userMessage = getText(R.string.connection_error_message).toString();
					mDescriptionTextView.setText(R.string.connection_troubleshooting_text);
					break;
                case 208://UTV_SSL_HOST_CERT_FAILS 
					userMessage = getText(R.string.connection_error_message).toString();
					mDescriptionTextView.setText(R.string.ssl_troubleshooting_text);
					break;                    
				case 12://UTV_NO_DOWNLOAD
                case 98://UTV_INCOMPATIBLE_SWVER
                case 99://UTV_INCOMPATIBLE_HWMODEL 
                case 110://UTV_BAD_MODVER_CURRENT
					mInfoTextView.setText(getText(R.string.system_up_to_date_text));
					buttonNo.setText(getText(R.string.ok));
					buttonNo.setVisibility(View.VISIBLE);
                    buttonNo.setEnabled(true);
                    buttonNo.requestFocus();
                    ULIService.ignoreAllEvents = false;
                    errorCode = 0;
					return;
                case 9://UTV_ABORT                   
                    errorCode = 0;
                    updateThread = null;
                    ULIService.ignoreAllEvents = false;
                    finish();
                    return;
				default:
					userMessage = ULIService.UtvGetMessageString(errorCode);
					userMessage = userMessage.substring(0,1).toUpperCase() + userMessage.substring(1);
				}

				userMessage += "\n" + getText(R.string.do_you_want_to_try_again);
				mInfoTextView.setText(userMessage);
			
				buttonNo.setEnabled(true);
				buttonNo.setVisibility(View.VISIBLE);
				buttonYes.setEnabled(true);
				buttonYes.setVisibility(View.VISIBLE);
                buttonYes.requestFocus();
			}
		});
	}

	public void displayNoUpdateZipDialog(){
		showDialog(DIALOG_UPDATE_DISAPPEARED_ID);
	}

	public void downloadComplete() {
		runOnUiThread(new Runnable() {
			public void run() {
				if(isLoopTest){
						startService(new Intent(ULIUpdateNotification.this, ULIService.class).putExtra("action", "ULI_Start_RecoverySystem_Update"));
					}
				else{
					ULIService.ignoreAllEvents = true;
                    mInfoTextView.setText(getText(R.string.downloading) + " "  + ULIService.getUpdateVersion() );
                    mInfoTextView.invalidate();
                    mDownloadProgress.setIndeterminate(false);
                    mDownloadProgress.invalidate();
                    mProgressTextView.setVisibility(View.VISIBLE);
                    mProgressTextView.invalidate();
				showDialog(DIALOG_DOWNLOAD_ENDED_ID);
			}
			}
		});
		
	}
	
	// class to hold state between Activity instances when config changes occur
	public class State {
		public int errorCode;
		public UpdateThread updateThread;
		public AsyncTask<String, Integer, Integer> checkTask;
		public boolean isMandatory;
        public boolean updateAvailable;
        public boolean isUpdateAvailableDialogDisplayed;
	}

	public static long GetCacheFreeSize()
	{
		StatFs stat_cache = new StatFs(Environment.getDownloadCacheDirectory().getPath());
        long blockSize_cache       = stat_cache.getBlockSize();
        long availableBlocks_cache = stat_cache.getAvailableBlocks();
        //long totalBlocks_cache     = stat_cache.getBlockCount();
		return availableBlocks_cache* blockSize_cache ;
	}
	
	public static int ClearOldFilesFromCache(long i_millisFromCurrent, boolean b_rootOnly) 
	{
		return RemoveOutOfDateFile(Environment.getDownloadCacheDirectory() , !b_rootOnly,  System.currentTimeMillis() - i_millisFromCurrent  );
	}
	
    private static int RemoveOutOfDateFile(File rootFolder, boolean b_reverseRemove, long i_createMillis )
    {
        //if the files' timestamp are i_createMillis before current datetime, then these out-of-date files would be removed.
        try {
            String[] children = rootFolder.list(); 
            for (int i=0; i<children.length; i++) {
                 File currentFile=new File(rootFolder, children[i]);
                 if (currentFile.isFile())
                 {
				     if (i_createMillis > currentFile.lastModified())//file too old, created before i_createMillis
                          currentFile.delete();
                 }else if (currentFile.isDirectory() && b_reverseRemove) {
                      RemoveOutOfDateFile(currentFile  ,true, i_createMillis);
                 }
            }
        }catch (Exception e) {
            return -1;//Log.e(TAG, "format fail:"+pathName);
        }
		return 0; 
    }
	
}
