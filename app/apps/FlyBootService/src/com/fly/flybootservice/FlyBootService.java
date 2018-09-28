package com.fly.flybootservice;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.WallpaperInfo;
import android.app.WallpaperManager;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.Service;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.WallpaperInfo;
import android.app.WallpaperManager;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentFilter.MalformedMimeTypeException;
import android.content.SharedPreferences;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.SystemClock;
import android.os.HandlerThread;
import android.os.Message;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.media.AudioManager;
import android.util.Log;
import android.text.TextUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.util.List;
import java.io.IOException;

import java.util.List;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
/*
 * ScreenOn ScreenOff DeviceOff Going2Sleep 四种状态分别表示：1.表示正常开屏状态2.表示关屏，但没关外设的状态
 * 0'~30'的阶段3.表示关屏关外设，但没到点进入深度休眠 30'~60'的阶段4.表示发出休眠请求到执行快速休眠 60'后,即进入深度休眠
 */
public class FlyBootService extends Service {

    // public static int NORMAL = 0;
    // public static int SUSPEND = 1;
    private static final String TAG = "boot";
    AlarmManager alarmManager;
    PendingIntent pendingIntent;
    public Handler handler = new Handler();
    public PowerManager pm;
    public static WakeLock mWakeLock = null;
    private Handler mHandler = null;
    public WakeLock mFullWakeLock = null;
    private static String ACTON_FINAL_SLEEP = "fly.alarm.timeup";
    // private int mBootState = NORMAL;

    public static String action = "com.flyaudio.power";
    public static String PowerBundle = "POWERBUNDLE";
    public static String keyScreenOn = "KEY_SCREEN_ON";
    public static String keyScreenOFF = "KEY_SCREEN_OFF";
    public static String keyEearlySusupendON = "KEY_EARLY_SUSUPEND_ON";
    public static String keyEearlySusupendOFF = "KEY_EARLY_SUSUPEND_OFF";
    public static String keyFastSusupendON = "KEY_FAST_SUSUPEND_ON";
    public static String keyFastSusupendOFF = "KEY_FAST_SUSUPEND_OFF";
    public static String KeyBootState = "KEYBOOTSTATE";
    private static String file = "/dev/lidbg_pm0";
    private static String SCREEN_ON = "flyaudio screen_on";// 开屏
    private static String SCREEN_OFF = "flyaudio screen_off";// 关屏

    private static String DEVICES_ON = "flyaudio devices_up";// 开外设
    private static String DEVICES_DOWN = "flyaudio devices_down";// 关外设

    private static String FLYFASTBOOTSTART = "com.flyaudio.ap.broadcast";

    private PowerBroadcastReceiver powerBroadcastReceiver = null;
//    private FbAirplanModeReceiver fbAirplanModeReceiver = null;
    private final int DEBUG_VALUE = 1;
    private static int ALERM_TIME = 20;
    private int EALYSUSPEND_TIME = 30;
    private ContentResolver resolver = null;
    private boolean DEBUG_ONCE = true;
    private Handler sleppHandler;
    private static String SYSTEM_RESUME = "com.flyaudio.system.resume";
//    private static FastBoot mFastBoot;
    private static FlyBootService mFlyBootService;
    private static boolean bIsKLDRunning = true;
    private HandlerThread fbHandlerThread;
    private Handler fbHandler = null;
    private static final int FASTBOOT_THREAD = 1;

    private PowerManager fbPm = null;
    private static boolean powerOn = false;
    private static final int SEND_AIRPLANE_MODE_BROADCAST = 1;
    private static final int SEND_BOOT_COMPLETED_BROADCAST = 2;
    private static boolean sendBroadcastDone = false;
    private ActivityManager mActivityManager = null;
    private HandlerThread mHandlerThread;
    private Handler mmHandler;
    Thread sendBroadcastThread = null;
    private static boolean firstBootFlag = false;
    private static boolean originPmMode = true;
    private String[] mWhiteList = null;

    // add launcher in protected list
    String systemLevelProcess[] = {
            "com.android.flyaudioui",
            "cn.flyaudio.android.flyaudioservice",
            "cn.flyaudio.navigation", "com.android.launcher",
            "cn.flyaudio.osd.service", "android.process.acore",
            "android.process.media", "com.android.systemui",
            "com.android.deskclock", "sys.DeviceHealth", "system",
            "com.fly.flybootservice","com.android.keyguard","android.policy","com.android.launcher3",
	        "com.example.sleeptest","com.flyaudio.proxyservice","com.goodocom.gocsdk","cn.flyaudio.assistant","cn.flyaudio.handleservice",
		"cn.flyaudio.clientservice","cn.flyaudio.clientservice:pushservice"
    };


    private enum emState {
        Init, ScreenOn, ScreenOff, DeviceOff, Going2Sleep, Sleep
    };

    private emState mState = emState.Init;

    @Override
    public void onCreate() {
        super.onCreate();
        resolver = this.getContentResolver();
        if (1 == Settings.System.getInt(resolver, "flyaudio_debug", 0)) {
            EALYSUSPEND_TIME = 10;
        }
        LIDBG_PRINT("flybootservice onCreate......");
        mHandler = new Handler();
        sleppHandler = new Handler();
        IntentFilter filter = new IntentFilter();
        filter.addAction(FLYFASTBOOTSTART);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(ACTON_FINAL_SLEEP);

        IntentFilter filter_secret = new IntentFilter();
        // filter_secret.addAction(SECRET_CODE_ACTION);
        filter_secret.addAction("android.provider.Telephony.SECRET_CODE");

        filter_secret.addDataScheme("android_secret_code");
        filter.setPriority(Integer.MAX_VALUE);
        powerBroadcastReceiver = new PowerBroadcastReceiver();
        registerReceiver(powerBroadcastReceiver, filter);
        registerReceiver(powerBroadcastReceiver, filter_secret);

        mFlyBootService = this;

        fbHandlerThread = new HandlerThread(TAG);
        fbHandlerThread.start();
        fbHandler = new Handler(fbHandlerThread.getLooper(), fbHandlerCallback);

        acquireWakeLock();
	mWhiteList = FileReadList("/flysystem/lib/out/appProtectList.conf","\n");
	if (mWhiteList != null)
	{
		for (int i = 0; i < mWhiteList.length; i++)
		{
			LIDBG_PRINT(i +"->"+ mWhiteList[i]);
		}
	}
	else
		LIDBG_PRINT("mWhiteList = null");

	
        delay(5000);
        //setAndroidState(true);

    }

	public String[] FileReadList(String fileName, String split)
	{
		// TODO Auto-generated method stub
		String[] mList = null;
		String tempString = FileRead(fileName);
		if (tempString != null && tempString.length() > 2)
		{
			mList = tempString.trim().split(split);
		}
		return mList;
	}
	public String FileRead(String fileName)
	{
		String res = null;
		File mFile = new File(fileName);
		if (!mFile.exists() || !mFile.canRead())
		{
			return res;
		}

		try
		{
			FileInputStream inputStream = new FileInputStream(mFile);
			int len = inputStream.available();
			byte[] buffer = new byte[len];
			inputStream.read(buffer);
			res = new String(buffer, "UTF-8");
			// toast_show("resString="+fineString);
			inputStream.close();
		} catch (FileNotFoundException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return res;
	}
    @Override
    public void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
        unregisterReceiver(powerBroadcastReceiver);
//        unregisterReceiver(fbAirplanModeReceiver);
        LIDBG_PRINT("flybootservice destory...");
    }

    Runnable runnable = new Runnable() {
        @Override
        public void run() {
            LIDBG_PRINT("final sleep timeup...");
            // TODO Auto-generated method stub
            sendBroadcast(new Intent("fly.alarm.timeup"));
        }
    };

    /*
     * public void acquireFullWakeLock() { if (mFullWakeLock == null) {
     * LIDBG_PRINT("acquireFullWakeLock "); pm = (PowerManager)
     * getSystemService(Context.POWER_SERVICE); mFullWakeLock = (WakeLock)
     * pm.newWakeLock( PowerManager.FULL_WAKE_LOCK, "FlyBootService"); if
     * (mFullWakeLock != null && !mFullWakeLock.isHeld())
     * mFullWakeLock.acquire(); } }
     * 
     * public void releaseFullWakeLock() { if (mFullWakeLock != null &&
     * mFullWakeLock.isHeld()) { LIDBG_PRINT("releaseFullWakeLock ");
     * mFullWakeLock.release(); mFullWakeLock = null; } }
     */
    public void acquireWakeLock() {
        if (mWakeLock == null) {
            LIDBG_PRINT(" +++++ acquire flybootservice wakelock +++++ ");
            pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            // mWakeLock = (WakeLock) pm.newWakeLock(
            // PowerManager.PARTIAL_WAKE_LOCK
            // | PowerManager.ACQUIRE_CAUSES_WAKEUP
            // | PowerManager.ON_AFTER_RELEASE, "flytag");

            mWakeLock = (WakeLock) pm.newWakeLock(
                    PowerManager.PARTIAL_WAKE_LOCK, "flytag");
            if (mWakeLock != null && !mWakeLock.isHeld())
                mWakeLock.acquire();
            else
				  LIDBG_PRINT(" Error: new flybootservice wakelock failed !");
        }
    }

    public static void releaseWakeLock() {
        LIDBG_PRINT(" ----- release flybootservice wakelock ----- ");
        if (mWakeLock != null && mWakeLock.isHeld()) {
            mWakeLock.release();
            if(mWakeLock.isHeld())
                LIDBG_PRINT(" Error: release flybootservice wakelock failed !");
            mWakeLock = null;
        }
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


    Runnable ealysuspend = new Runnable() {
        @Override
        public void run() {
		// 发送广播
		SendBroadcastToService(KeyBootState, keyEearlySusupendOFF);
		LIDBG_PRINT("Device Off [enterAirplaneMode]");

		final boolean booleanRemoteControl = SystemProperties.getBoolean("persist.lidbg.RmtCtrlenable",false);
		if(booleanRemoteControl == true){
			LIDBG_PRINT("Flyaudio Remote-Control enabled, booleanRemoteControl:"+booleanRemoteControl);
		}else{
			enterAirplaneMode();
			LIDBG_PRINT("Flyaudio Remote-Control disabled, booleanRemoteControl:"+booleanRemoteControl);
		}

		writeToFile(file, DEVICES_DOWN);
		delay(500);
		sleppHandler.postDelayed(runnable, ALERM_TIME * 1000);
		mState = emState.DeviceOff;
        }
    };

    public void SendBroadcastToService(String key, String value) {
        // Log.d(TAG, " PowerBundle :  " + value);
        Intent intent = new Intent(action);
        Bundle bundle = new Bundle();
        bundle.putString(key, value);
        intent.putExtra(PowerBundle, bundle);
        sendBroadcast(intent);
    }

    private void procScreenOn() {
        if (mState == emState.Init) {
            mState = emState.ScreenOn;
            acquireWakeLock();
        } else if (mState == emState.ScreenOff) {
            // setAndroidState(true);
            mHandler.removeCallbacks(ealysuspend);
            sleppHandler.removeCallbacks(runnable);
            acquireWakeLock();

            restoreAirplaneMode(mFlyBootService);
            writeToFile(file, SCREEN_ON);// 正常ACC，开背光
            LIDBG_PRINT("writeToFile Screen On");
            SendBroadcastToService(KeyBootState, keyScreenOn);
            mState = emState.ScreenOn;
        } else if (mState == emState.DeviceOff) {
            mHandler.removeCallbacks(ealysuspend);
            sleppHandler.removeCallbacks(runnable);

            restoreAirplaneMode(mFlyBootService);
            writeToFile(file, DEVICES_ON);// 唤醒
            writeToFile(file, SCREEN_ON);// 开背光
            LIDBG_PRINT("writeToFile Device On");
            SendBroadcastToService(KeyBootState, keyEearlySusupendON);
            SendBroadcastToService(KeyBootState, keyScreenOn);
            mState = emState.ScreenOn;
        } else if (mState == emState.Going2Sleep) {
            // 不予处理
        } else if (mState == emState.Sleep) {
            SendBroadcastToService(KeyBootState, keyScreenOn);
	    SendBroadcastToService(KeyBootState, keyFastSusupendON);
            sendBroadcast(new Intent(SYSTEM_RESUME));
            setAndroidState(true);

            restoreAirplaneMode(mFlyBootService);
            writeToFile(file, DEVICES_ON);// 唤醒
            writeToFile(file, SCREEN_ON);// 开背光

            acquireWakeLock();
            mState = emState.ScreenOn;

           Intent intentBoot = new Intent(Intent.ACTION_BOOT_COMPLETED);
           intentBoot.putExtra("flyauduio_accon", "accon");
           sendBroadcast(intentBoot);  
	
		
        }
    }

    private void procScreenOff() {
        if (mState == emState.ScreenOn) {
            LIDBG_PRINT("Screen off");
            writeToFile(file, SCREEN_OFF);// 关背光
            SendBroadcastToService(KeyBootState, keyScreenOFF);
            mHandler.postDelayed(ealysuspend, EALYSUSPEND_TIME * 1000);
            mState = emState.ScreenOff;
        }
    }

	private void start_fastboot(){

            firstBootFlag = true;
//            IntentFilter filter_fbAirplaneMode = new IntentFilter();
//            filter_fbAirplaneMode.addAction(Intent.ACTION_BOOT_COMPLETED);
//            fbAirplanModeReceiver = new FbAirplanModeReceiver();
//            registerReceiver(fbAirplanModeReceiver, filter_fbAirplaneMode);

		     fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            mActivityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);

            if(originPmMode){
	            delay(8000);
	            mState = emState.Sleep;
            }

            LIDBG_PRINT(" ********** start fastboot ********** ");
            mHandlerThread = new HandlerThread("fastboot");
            mHandlerThread.start();

            mmHandler = new Handler(mHandlerThread.getLooper(), mHandlerCallback);
            registerReceiver(systemResumeBroadcast, new IntentFilter(SYSTEM_RESUME));

//            new Thread() {
//                @Override
//                public void run() {
//                    LIDBG_PRINT("start fastbboot thread.");
                    powerOffSystem();
//                }
//            }.start();

	}

    // 动态注册广播 ，接收screen_of及screen_on.
    class PowerBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
			// TODO Auto-generated method stub
			String action = intent.getAction();
			LIDBG_PRINT("get action:" + action + " state:" + mState);
			if(originPmMode)
			{
				LIDBG_PRINT("<<<<<<<<<< FlyBootService originPmMode 1111>>>>>>>>>>");
//				if (action.equals(FLYFASTBOOTSTART)) {
//					LIDBG_PRINT(" start fastpower on ");
//                sendBroadcast(new Intent(
//                        "android.intent.action.FAST_BOOT_START"));// \u542f\u52a8FastBootPowerOn
//					LIDBG_PRINT("+++ send fastboot message +++");
//					fbHandler.sendMessage(Message.obtain(fbHandler, FASTBOOT_THREAD));
//					LIDBG_PRINT(" FLYFASTBOOTSTART-");

//					firstBootFlag = true;
//					mState = emState.Sleep;

//				} else if (action.equals(Intent.ACTION_SCREEN_ON)) {
              if (action.equals(Intent.ACTION_SCREEN_ON)) {
					procScreenOn();
				} else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
					procScreenOff();
				} else if (action.equals(ACTON_FINAL_SLEEP)) {
				        if (mState != emState.DeviceOff) {
					LIDBG_PRINT("====skip :[mState != emState.DeviceOff]["+mState+"]==\n\n\n\n");
					return;
				        	}
					SendBroadcastToService(KeyBootState, keyFastSusupendOFF);
					setAndroidState(false);// \u5411\u5e95\u5c42\u8bf7\u6c42FAST_BOOT_START\u5e7f\u64ad
					delay(100);
					mState = emState.Going2Sleep;

					LIDBG_PRINT("+++ send fastboot message +++");
					fbHandler.sendMessage(Message.obtain(fbHandler, FASTBOOT_THREAD));
//					releaseWakeLock();
					LIDBG_PRINT("going to sleep");
				} else if (action.equals("android.provider.Telephony.SECRET_CODE")) {
					LIDBG_PRINT("SECRET_CODE :::" + action + " state:" + mState);
					procSecretCode(intent);
				}
			}
			else{
				LIDBG_PRINT("<<<<<<<<<< FlyBootService testPmMode >>>>>>>>>>");
				if (action.equals(Intent.ACTION_SCREEN_ON)) {
					procScreenOn();
				} else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
					procScreenOff();
				} else if (action.equals(ACTON_FINAL_SLEEP)) {
					SendBroadcastToService(KeyBootState, keyFastSusupendOFF);
					setAndroidState(false);// 向底层请求FAST_BOOT_START广播

					LIDBG_PRINT("+++ send fastboot message +++");
					fbHandler.sendMessage(Message.obtain(fbHandler, FASTBOOT_THREAD));

					mState = emState.Sleep;
					delay(100);
					//					releaseWakeLock();
					LIDBG_PRINT("going to sleep");
				} else if (action.equals("android.provider.Telephony.SECRET_CODE")) {
					procSecretCode(intent);
				}
			}
			}
    }

	private Handler.Callback fbHandlerCallback = new Handler.Callback() {
		public boolean handleMessage(Message msg) {
			Log.d(TAG, "    msg.what = " + msg.what + "    thread id : "
					+ " thread name: " + Thread.currentThread().getId()
					+ Thread.currentThread().getName());
			switch (msg.what) {
			case FASTBOOT_THREAD:
				start_fastboot();
				break;
			default:
			    return false;
			}
			return true;
		}
	};

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
                            sendBroadcasResult, mmHandler, 0, null, null);
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
	            if ((action.equals(SYSTEM_RESUME)) && (firstBootFlag)) {
	                // receiver the system resume msg ,show logo and finish fastboot.
	                LIDBG_PRINT("systemResumeBroadcast  resume, fast power on.");
	                enableShowLogo(true);
	                SystemClock.sleep(3000);
	                // enableShowLogo(false);
	                // SystemClock.sleep(700);
	                LIDBG_PRINT("enableShowLogo");
	                powerOnSystem(mFlyBootService);

	        
//	                Intent iFinish = new Intent("FinishActivity");
//	                sendBroadcastAsUser(iFinish,UserHandle.ALL);
	                mHandlerThread.quit();
	                LIDBG_PRINT("quit fastboot thread.");
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

//		enterAirplaneMode();
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
		if(originPmMode)
			fbPm.goToSleep(SystemClock.uptimeMillis());
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

        LIDBG_PRINT("begin to KillProcess."+(mWhiteList == null));
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
	if (mWhiteList != null)
	{
	        for (String processName : mWhiteList) {
	            if (processName.equals(packageName)) {
	                return false;
	            }
	        }
	}
	else
	{
	        for (String processName : systemLevelProcess) {
	            if (processName.equals(packageName)) {
	                return false;
	            }
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
            mmHandler.sendMessage(Message.obtain(mmHandler,
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
    
    private void enterAirplaneMode() {
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


    private void procSecretCode(Intent intent) {
        Log.d(TAG, "  SECRET_CODE_ACTION  " + intent.getData().getHost());
        String secretHost = intent.getData().getHost();
        if (secretHost.equals("4633")) {
            LIDBG_PRINT(" Return FLYAUDIO PRODUCE  MODE!!!! ");

            Settings.System.putInt(resolver, "flyaudio_debug", 0);
            Settings.System.putInt(resolver, "EALYSUSPEND_TIME", 60);
            EALYSUSPEND_TIME = Settings.System.getInt(resolver,
                    "EALYSUSPEND_TIME", 60);

        } else if (secretHost.equals("4634")) {
            LIDBG_PRINT(" Enter FLYAUDIO DEBUG MODE!!!! ");

            Settings.System.putInt(resolver, "flyaudio_debug", 1);
            Settings.System.putInt(resolver, "EALYSUSPEND_TIME", 10);
            EALYSUSPEND_TIME = Settings.System.getInt(resolver,
                    "EALYSUSPEND_TIME", 10);

        }else if (secretHost.equals("4632")) {
            LIDBG_PRINT(" Enter flyaudio  oncetimes debug mode!!!! ");

            Settings.System.putInt(resolver, "flyaudio_debug", 0);

            Settings.System.putInt(resolver, "EALYSUSPEND_TIME", 60);
            EALYSUSPEND_TIME = 10;
            DEBUG_ONCE = false;
        }else if (secretHost.equals("4631")) {
				LIDBG_PRINT(" Enter flyaudio  oncetimes debug mode!!!! ");

				Settings.System.putInt(resolver, "flyaudio_debug", 0);

				Settings.System.putInt(resolver, "EALYSUSPEND_TIME", 60);
				ALERM_TIME = 10;
				EALYSUSPEND_TIME = 10;
				DEBUG_ONCE = false;
        }else if (secretHost.equals("4630")) {
				LIDBG_PRINT(" Enter flyaudio  oncetimes debug mode!!!! ");

				Settings.System.putInt(resolver, "flyaudio_debug", 0);

				Settings.System.putInt(resolver, "EALYSUSPEND_TIME", 60);
				ALERM_TIME = 5;
				EALYSUSPEND_TIME = 5;
				DEBUG_ONCE = false;
        }else if (secretHost.equals("4629")) {
				LIDBG_PRINT("Debug mode starting, mState: %s" + mState);

				if(mState == emState.Sleep){
					mActivityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
					delay(100);
					KillProcess();
				}
        } else if (secretHost.equals("4600")) {
            LIDBG_PRINT(" Enter flyaudio  oncetimes debug fast mode!!!! ");
            ALERM_TIME = 0;
            EALYSUSPEND_TIME = 0;
            DEBUG_ONCE = false;
        }
		else if (secretHost.equals("4700")) {
				LIDBG_PRINT("Debug mode starting ...");
				mActivityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
				delay(100);
				KillProcess();
        }
        Log.d(TAG,
                " value :"
                        + Settings.System.getInt(resolver, "flyaudio_debug", 0)
                        + " ALERM_TIME:" + ALERM_TIME + " EALYSUSPEND_TIME:"
                        + EALYSUSPEND_TIME);

    }

    private void setAndroidState(boolean bUpDown) {
        // FileOutputStream stateOutputStream;
        try {
            LIDBG_PRINT(" shareFastBootState " + bUpDown);
            if (bUpDown) {
                writeToFile(file, "flyaudio android_up");
            } else {
                writeToFile(file, "flyaudio android_down");
            }
        } catch (Exception e) {
            Log.d(TAG, e.getMessage());
        }
    }

    public void writeToFile(String filePath, String str) {
        File mFile = new File(filePath);
        if (mFile.exists()) {
            try {
                LIDBG_PRINT(" writeToFile   new fileoutputstream+ accstate:"
                        + str);
                FileOutputStream fout = new FileOutputStream(
                        mFile.getAbsolutePath());
                byte[] bytes = str.getBytes();
                fout.write(bytes);
                fout.close();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                LIDBG_PRINT(" writeToFile  IOException " + e.getMessage());
                e.printStackTrace();
            }
        } else {
            LIDBG_PRINT("file not exists!!!");
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
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

    private static void LIDBG_PRINT(String msg) {
        Log.d(TAG, msg);
        //
        // if (0 == Settings.System.getInt(resolver, "flyaudio_debug", 0)) {
        // if (DEBUG_ONCE) {
        // return;
        // }
        // }

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
                Log.e(TAG, "Failed to lidbg_printk");
            }

        } else {
            Log.e(TAG, "/dev/lidbg_msg not exist");
        }

    }

    private void delay(int ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

}

