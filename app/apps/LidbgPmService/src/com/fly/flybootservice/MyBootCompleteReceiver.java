package com.fly.flybootservice;

import android.app.Activity;  
import android.content.ComponentName; 
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Slog; 

public class MyBootCompleteReceiver extends BroadcastReceiver
{
	@Override
	public void onReceive(Context context, Intent intent)
	{
		// TODO Auto-generated method stub
 	 	Intent mIntent = new Intent();
 	 	mIntent.setComponent(new ComponentName("com.fly.flybootservice","com.fly.flybootservice.FlyBootService"));
 	 	//mIntent.setClass(context, FlyBootService.class);
 	 	//mIntent.addFlags(Intent.FLAG_DEBUG_TRIAGED_MISSING);
         	context.startService(mIntent);
         	Slog.i("boot","flyservice BC start......starting flybootservice "+mIntent); 
	}
}

