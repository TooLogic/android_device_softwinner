/* ULINotificationDialog
 * 
 * Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved. 
 *
 */

package com.uli.supervisor;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.app.Dialog;

public class ULINotificationDialog extends Activity {
	private String param1 = "";
	private DialogInterface.OnClickListener okListener;
	private DialogInterface.OnClickListener askMeLaterListener;
	private Intent updaterActivity;
	private Bundle bundle;
	private boolean isMandatory = false;
	private AlertDialog currentDialog;
	static final int DIALOG_UPDATE_ID = 0;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		this.setTheme(android.R.style.Theme_NoDisplay);
		super.onCreate(savedInstanceState);

        ULIService.ignoreAllEvents = true;
        ULIService.dialogVisible = true;
        
		bundle = this.getIntent().getExtras();
		if(bundle != null){
			param1 = bundle.getString("param1");
		}
		isMandatory = param1.contains("-mandatoryUpdate");

		updaterActivity = new Intent(this, ULIUpdateNotification.class);
		
		okListener = new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int id) {
				//updaterActivity.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                ULIService.dialogVisible = false;
				updaterActivity.putExtras(bundle);
				startActivity(updaterActivity);
				dialog.cancel();
                finish();
			}
		};
		
		askMeLaterListener = new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int id) {
                ULIService.dialogVisible = false;
                ULIService.ignoreAllEvents = false;
				startService(new Intent(ULINotificationDialog.this, ULIService.class).putExtra("action", "ULI_RecoverySystem_Update_Postponed"));
				dialog.cancel();
                finish();
			}
		};
		
		showDialog(DIALOG_UPDATE_ID);
		if (param1.contains("-looptest")){
			currentDialog.getButton(DialogInterface.BUTTON_POSITIVE).performClick();
		}
		
	}

		
	@Override
	protected Dialog onCreateDialog(int id) {
		AlertDialog dialog;
		switch(id) {
		case DIALOG_UPDATE_ID:
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setCancelable(false);
            //builder.setUseZephyrStyle(true);

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
		default:
			dialog = null;
		}
		currentDialog = dialog;
		return dialog;
	}

	@Override
	protected void onPostCreate (Bundle savedInstanceState) {
		super.onPostCreate(savedInstanceState);
	}

}