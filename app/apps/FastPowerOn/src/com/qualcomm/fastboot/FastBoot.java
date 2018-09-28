/*
 * packages/apps/FastPowerOn/src/com/qualcomm/fastboot/FastBoot.java** Copyright
 * 2006, The Android Open Source Project* Copyright (c) 2012, The Linux
 * Foundation. All Rights Reserved.** Licensed under the Apache License, Version
 * 2.0 (the "License");* you may not use this file except in compliance with the
 * License.* You may obtain a copy of the License at**
 * http://www.apache.org/licenses/LICENSE-2.0** Unless required by applicable
 * law or agreed to in writing, software* distributed under the License is
 * distributed on an "AS IS" BASIS,* WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.* See the License for the specific language
 * governing permissions and* limitations under the License.
 */

package com.qualcomm.fastboot;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.ContentResolver;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.Service;
import android.app.WallpaperInfo;
import android.app.WallpaperManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.util.List;
import java.io.IOException;

public class FastBoot extends Activity {
    private static final String TAG = "FastBoot";
    private static FastBoot mFastBoot;
    private static final String SYSTEM_RESUME = "com.flyaudio.system.resume";
    private static boolean bIsKLDRunning = true;
    public static PowerManager pm;
    public static WakeLock mFbWakeLock = null;

    private static void LIDBG_PRINT(String msg) {
        Log.d(TAG, msg);
        String newmsg = "lidbg_msg: " + msg+".";
        File mFile = new File("/dev/lidbg_msg");
        if (mFile.exists()) {
            try {
                FileOutputStream fout = new FileOutputStream(
                        mFile.getAbsolutePath());
                byte[] bytes = newmsg.getBytes();
                fout.write(bytes);
                fout.close();
            } catch (Exception e) {
                Log.e(TAG, "Failed to lidbg_printk");
            }

        } else {
            Log.e(TAG, "/dev/lidbg_msg not exist");
        }
    }

        private static void msgTokenal(String msg) {
            // TODO Auto-generated method stub
                File mFile = new File("/dev/lidbg_pm0");
                String str = msg;
                if (mFile.exists()) {
                    try {
                        LIDBG_PRINT(" msgTokenal");
                        FileOutputStream fout = new FileOutputStream(
                                mFile.getAbsolutePath());
                        byte[] bytes = str.getBytes();
                        fout.write(bytes);
                        fout.close();
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        LIDBG_PRINT(" writeToFile  IOException ");
                        e.printStackTrace();
                    }
                } else {
                    LIDBG_PRINT("file not exists!!!");
                }
            }


    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.connectivity);

        LIDBG_PRINT("*** FastBoot onCreate start ***");
        mFastBoot = this;
        IntentFilter finishFilter = new IntentFilter();
        finishFilter.addAction("FinishActivity");
        registerReceiver(mCommunicateReceiver, finishFilter);

        LIDBG_PRINT("before startService+");
        startService(new Intent(this, localSerice.class));
        LIDBG_PRINT("*** FastBoot onCreate end ***");
		 acquireWakeLock();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    public void acquireWakeLock() {
        if (mFbWakeLock == null) {
            LIDBG_PRINT("+++ acquire fastboot WakeLock +++");
            pm = (PowerManager) getSystemService(Context.POWER_SERVICE);

            mFbWakeLock = (WakeLock) pm.newWakeLock(
                    PowerManager.PARTIAL_WAKE_LOCK, "fastbootWakeLock");
				if (mFbWakeLock != null && !mFbWakeLock.isHeld()){
					LIDBG_PRINT("New wakelock ok");
					mFbWakeLock.acquire();
					if(!mFbWakeLock.isHeld())
						LIDBG_PRINT("Error: acquire WakeLock failed");
				}
				else{
					LIDBG_PRINT("New wakelock failed, acquire again");
					mFbWakeLock = (WakeLock) pm.newWakeLock(
						PowerManager.PARTIAL_WAKE_LOCK, "fastbootWakeLock");
					if (mFbWakeLock != null && !mFbWakeLock.isHeld())
					mFbWakeLock.acquire();
				}
        }
    }

    private static void releaseWakeLock() {
        LIDBG_PRINT("---release fastboot WakeLock ---");
        if (mFbWakeLock != null && mFbWakeLock.isHeld()) {
			mFbWakeLock.release();
			if(mFbWakeLock.isHeld()){
				LIDBG_PRINT("Error: release WakeLock failed,do it again");
				mFbWakeLock.release();
			}
			mFbWakeLock = null;
        }
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    public static void restoreAirplaneMode(Context context) {
        LIDBG_PRINT("restoreAirplaneMode+");
        if (Settings.Global.getInt(context.getContentResolver(), "fastboot_airplane_mode", -1) != 0) {
            return;
        }
	/*
	final ContentResolver cr = context.getContentResolver();
        Settings.Secure.putInt(cr, Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_HIGH_ACCURACY);
        LIDBG_PRINT("ON  LOCATION_MODE: " + Settings.Secure.getInt(context.getContentResolver(), Settings.Secure.LOCATION_MODE,
                        -100));
	*/
	
        boolean b = Settings.Global.putInt(context.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0);

        LIDBG_PRINT("restoreAirplane isSet:"+b);
        
        Intent intentAirplane = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        intentAirplane.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intentAirplane.putExtra("state", false);

        context.sendBroadcastAsUser(intentAirplane, UserHandle.ALL);
        Settings.Global.putInt(context.getContentResolver(), "fastboot_airplane_mode", -1);

        LIDBG_PRINT("restoreAirplaneMode end");
        
    }

    protected BroadcastReceiver mCommunicateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            String action = intent.getAction();
            if (action.equals("FinishActivity")) {
                unregisterReceiver(mCommunicateReceiver);
                //stopService(new Intent(FastBoot.this, localSerice.class));
                stopServiceAsUser(new Intent(FastBoot.this, localSerice.class),UserHandle.CURRENT_OR_SELF);
                LIDBG_PRINT("finsh FastBoot!!!");
                mFastBoot.finish();
                ActivityManager  mActivityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
                mActivityManager.forceStopPackage("com.qualcomm.fastboot");
            }
        }
    };

    public static class localSerice extends Service {
        private PowerManager mPm = null;
        private static boolean powerOn = false;
        private static final int SEND_AIRPLANE_MODE_BROADCAST = 1;
        private static final int SEND_BOOT_COMPLETED_BROADCAST = 2;
        private static boolean sendBroadcastDone = false;
        private ActivityManager mActivityManager = null;
        private HandlerThread mHandlerThread;
        private Handler mHandler;
        Thread sendBroadcastThread = null;
        // add launcher in protected list
        String systemLevelProcess[] = {
                "com.android.flyaudioui",
                "cn.flyaudio.android.flyaudioservice",
                "cn.flyaudio.navigation", "com.android.launcher",
                "cn.flyaudio.osd.service", "android.process.acore",
                "android.process.media", "com.android.systemui",
                "com.android.deskclock", "sys.DeviceHealth", "system",
                "com.fly.flybootservice","com.android.keyguard","android.policy"
        };

        @Override
        public void onCreate() {
            super.onCreate();
            LIDBG_PRINT("localSerice onCreate+");

            mPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            mActivityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
            LIDBG_PRINT("localSerice after getSystemService");

            mHandlerThread = new HandlerThread(TAG);
            mHandlerThread.start();
            LIDBG_PRINT("localSerice after Thread.start");

            mHandler = new Handler(mHandlerThread.getLooper(), mHandlerCallback);

            new Thread() {
                @Override
                public void run() {
                    LIDBG_PRINT("begin to run thread.");
                    powerOffSystem();
                }
            }.start();
            
            registerReceiver(systemResumeBroadcast, new IntentFilter(SYSTEM_RESUME));
            LIDBG_PRINT("localSerice onCreate-");
        }

        @Override
        public IBinder onBind(Intent intent) {
            return null;
        }

        private Handler.Callback mHandlerCallback = new Handler.Callback() {
            /**
             * {@inheritDoc}
             * 
             * @return
             */
            public boolean handleMessage(Message msg) {
                Log.d(TAG,
                        "handleMessage begin in "
                                + SystemClock.elapsedRealtime());
                LIDBG_PRINT("begin handleMessage.");

                switch (msg.what) {
                    case SEND_AIRPLANE_MODE_BROADCAST:
                        LIDBG_PRINT("Send Set airplane mode broadcast.");

                        Log.d(TAG,
                                "Set airplane mode begin in**** "
                                        + SystemClock.elapsedRealtime()
                                        + ", airplane mode : " + msg.arg1);
                        Intent intentAirplane = new Intent(
                                Intent.ACTION_AIRPLANE_MODE_CHANGED);
                        intentAirplane
                                .addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
                        intentAirplane.putExtra("state", msg.arg1 == 1);
                        sendOrderedBroadcast(intentAirplane, null,
                                sendBroadcasResult, mHandler, 0, null, null);
                        LIDBG_PRINT("end Send Set airplane mode broadcast.");
                        break;
                    case SEND_BOOT_COMPLETED_BROADCAST:
			/*                        
			Log.d(TAG,
                                "Send bootCompleted begin in "
                                        + SystemClock.elapsedRealtime());
                        LIDBG_PRINT("Send bootCompleted.");

		        Intent intentBoot = new Intent("android.intent.action.BOOT_COMPLETED");
        		intentBoot.putExtra("flyauduio_accon", "accon");
                        sendOrderedBroadcast(intentBoot, null, sendBroadcasResult,
                                mHandler, 0, null, null);
                        LIDBG_PRINT("end Send bootCompleted.");
			*/
                        break;
                    default:
                        sendBroadcastDone = true;
                        return false;
                }
                return true;
            }
        };

    BroadcastReceiver systemResumeBroadcast = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                Log.d(TAG, "Send Broadcast finish in " + SystemClock.elapsedRealtime());
                String action = intent.getAction();
                if (action.equals(SYSTEM_RESUME)) {
                    // receiver the system resume msg ,show logo and finish fastboot.
                    LIDBG_PRINT("systemResumeBroadcast  resume, fast power on.");
                    enableShowLogo(true);
                    SystemClock.sleep(3000);
                    // enableShowLogo(false);
                    // SystemClock.sleep(700);
                    LIDBG_PRINT("enableShowLogo");
                    powerOnSystem(mFastBoot);

            
                    Intent iFinish = new Intent("FinishActivity");
                    sendBroadcastAsUser(iFinish,UserHandle.ALL);
                    unregisterReceiver(systemResumeBroadcast);
                    LIDBG_PRINT("unregisterReceiver sendBroadcasResult-");
        }
            }
};


        BroadcastReceiver sendBroadcasResult = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                    Log.d(TAG, "Send Broadcast finish in " + SystemClock.elapsedRealtime());
                    LIDBG_PRINT("o Send Broadcast finish");
                    sendBroadcastDone = true;
            }
        };


        public boolean IsKLDrunning() {
            ActivityManager am = (ActivityManager) this
                    .getSystemService(Context.ACTIVITY_SERVICE);
            List<RunningTaskInfo> list = am.getRunningTasks(200);

            for (RunningTaskInfo info : list) {
                if (info.topActivity.getPackageName().equals(
                        "cld.navi.c2739.mainframe")
                        && info.baseActivity.getPackageName().equals(
                                "cld.navi.c2739.mainframe")
                        || info.topActivity.getPackageName().equals(
                                "com.autonavi.xmgd.navigator")
                        && info.baseActivity.getPackageName().equals(
                                "com.autonavi.xmgd.navigator")
                        || info.topActivity.getPackageName().equals(
                                "com.baidu.BaiduMap")
                        && info.baseActivity.getPackageName().equals(
                                "com.baidu.BaiduMap")
			|| info.topActivity.getPackageName().equals(
                                "com.waze")
                        && info.baseActivity.getPackageName().equals(
                                "com.waze")) {
                    return true;
                }
            }
            return false;
        }

        private boolean isFlyApp(String packageName) {
            return ("com.android.flyaudioui").equals(packageName)
                    || ("cn.flyaudio.media").equals(packageName)
                    || ("com.qualcomm.fastboot").equals(packageName);
        }

        private void getLastPackage() {
            ActivityManager mAManager = (ActivityManager) this
                    .getSystemService(Context.ACTIVITY_SERVICE);
            List<ActivityManager.RecentTaskInfo> list = mAManager
                    .getRecentTasks(20, 0);

            for (ActivityManager.RecentTaskInfo item : list) {
                String packageName = item.baseIntent.getComponent()
                        .getPackageName();
                Log.d(TAG, "@@" + packageName);
                if (!isFlyApp(packageName)) {
                    SystemProperties.set("fly.third.LastPageName", packageName);
                    SystemProperties.set("fly.third.LastClassName",
                            item.baseIntent.getComponent().getClassName());
                    Log.d(TAG, "LastPageName--->" + packageName);
                    Log.d(TAG, "LastClassName--->"
                            + item.baseIntent.getComponent().getClassName());
                    return;
                }
            }

        }

        private void powerOffSystem() {
            // notify music application to pause music before kill the
            // application
            LIDBG_PRINT("powerOffSystem+");
            sendBecomingNoisyIntent();
            LIDBG_PRINT("powerOffSystem step 1");

            SystemProperties.set("ctl.start", "bootanim");
            LIDBG_PRINT("powerOffSystem step 2");

            enterAirplaneMode();
            LIDBG_PRINT("powerOffSystem step 3");
            getLastPackage();
            // flyaudio
            bIsKLDRunning = IsKLDrunning();

            if (bIsKLDRunning) {
                // FlyPowerOnWithKLD();
                SystemProperties.set("fly.gps.run", "1");
                Log.d(TAG, "-----fly.gps.run----1----");
            } else {
                SystemProperties.set("fly.gps.run", "0");
                Log.d(TAG, "-----fly.gps.run-----0---");
            }
            LIDBG_PRINT("powerOffSystem step 4");
            KillProcess();
            LIDBG_PRINT("powerOffSystem step5");
	    //goToSleep前通知内核,作用关外设
	    msgTokenal("flyaudio gotosleep");

            LIDBG_PRINT("powerOffSystem step6");
            SystemClock.sleep(1000);
            LIDBG_PRINT("powerOffSystem step7");
//            mPm.goToSleep(SystemClock.uptimeMillis());
			  releaseWakeLock();
            LIDBG_PRINT("powerOffSystem-");
            // Intent iFinish = new Intent("FinishActivity");
            // sendBroadcast(iFinish);
        }

        private void powerOnSystem(Context context) {
            LIDBG_PRINT("qf powerOnSystem+");
	    //msgTokenal("bootcompleted");
            //sendBootCompleted(false);
            LIDBG_PRINT("powerOnSystem step 1");
            restoreAirplaneMode(context);
            LIDBG_PRINT("powerOnSystem step 2");
            SystemProperties.set("ctl.stop", "bootanim");
            LIDBG_PRINT("powerOnSystem-");
        }

        // send broadcast to music application to pause music
        private void sendBecomingNoisyIntent() {
            sendBroadcast(new Intent(AudioManager.ACTION_AUDIO_BECOMING_NOISY));
        }

        private void KillProcess() {
            List<ActivityManager.RunningAppProcessInfo> appProcessList = null;

            appProcessList = mActivityManager.getRunningAppProcesses();

            LIDBG_PRINT("begin to KillProcess.");
            for (ActivityManager.RunningAppProcessInfo appProcessInfo : appProcessList) {
                int pid = appProcessInfo.pid;
                int uid = appProcessInfo.uid;
                String processName = appProcessInfo.processName;
                if (isKillableProcess(processName)) {
                    // mActivityManager.killBackgroundProcesses(processName);
                    LIDBG_PRINT(processName +"."+pid +" will be killed");
                    mActivityManager.forceStopPackage(processName);
		    /*if(processName.equals("cld.navi.c2739.mainframe")){
			msgTokenal("flyaudio kill "+pid);
			LIDBG_PRINT(pid + " cld.navi.c2739.mainframe"+" will be kill by the kenal");	
			}*/

                }
            }
        }

        private boolean isKillableProcess(String packageName) {
            for (String processName : systemLevelProcess) {
                if (processName.equals(packageName)) {
                    return false;
                }
            }
            String currentProcess = getApplicationInfo().processName;
            if (currentProcess.equals(packageName)) {
                return false;
            }

            // couldn't kill the live wallpaper process, if kill it, the system
            // will set the wallpaper as the default.
            WallpaperInfo info = WallpaperManager.getInstance(this)
                    .getWallpaperInfo();
            if (info != null && !TextUtils.isEmpty(packageName)
                    && packageName.equals(info.getPackageName())) {
                return false;
            }

            // couldn't kill the IME process.
            String currentInputMethod = Settings.Secure.getString(
                    getContentResolver(), Settings.Secure.DEFAULT_INPUT_METHOD);
            if (!TextUtils.isEmpty(currentInputMethod)
                    && currentInputMethod.startsWith(packageName)) {
                return false;
            }
            return true;
        }

        private void sendBootCompleted(boolean wait) {
        LIDBG_PRINT(" sendBootCompleted :" + wait);
            synchronized (this) {
                sendBroadcastDone = false;
                // sendBroadcastThread.start();
                mHandler.sendMessage(Message.obtain(mHandler,
                        SEND_BOOT_COMPLETED_BROADCAST));
                while (wait && !sendBroadcastDone) {
                    SystemClock.sleep(100);
                    LIDBG_PRINT("sendBootCompleted sleep-");
                }
                sendBroadcastDone = false;
            }
       		LIDBG_PRINT("sendBootCompleted-");
        }

        
        
        
        
        
        
        public static boolean isAirplaneModeOn(Context context) {
            return Settings.Global.getInt(context.getContentResolver(),
                    Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
        }
        
	/*
	private static boolean isGpsOn(Context context){
		return Settings.Secure.getInt(context.getContentResolver(),
                    Settings.Secure.LOCATION_MODE, 0) != 0;
	}
        */
        
        private void enterAirplaneMode() {
	    /*
	    if(isGpsOn(this)){
	        final ContentResolver cr = this.getContentResolver();
                Settings.Secure.putInt(cr, Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_HIGH_ACCURACY);
		LIDBG_PRINT("isGpsOn, we off gps :"+Settings.Secure.getInt(getContentResolver(), Settings.Secure.LOCATION_MODE,
                        -100));
	    }
	    */
	    
            if (isAirplaneModeOn(this)) {
		LIDBG_PRINT("isAirplaneModeOn return.");
                return;
            }
            Settings.Global.putInt(getContentResolver(), "fastboot_airplane_mode", 0);

            // Change the system setting
            Settings.Global.putInt(getContentResolver(), Settings.Global.AIRPLANE_MODE_ON,1);

            // Update the UI to reflect system setting
            // Post the intent
            Intent intent = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
            intent.putExtra("state", true);
            sendBroadcastAsUser(intent, UserHandle.ALL);
        }


        private void enableShowLogo(boolean on) {
            String disableStr = (on ? "1" : "0");
            SystemProperties.set("hw.showlogo.enable", disableStr);
        }
    }
}

