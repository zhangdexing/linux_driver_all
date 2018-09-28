package com.example.sleeptest;

import java.io.File;
import java.io.FileOutputStream;

import android.app.Activity;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.SystemClock;
import android.util.Log;

public class SleepTest extends Activity {
	
	private PowerManager fbPm = null;
	private long mAwakeTime = 5000L;
	private long mSleepTime = 80000L;
	private int mTestCount = 0;
	private int mLimitCount = 0;
	private int mics = 10000;
	private static SleepTest mSleepTest;
	private PowerBroadcastReceiver powerBroadcastReceiver = null;

	private static final String LOG_TAG = "SleepTestActivity";
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		LIDBG_PRINT("====== [SleepTest] OnCreate ========");		
		//setContentView(R.layout.activity_sleep_test);
	        IntentFilter filter_secret = new IntentFilter();
	        filter_secret.addAction("android.provider.Telephony.SECRET_CODE");
		filter_secret.addDataScheme("android_secret_code");
		powerBroadcastReceiver = new PowerBroadcastReceiver();
	        registerReceiver(powerBroadcastReceiver, filter_secret);
	        registerReceiver(SleepTestReceiver, new IntentFilter(
				"com.rockchip.sleep.ACTION_TEST_CASE_SLEEP"));
		fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		mSleepTest = this;
		Intent intent = getIntent();
		int pid = intent.getIntExtra("time", -1);
		mSleepTime = pid/100 * 1000;
		mics = (pid % 100) * 1000;
		if(mSleepTime < 70000L)
			mSleepTime = 80000L;
		if(mics < 10000)
			mics = 10000;
		LIDBG_PRINT("==== value mSleepTime mics ===="+pid +mSleepTime +mics);
		startTest(this);
	}
	
	private void startTest(Context context) {
		//LIDBG_PRINT("====== [SleepTest] GotoSleep =======");
		//fbPm.goToSleep(SystemClock.uptimeMillis());
		//设置闹钟
		LIDBG_PRINT("====== [SleepTest] Set Alarm =======");
		setAlarm(context, mSleepTime, true);	
	}
	
	private void setAlarm(Context paramContext, long paramLong, boolean repeat) {
		AlarmManager localAlarmManager = (AlarmManager) paramContext
				.getSystemService("alarm");
		PendingIntent localPendingIntent = PendingIntent.getBroadcast(
				paramContext, 0, new Intent(
						"com.rockchip.sleep.ACTION_TEST_CASE_SLEEP"), 0);
		localAlarmManager.set(AlarmManager.RTC_WAKEUP, paramLong + System.currentTimeMillis(),	
				localPendingIntent);
		if (!repeat){
			localAlarmManager.setRepeating(0,
					paramLong + System.currentTimeMillis(), paramLong,
					localPendingIntent);
		}
	}
	
	private void stopAlarm(Context paramContext) {
		PendingIntent localPendingIntent = PendingIntent.getBroadcast(
				paramContext, 0, new Intent(
						"com.rockchip.sleep.ACTION_TEST_CASE_SLEEP"), 0);
		((AlarmManager) paramContext.getSystemService("alarm")).cancel(localPendingIntent);
	}
	
	private void stopTest(Context context) {
		Log.d(LOG_TAG, "stopTest ...");
	    	stopAlarm(context);
		}
	
    	class PowerBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
			// TODO Auto-generated method stub
			String action = intent.getAction();
			LIDBG_PRINT("====== [SleepTest] Broadcast Receiver ======"+action);
			if (action.equals("android.provider.Telephony.SECRET_CODE")) {
				procSecretCode(intent);
			}
		}
		
	}
	
	
    	private BroadcastReceiver SleepTestReceiver = new BroadcastReceiver() {

		@Override
		public void onReceive(Context context, Intent intent) {
			Log.e(LOG_TAG, "SleepTestReceiver onReceive...");
			 mTestCount = mTestCount+1;
			 if (mLimitCount != 0 && mTestCount >= mLimitCount) {
				 stopTest(context);
			 } else {
				((PowerManager) context.getSystemService("power")).newWakeLock(
						PowerManager.ACQUIRE_CAUSES_WAKEUP
								| PowerManager.FULL_WAKE_LOCK, "ScreenOnTimer")
						.acquire(mAwakeTime);//超时锁;
				LIDBG_PRINT("====== [SleepTest] Start SleepTest Again ====== time :"+mTestCount);
				delay(mics);
				startTest(mSleepTest);
			 }
		}
		
	};
	
	
    private void procSecretCode(Intent intent) {
        Log.d(LOG_TAG, "  SECRET_CODE_ACTION  " + intent.getData().getHost());
        String secretHost = intent.getData().getHost();
        if (secretHost.equals("4600")) {
        	/*启动测试*/
		LIDBG_PRINT("====== [SleepTest] Start SleepTest ======");
        	//startTest(mSleepTest);
        }
    }
    
    private static void LIDBG_PRINT(String msg) {
        Log.d(LOG_TAG, msg);
        String newmsg = "lidbg_msg: " + msg;
        File mFile = new File("/dev/lidbg_msg");
        if (mFile.exists()) {
            try {
                FileOutputStream fout = new FileOutputStream(
                        mFile.getAbsolutePath());
                byte[] bytes = newmsg.getBytes();
                fout.write(bytes);
                fout.close();
            } catch (Exception e) {
                Log.e(LOG_TAG, "Failed to lidbg_printk");
            }

        } else {
            Log.e(LOG_TAG, "/dev/lidbg_msg not exist");
        }
    }
    
    private void delay(int ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block am start -n com.example.sleeptest/com.example.sleeptest.SleepTest
            e.printStackTrace();
        }
    }

    public void onDestroy() {
	super.onDestroy();	
	unregisterReceiver(powerBroadcastReceiver);
	unregisterReceiver(SleepTestReceiver);
	stopTest(mSleepTest);
	LIDBG_PRINT("======== SleepTest On Destory ========");
    }
}




