package com.fly.flybootservice;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.WallpaperInfo;
import android.app.WallpaperManager;
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
import android.os.IBinder;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.SystemClock;
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
import java.io.FileInputStream;
import java.io.DataInputStream;
import java.io.FileNotFoundException;
import java.util.List;
import java.io.IOException;

import java.util.List;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import java.util.ArrayList;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.widget.Toast;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;
import android.app.AlarmManager;

import android.os.Build;


import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;

import android.telephony.TelephonyManager;
/*
 * ScreenOn ScreenOff DeviceOff Going2Sleep 四种状态分别表示：1.表示正常开屏状态2.表示关屏，但没关外设的状态
 * 0'~30'的阶段3.表示关屏关外设，但没到点进入深度休眠 30'~60'的阶段4.表示发出休眠请求到执行快速休眠 60'后,即进入深度休眠
 */
public class FlyBootService extends Service {
    private static final String TAG = "boot";

    private static int FBS_SCREEN_OFF = 0;
    private static int FBS_DEVICE_DOWN = 1;
    private static int FBS_FASTBOOT_REQUEST = 2;
    private static int FBS_ANDROID_DOWN = 3;
    private static int FBS_GOTO_SLEEP = 4;
    private static int FBS_KERNEL_DOWN = 5;
    private static int FBS_KERNEL_UP = 6;
    private static int FBS_ANDROID_UP = 7;
    private static int FBS_DEVICE_UP = 8;
    private static int FBS_SCREEN_ON = 9;
    private static int FBS_SLEEP_TIMEOUT = 10;
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
    private static String pmFile = "/dev/flyaudio_pm0";

    private static String SYSTEM_RESUME = "com.flyaudio.system.resume";

    private static FlyBootService mFlyBootService;

    private PowerManager fbPm = null;
    public static WakeLock mWakeLock = null;
    public static WakeLock mBrightWakeLock = null;
    private ActivityManager mActivityManager = null;
    private PackageManager mPackageManager = null;
    private static boolean bIsKLDRunning = true;
    private static boolean sendBroadcastDone = false;
    private static boolean firstBootFlag = false;
    private boolean AirplaneEnable = false;
    private boolean booleanAccWakedupState = false;
    private static int pmState = -1;
    private int intPlatformId = 0;
    private boolean blSuspendUnairplaneFlag = false;
    private boolean blDozeModeFlag = false;

    //do not force-stop apps in list
    private String[] mWhiteList = null;
    //list who can access Internet
    private String[] mInternelWhiteList = null;
    //list about all apps'uid who request Internet permission	
    private List<Integer> mInternelAllAppListUID= new ArrayList<Integer>();
    private List<Integer> mInternelWhiteAppListUID= new ArrayList<Integer>();
    private boolean dbgMode = true;
    private boolean mFlyaudioInternetActionEn = true;
    private boolean mKillProcessEn = true;
    private Toast toast = null;
    private boolean isWifiApEnabled=false;
    private boolean isWifiEnabled=false;
    private boolean isSimCardReady=false;

    String mInternelBlackList[] = {
            "com.qti.cbwidget"
    };
    // add launcher in protected list
    String systemLevelProcess[] = {
            "com.android.flyaudioui",
            "cn.flyaudio.android.flyaudioservice",
            "cn.flyaudio.navigation", "com.android.launcher",
            "cn.flyaudio.osd.service", "android.process.acore",
            "android.process.media", "com.android.systemui",
            "com.android.deskclock", "sys.DeviceHealth", "system",
            "com.fly.flybootservice","com.android.keyguard","android.policy","com.android.launcher3",
	        "com.example.sleeptest","com.flyaudio.proxyservice","com.goodocom.gocsdk","cn.flyaudio.assistant","cn.flyaudio.handleservice"
    };

    @Override
    public void onCreate() {
        super.onCreate();
	mFlyBootService = this;
        LIDBG_PRINT("flybootservice onCreate-->start LidbgCommenLogic\n");
        Intent mIntent = new Intent();
        mIntent.setComponent(new ComponentName("com.fly.lidbg.LidbgCommenLogic","com.fly.lidbg.LidbgCommenLogic.LidbgCommenLogicService"));
        this.startService(mIntent);
        LIDBG_PRINT("flybootservice onCreate-->start H264ToMp4Service\n");
        Intent mIntent2 = new Intent();
        mIntent2.setComponent(new ComponentName("com.flyaudio.lidbg.H264ToMp4","com.flyaudio.lidbg.H264ToMp4.H264ToMp4Service"));
        this.startService(mIntent2);
        writeToFile("/dev/lidbg_pm0","flyaudio PmServiceStar");

	acquireWakeLock();
	mPackageManager = this.getPackageManager();
	fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
	mActivityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
	mWhiteList = FileReadList("/flysystem/lib/out/appProtectList.conf","\n");
	mInternelWhiteList = FileReadList("/flysystem/lib/out/appInternetProtectList.conf","\n");
	DUMP();
        LIDBG_PRINT("flybootservice start [getInternelAllAppUids]\n");
	getInternelAllAppUids(mInternelAllAppListUID);
        LIDBG_PRINT("flybootservice stop [getInternelAllAppUids]\n");
        FlyaudioBlackListInternetControl(false);
	IntentFilter filter = new IntentFilter();
	filter.addAction("android.intent.action.BOOT_COMPLETED");
	filter.addAction("com.lidbg.flybootserver.action");
	filter.addAction(Intent.ACTION_SCREEN_OFF);
	filter.addAction(Intent.ACTION_SCREEN_ON);
	filter.setPriority(Integer.MAX_VALUE);
	registerReceiver(myReceiver, filter);

		setAndaddAlarmAtTtime(cmdPara[cmdParabase][0], cmdPara[cmdParabase][1],
				cmdPara[cmdParabase][2], cmdPara[cmdParabase][3],
				cmdPara[cmdParabase][4], cmdPara[cmdParabase][5]);

	intPlatformId = SystemProperties.getInt("persist.lidbg.intPlatformId", 0);
	switch (intPlatformId) {
		case 0:	//msm7627a
		case 1:	//msm8625
		case 2:	//msm8226 Android_4.4.2
		case 3:	//msm8926 Android_4.4.4
		case 4:	//msm8974 Android_4.4.4
		case 5:	//mt3360  Android_4.2
		case 6:	//msm8226 M8626AAAAANLYD1431 Android_5.0
		case 7:	//msm8974 M8974AAAAANLYD4275 Android_5.1
		case 8:	//rk3188 Radxa Rock Pro 4.4.2
		case 9:	//rk3188 PX3 Pro 4.4.4
		case 10:	//msm8226 Android_5.1.1
		case 12:	//msm8226 Android_4.4.4
		case 13:	//A80 Android_4.4
		case 14:	//G9 Android_6.0
			blSuspendUnairplaneFlag = false;
			break;
		default:
                        blSuspendUnairplaneFlag = true;
                        reSetPmState();
			break;
	}

	if (android.os.Build.VERSION.SDK_INT >= 23)//greater then Android_6.0
		blDozeModeFlag = true;

	LIDBG_PRINT("flybootservice get:\nplatform_id: " + intPlatformId
			+ "\n SuspendUnairplane: " + blSuspendUnairplaneFlag
			+ "\n blDozeModeFlag: " + blDozeModeFlag
			+ "\n Build.VERSION.SDK_INT: " + android.os.Build.VERSION.SDK_INT
			+ "\n Build.VERSION.RELEASE: " + android.os.Build.VERSION.RELEASE);

        new Thread() {
            @Override
            public void run() {
					while(true){
						pmState = readFromFile(pmFile);
						if(pmState < 0)
						{
							LIDBG_PRINT("FlyBootService get pm state failed.");
							delay(500);
						}
						else{
							if(pmState == FBS_SCREEN_OFF){
								isSimCardReady=isSimCardReady();
								LIDBG_PRINT("FlyBootService get pm state: FBS_SCREEN_OFF/isSimCardReady:"+isSimCardReady+"\n");
								previousACCOffTime = SystemClock.elapsedRealtime();
								SendBroadcastToService(KeyBootState, keyScreenOFF);
							}else if(pmState == FBS_DEVICE_DOWN){
								LIDBG_PRINT("FlyBootService get pm state: FBS_DEVICE_DOWN\n");
								if(!blDozeModeFlag)
									FlyaudioInternetDisable();
								AirplaneEnable = SystemProperties.getBoolean("persist.lidbg.AirplaneEnable",false);
								LIDBG_PRINT("FlyBootService FBS_DEVICE_DOWN:isSimCardReady:"+isSimCardReady+"/AirplaneEnable:"+AirplaneEnable+"/blSuspendUnairplaneFlag:"+blSuspendUnairplaneFlag+"\n");
								if((AirplaneEnable) || (!blSuspendUnairplaneFlag)||!isSimCardReady){
									LIDBG_PRINT("FlyBootService device down enable AirplaneMode\n");
									enterAirplaneMode();
								}else
									LIDBG_PRINT("FlyBootService device down disable AirplaneMode\n");
								SendBroadcastToService(KeyBootState, keyEearlySusupendOFF);
								LIDBG_PRINT("FlyBootService sent device_down to hal\n");
								isWifiApEnabled = isWifiApEnabled();
								isWifiEnabled = isWifiEnabled();
								LIDBG_PRINT("FlyBootService isWifiApEnabled:"+isWifiApEnabled+"/isWifiEnabled:"+isWifiEnabled+"\n");
								if (isWifiApEnabled)
								{
									setWifiApState(false);
								}
								if (isWifiEnabled)
								{
									setWifiState(false);
								}
							}else if(pmState == FBS_FASTBOOT_REQUEST){
								LIDBG_PRINT("FlyBootService get pm state: FBS_FASTBOOT_REQUEST\n");
							}else if(pmState == FBS_ANDROID_DOWN){
								LIDBG_PRINT("FlyBootService get pm state: FBS_ANDROID_DOWN\n");
								SendBroadcastToService(KeyBootState, keyFastSusupendOFF);
								start_fastboot();
							}else if(pmState == FBS_GOTO_SLEEP){
								LIDBG_PRINT("FlyBootService get pm state: FBS_GOTO_SLEEP\n");
								//if(blSuspendUnairplaneFlag)
								//	KillProcess();
								system_gotosleep();
							}else if(pmState == FBS_KERNEL_DOWN){
								LIDBG_PRINT("FlyBootService get pm state: FBS_KERNEL_DOWN\n");
							}else if(pmState == FBS_KERNEL_UP){
								LIDBG_PRINT("FlyBootService get pm state: FBS_KERNEL_UP\n");
								if(blSuspendUnairplaneFlag){
									fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
									fbPm.wakeUp(SystemClock.uptimeMillis());
								}
							}else if(pmState == FBS_ANDROID_UP){
								LIDBG_PRINT("FlyBootService get pm state: FBS_ANDROID_UP\n");
								SendBroadcastToService(KeyBootState, keyFastSusupendON);
								sendBroadcast(new Intent(SYSTEM_RESUME));
								Intent intentBoot = new Intent(Intent.ACTION_BOOT_COMPLETED);
								intentBoot.putExtra("flyauduio_accon", "accon");
								sendBroadcast(intentBoot);
							}else if(pmState == FBS_DEVICE_UP){
								LIDBG_PRINT("FlyBootService get pm state: FBS_DEVICE_UP\n");
								LIDBG_PRINT("FlyBootService FBS_DEVICE_UP:isSimCardReady:"+isSimCardReady+"/AirplaneEnable:"+AirplaneEnable+"/blSuspendUnairplaneFlag:"+blSuspendUnairplaneFlag+"\n");
								if((AirplaneEnable) || (!blSuspendUnairplaneFlag)||!isSimCardReady)
									restoreAirplaneMode(mFlyBootService);
								SendBroadcastToService(KeyBootState, keyEearlySusupendON);
								InternetEnable();
								if(!blDozeModeFlag)
									FlyaudioInternetEnable();
								if (isWifiApEnabled)
								{
									setWifiApState(true);
								}
								if (isWifiEnabled)
								{
									setWifiState(true);
								}
							}else if(pmState == FBS_SCREEN_ON){
								LIDBG_PRINT("FlyBootService get pm state: FBS_SCREEN_ON");
								acquireWakeLock();
								SendBroadcastToService(KeyBootState, keyScreenOn);
								system_resume();
							}else if(pmState == FBS_SLEEP_TIMEOUT){
								LIDBG_PRINT("FlyBootService get pm state: FBS_SLEEP_TIMEOUT");
								if(blSuspendUnairplaneFlag)
									KillProcess(true);
								InternetDisable();
							}else
								LIDBG_PRINT("FlyBootService undefined pm state: " + pmState);
						}
					}
            }
        }.start();

    }

	public void reSetPmState () {
		LIDBG_PRINT("flybootservice reset PM state.");
		FBS_SCREEN_OFF = 0;
		FBS_GOTO_SLEEP = 1;
		FBS_DEVICE_DOWN = 2;
		FBS_FASTBOOT_REQUEST = 3;
		FBS_ANDROID_DOWN = 4;
		FBS_SLEEP_TIMEOUT = 5;
		FBS_KERNEL_DOWN = 6;
		FBS_KERNEL_UP = 7;
		FBS_ANDROID_UP = 8;
		FBS_DEVICE_UP = 9;
		FBS_SCREEN_ON = 10;
	}

	public void showToastQuick(String toast_string)
	{
		// TODO Auto-generated method stub
		if (toast_string != null)
		{
			if (toast == null)
			{
				toast = Toast.makeText(this, toast_string,
						Toast.LENGTH_LONG);
			} else
			{
				toast.setText(toast_string);
			}
			toast.show();
		}
	}

	//am broadcast -a com.lidbg.flybootserver.action --ei action 0
	private BroadcastReceiver myReceiver = new BroadcastReceiver()
	{
		@Override
		public void onReceive(Context context, Intent intent)
		{
			if ( intent == null)
			{
				LIDBG_PRINT("err.return:intent == null \n");
				return;
			}

			LIDBG_PRINT("flybootserver.BroadcastReceiver:["+intent.getAction()+"]\n");
			if (intent.getAction().equals("android.intent.action.BOOT_COMPLETED"))
			{
				acquireBrightWakeLock();
				writeToFile("/dev/lidbg_interface", "BOOT_COMPLETED");
				return;
			}
			else  if (intent.getAction().equals(Intent.ACTION_SCREEN_ON))
			{
				acquireBrightWakeLock();
				return;
			}
			else  if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF))
			{
				releaseBrightWakeLock();
				return;
			}


			if (intent.hasExtra("toast"))
			{
				String toastString = intent.getExtras().getString("toast");
				showToastQuick(toastString);
				LIDBG_PRINT("BroadcastReceiver.toast:["+toastString+"].return\n");
				return;
			}
			
			if ( !intent.hasExtra("action"))
			{
				LIDBG_PRINT("err.return:!intent.hasExtra(\"action\")\n");
				return;
			}
			int action = intent.getExtras().getInt("action");
			LIDBG_PRINT("BroadcastReceiver.action:"+action+"\n");
			switch (action)
			{
			case 0:
				FlyaudioInternetDisable();
			break;
			case 1:
				FlyaudioInternetEnable();
			break;
			case 2:
				dbgMode=!dbgMode;
				LIDBG_PRINT("dbgMode->"+ dbgMode+"\n");
			break;
			case 3:
				DUMP();
			break;
			case 4:
				FlyaudioWhiteListInternetEnable(false);
				LIDBG_PRINT("FlyaudioWhiteListInternetEnable(false)\n");
			break;
			case 5:
				FlyaudioWhiteListInternetEnable(true);
				LIDBG_PRINT("FlyaudioWhiteListInternetEnable(true)\n");
			break;
			case 6:
				mFlyaudioInternetActionEn=false;
				LIDBG_PRINT("mFlyaudioInternetActionEn->"+ mFlyaudioInternetActionEn+"\n");
			break;
			case 7:
				mFlyaudioInternetActionEn=true;
				LIDBG_PRINT("mFlyaudioInternetActionEn->"+ mFlyaudioInternetActionEn+"\n");
			break;
			case 8:
				mKillProcessEn=false;
				LIDBG_PRINT("mKillProcessEn->"+ mKillProcessEn+"\n");
			break;
			case 9:
				InternetDisable();
				LIDBG_PRINT("InternetDisable()\n");
			break;
			case 10:
				InternetEnable();
				LIDBG_PRINT("InternetEnable()\n");
			break;
			case 11:
			break;
			case 12:
			break;
			case 13:
				FlyaudioBlackListInternetControl(true);
			break;
			case 14:
				LIDBG_PRINT("start  test mode:setAndaddAlarmAtTtime\n");
				cmdParabase = 2;
				setAndaddAlarmAtTtime(cmdPara[cmdParabase][0], cmdPara[cmdParabase][1],
				cmdPara[cmdParabase][2], cmdPara[cmdParabase][3],
				cmdPara[cmdParabase][4], cmdPara[cmdParabase][5]);
				handleRebootEvent();
			break;
			case 15:
				LIDBG_PRINT("FlyBootService isWifiApEnabled:"+isWifiApEnabled()+"\n");
			break;
			case 16:
				LIDBG_PRINT("FlyBootService enableWifiApState:"+setWifiApState(true)+"\n");
			break;
			case 17:
				LIDBG_PRINT("FlyBootService disableWifiApState:"+setWifiApState(false)+"\n");
			break;
			case 18:
				LIDBG_PRINT("FlyBootService isWifiEnabled:"+isWifiEnabled()+"\n");
			break;
			case 19:
				LIDBG_PRINT("FlyBootService enableWiFi:"+setWifiState(true)+"\n");
			break;
			case 20:
				LIDBG_PRINT("FlyBootService disableWiFi:"+setWifiState(false)+"\n");
			break;
								
			default:
			LIDBG_PRINT("BroadcastReceiver.action:unkown"+action+"\n");
			break;
			}
			// TODO Auto-generated method stub
		}

	};
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
        LIDBG_PRINT("flybootservice destory...");
    }

    public void acquireWakeLock() {
        if (mWakeLock == null) {
            LIDBG_PRINT(" +++++ acquire flybootservice wakelock +++++ ");
            fbPm= (PowerManager) getSystemService(Context.POWER_SERVICE);

            mWakeLock = (WakeLock) fbPm.newWakeLock(
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

public void acquireBrightWakeLock()
{
    if (mBrightWakeLock == null)
    {
        fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
	mBrightWakeLock = (WakeLock) fbPm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "lidbg.bright.wakelock");
        if (mBrightWakeLock != null)
        {
            LIDBG_PRINT(" ----- acquireBrightWakeLock ----- ");
            mBrightWakeLock.acquire();
        }
        else
            LIDBG_PRINT(" Error: acquireBrightWakeLock");
    }
}
public static void releaseBrightWakeLock()
{
    if (mBrightWakeLock != null )
    {
        LIDBG_PRINT(" ----- releaseBrightWakeLock ----- ");
        mBrightWakeLock.release();
        if(mBrightWakeLock.isHeld())
            LIDBG_PRINT(" Error: releaseBrightWakeLock !");
        mBrightWakeLock = null;
    }
}


    public static void restoreAirplaneMode(Context context) {
        LIDBG_PRINT("restoreAirplaneMode+");
        if (Settings.Global.getInt(context.getContentResolver(), "fastboot_airplane_mode", -1) != 0) {
            return;
        }

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

    public void SendBroadcastToService(String key, String value) {
        // Log.d(TAG, " PowerBundle :  " + value);
        Intent intent = new Intent(action);
        Bundle bundle = new Bundle();
        bundle.putString(key, value);
        intent.putExtra(PowerBundle, bundle);
        sendBroadcast(intent);
    }

	private void system_gotosleep(){

		LIDBG_PRINT(" ********** system gotosleep ********** ");
		if(blSuspendUnairplaneFlag){
			fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
			msgTokenal("flyaudio gotosleep");
			fbPm.goToSleep(SystemClock.uptimeMillis());
		}else{
			fbPm.goToSleep(SystemClock.uptimeMillis());
			msgTokenal("flyaudio gotosleep");
		}
		releaseWakeLock();
	}

	private void start_fastboot(){
		firstBootFlag = true;

		LIDBG_PRINT(" ********** start fastboot ********** ");
		fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);

		powerOffSystem();
	}

	private void system_resume(){
		if(firstBootFlag){
			LIDBG_PRINT("FlyBootService system resume...");
			enableShowLogo(true);
			powerOnSystem(mFlyBootService);
		}
	}

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
		LIDBG_PRINT("powerOffSystem+");
		sendBecomingNoisyIntent();
		LIDBG_PRINT("powerOffSystem step 1");

		SystemProperties.set("ctl.start", "bootanim");
		LIDBG_PRINT("powerOffSystem step 2");

		LIDBG_PRINT("powerOffSystem step 3");
		getLastPackage();
		bIsKLDRunning = IsKLDrunning();

		if (bIsKLDRunning) {
		SystemProperties.set("fly.gps.run", "1");
			Log.d(TAG, "-----fly.gps.run----1----");
		} else {
			SystemProperties.set("fly.gps.run", "0");
			Log.d(TAG, "-----fly.gps.run-----0---");
		}
		LIDBG_PRINT("powerOffSystem step 4");
		//if(!blSuspendUnairplaneFlag)
			KillProcess(false);
		msgTokenal("flyaudio pre_gotosleep");
		LIDBG_PRINT("powerOffSystem-");
    }

    private void powerOnSystem(Context context) {
	new Thread(new Runnable()
	{

		@Override
		public void run()
		{
			// TODO Auto-generated method stub
			int cnt = 0;
			LIDBG_PRINT("powerOnSystem+\n");
			//if(!blSuspendUnairplaneFlag)
			//	restoreAirplaneMode(context);
			while((SystemProperties.getBoolean("lidbg.hold_bootanim", false)||SystemProperties.getBoolean("lidbg.hold_bootanim2", false)) && (cnt < 50))
			{
				SystemClock.sleep(100);
				cnt ++;
				if(cnt%10==0)
					LIDBG_PRINT(cnt+"hold_bootanim2.stop,["+SystemProperties.getBoolean("lidbg.hold_bootanim", false)+"/"+SystemProperties.getBoolean("lidbg.hold_bootanim2", false)+"]\n");
			}
			SystemProperties.set("ctl.stop", "bootanim");
			LIDBG_PRINT(cnt+"real.hold_bootanim2.stop,["+SystemProperties.getBoolean("lidbg.hold_bootanim", false)+"/"+SystemProperties.getBoolean("lidbg.hold_bootanim2", false)+"]\n");
		}
	}, "waitBootanima").start();
    }

    // send broadcast to music application to pause music
    private void sendBecomingNoisyIntent() {
        sendBroadcast(new Intent(AudioManager.ACTION_AUDIO_BECOMING_NOISY));
    }

    private void KillProcess(boolean mWhiteListKillEn) {
	if(!mKillProcessEn)
	{
		LIDBG_PRINT("skip KillProcess.mKillProcessEn=false\n");
		return;
	}
	acquireWakeLock();
        List<ActivityManager.RunningAppProcessInfo> appProcessList = null;

        appProcessList = mActivityManager.getRunningAppProcesses();
	delay(200);
        LIDBG_PRINT("begin to KillProcess."+(mWhiteList == null)+"/"+mWhiteListKillEn+"\n");
        for (ActivityManager.RunningAppProcessInfo appProcessInfo : appProcessList) {
            int pid = appProcessInfo.pid;
            int uid = appProcessInfo.uid;
            String processName = appProcessInfo.processName;
            if(blSuspendUnairplaneFlag){
	            booleanAccWakedupState = SystemProperties.getBoolean("persist.lidbg.AccWakedupState",false);
	            if(booleanAccWakedupState){
	                LIDBG_PRINT("Prop AccWakedupState be set:" + booleanAccWakedupState + ", stop kill process.");
	                break;
	            }
            }
            if (isKillableProcess(processName)) {
                LIDBG_PRINT(processName +"."+pid +" will be killed\n");
                mActivityManager.forceStopPackage(processName);
            }
	else if (mWhiteListKillEn&&processName.contains("flyaudio")) 
		{
			LIDBG_PRINT(processName +"."+pid +" will be killed in white list\n");
			mActivityManager.forceStopPackage(processName);
		}
        }
    releaseWakeLock();
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

    public static boolean isAirplaneModeOn(Context context) {
        return Settings.Global.getInt(context.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
    }
    
    private void enterAirplaneMode() 
   {
	LIDBG_PRINT("Flyaudio Remote-Control disabled, AirplaneEnable:::"+AirplaneEnable);
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

	public int readFromFile(String fileName)
	{
		try
		{
			int temp = -1;
			DataInputStream dis = new DataInputStream(new FileInputStream(
					new File(fileName)));
			temp = dis.readUnsignedByte();
			dis.close();
			return temp;
		} catch (FileNotFoundException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
			return -1;
		} catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
			return -1;
		}
	}

    public void writeToFile(String filePath, String str) {
        File mFile = new File(filePath);
        if (mFile.exists()) {
            try {
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
	public void FlyaudioInternetEnable()
	{
	    LIDBG_PRINT("FlyaudioInternetEnable:"+mFlyaudioInternetActionEn+"\n");
	    if(!mFlyaudioInternetActionEn)
	    	    return;
	    appInternetControl(true);
	}
	public void FlyaudioInternetDisable()
	{
	    LIDBG_PRINT("FlyaudioInternetDisable:"+mFlyaudioInternetActionEn+"\n");
	    if(!mFlyaudioInternetActionEn)
	    	    return;
	    appInternetControl(false);
	}
	public void FlyaudioWhiteListInternetEnable(boolean enable)
	{
	    LIDBG_PRINT("FlyaudioWhiteListInternetEnable:"+enable+"\n");
		if (mInternelWhiteAppListUID!= null)
		{
			for (int i = 0; i < mInternelWhiteAppListUID.size(); i++)
			{
				Integer uid = mInternelWhiteAppListUID.get(i);	            
				LIDBG_PRINT("appInternetControl.exe.whitelist:" +(enable ? "enable/" : "disable/")+ i + "-->" + uid + "/" +  mPackageManager.getNameForUid(uid) + "\n");
				// -o rmnet+
				InternetControlUID(enable,uid);
			}
		}
	}
	public void FlyaudioBlackListInternetControl(boolean enable)
	{
	    LIDBG_PRINT("FlyaudioBlackListInternetControl:"+enable+"\n");
		if (mInternelBlackList!= null)
		{
			for (int i = 0; i < mInternelBlackList.length; i++)
			{
			ApplicationInfo info =getApplicationInfo(mInternelBlackList[i]);
				if (info!=null)
				{
					LIDBG_PRINT("appInternetControl.exe.BlackList:" +(enable ? "enable/" : "disable/")+ i + "-->" + info.uid + "/" +  info.packageName + "\n");
					InternetControlUID(false,info.uid);
				}
			}
		}
	}
	public void InternetDisable()
	{
		LIDBG_PRINT("InternetDisable:"+mFlyaudioInternetActionEn+"\n");
		if(!mFlyaudioInternetActionEn)
		return;
		writeToFile("/dev/lidbg_misc0","flyaudio:iptables -t filter -P OUTPUT DROP");
		writeToFile("/dev/lidbg_misc0","flyaudio:iptables -t filter -P INPUT DROP");
		writeToFile("/dev/lidbg_misc0","flyaudio:iptables -t filter -P FORWARD DROP");
	}

	public void InternetEnable()
	{
		LIDBG_PRINT("InternetEnable:"+mFlyaudioInternetActionEn+"\n");
		if(!mFlyaudioInternetActionEn)
		return;
		writeToFile("/dev/lidbg_misc0", "flyaudio:iptables -t filter -P OUTPUT ACCEPT");
		writeToFile("/dev/lidbg_misc0","flyaudio:iptables -t filter -P INPUT ACCEPT");
		writeToFile("/dev/lidbg_misc0","flyaudio:iptables -t filter -P FORWARD ACCEPT");
	}

	public void InternetControlUID(boolean enable,Integer uid )
	{
		writeToFile("/dev/lidbg_misc0", "flyaudio:iptables " + (enable ? "-D" : "-I") + " FORWARD  -m owner --uid-owner " + uid	+ " -j REJECT");
		writeToFile("/dev/lidbg_misc0", "flyaudio:iptables " + (enable ? "-D" : "-I") + " OUTPUT  -m owner --uid-owner " + uid + " -j REJECT");
		writeToFile("/dev/lidbg_misc0", "flyaudio:iptables " + (enable ? "-D" : "-I") + " INPUT  -m owner --uid-owner " + uid + " -j REJECT");
	}
	public List<Integer> getInternelAllAppUids(List<Integer> mlist)
	{
	    List<PackageInfo> packinfos = mPackageManager.getInstalledPackages(0);
	    int i = 0,j=0;
	for (PackageInfo info : packinfos)
	    {
	    	if ((info.applicationInfo.packageName.contains("flyaudio"))||((info.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) <= 0))
	                {
	                    int uid = info.applicationInfo.uid;
	                    if (isPackageInWhiteList(info.applicationInfo.packageName))
	                    {
				i++;
				LIDBG_PRINT("appInternetControl.protect:" + i + "-->" + uid + "/" +  info.applicationInfo.packageName + "\n");
				if (mInternelWhiteAppListUID!= null && !mInternelWhiteAppListUID.contains(uid))
				{
					mInternelWhiteAppListUID.add(uid);
				}
				continue;
	                    }
	                    j++;
	           	 //if (dbgMode)
	           	 //LIDBG_PRINT("appInternetControl.prepare:" + j + "-->" + uid + "/" +  info.applicationInfo.packageName + "\n");
	                    if (mlist != null && !mlist.contains(uid))
	                    {
	                        mlist.add(uid);
	                    }
	                }
	    }
	    return mlist;
	}
	
	public Boolean isPackageInWhiteList(String pkg)
	{
		if (mInternelWhiteList != null)
		{
			for (int i = 0; i < mInternelWhiteList.length; i++)
			{
				if (pkg.equals(mInternelWhiteList[i]))
				{
					return true;
				}
			}
		}
		return false;
	}
	public void appInternetControl(boolean enable)
	{
	    // TODO Auto-generated method stub
	    mInternelAllAppListUID = getInternelAllAppUids(mInternelAllAppListUID);

	    LIDBG_PRINT("appInternetControl:" + mInternelAllAppListUID.size() +"\n");
	    if (mInternelAllAppListUID != null && mInternelAllAppListUID.size() > 0)
	    {
	        for (int i = 0; i < mInternelAllAppListUID.size(); i++)
	        {
	            Integer uid = mInternelAllAppListUID.get(i);	            
	            if (uid==1000)
		   continue;
	            if (dbgMode)
	            LIDBG_PRINT("appInternetControl.exe:" + i + "-->" + uid + "/" +  mPackageManager.getNameForUid(uid) + "\n");
	            // -o rmnet+
	            InternetControlUID(enable,uid);
	        }

	    }
	    else
	        LIDBG_PRINT("mInternelAllAppList == null || mInternelAllAppList.size() < 0");
	}
	private ApplicationInfo getApplicationInfo(String pkg)
	{
		ApplicationInfo ai = null;
		// TODO Auto-generated method stub
		try
		{
			ai = mPackageManager.getApplicationInfo(pkg, 0);
		} catch (NameNotFoundException e)
		{
			e.printStackTrace();
		}
		return ai;
	}
	public void DUMP()
	{
		if (mWhiteList != null)
		{
			for (int i = 0; i < mWhiteList.length; i++)
			{
				LIDBG_PRINT(i +"->"+ mWhiteList[i]+"\n");
			}
		}
		else
			LIDBG_PRINT("mWhiteList = null\n");
		if (mInternelWhiteList != null)
		{
			for (int i = 0; i < mInternelWhiteList.length; i++)
			{
				LIDBG_PRINT(i +"->"+ mInternelWhiteList[i]+"\n");
			}
		}
		else
			LIDBG_PRINT("mInternelWhiteList = null\n");
	}
	
	protected boolean isSimCardReady()
	{
		// TODO Auto-generated method stub
		TelephonyManager mTelephonyManager = (TelephonyManager) mFlyBootService
				.getSystemService(Context.TELEPHONY_SERVICE);
		return (mTelephonyManager.getSimState() == TelephonyManager.SIM_STATE_READY);
	}
///////////////////////////////////
	public boolean setWifiApState(boolean enable)
	{
		String msg = "info:";
		// TODO Auto-generated method stub
		LIDBG_PRINT("FlyBootService setWifiApState:"+enable+"\n");
		try
		{
			WifiManager mWifiManager = (WifiManager) mFlyBootService
					.getSystemService(Context.WIFI_SERVICE);
			Method method = mWifiManager.getClass().getMethod(
					"getWifiApConfiguration");
			method.setAccessible(true);
			WifiConfiguration config = (WifiConfiguration) method
					.invoke(mWifiManager);
			Method method2 = mWifiManager.getClass().getMethod(
					"setWifiApEnabled", WifiConfiguration.class, boolean.class);
			method2.invoke(mWifiManager, config, enable);
			LIDBG_PRINT("FlyBootService setWifiApState:exe suncess\n");
			return true;
		} catch (NoSuchMethodException e)
		{
			// TODO Auto-generated catch block
			msg = e.getMessage();
		} catch (IllegalArgumentException e)
		{
			// TODO Auto-generated catch block
			msg = e.getMessage();
		} catch (IllegalAccessException e)
		{
			// TODO Auto-generated catch block
			msg = e.getMessage();
		} catch (InvocationTargetException e)
		{
			// TODO Auto-generated catch block
			msg = e.getMessage();
		}
		LIDBG_PRINT("FlyBootService setWifiApState:exe error:"+msg+"\n");
		return false;
	}

	public boolean isWifiApEnabled()
	{
		String msg = "info:";
		// TODO Auto-generated method stub
		try
		{
			WifiManager mWifiManager = (WifiManager) mFlyBootService
					.getSystemService(Context.WIFI_SERVICE);
			Method method = mWifiManager.getClass()
					.getMethod("isWifiApEnabled");
			method.setAccessible(true);
			LIDBG_PRINT("FlyBootService isWifiApEnabled:exe suncess\n");
			return (Boolean) method.invoke(mWifiManager);
		} catch (NoSuchMethodException e)
		{
			msg = e.getMessage();
		} catch (Exception e)
		{
			msg = e.getMessage();
		}
		LIDBG_PRINT("FlyBootService isWifiApEnabled:exe error:"+msg+"\n");
		return false;
	}
	private boolean isWifiEnabled()
	{
		// TODO Auto-generated method stub
		WifiManager mWifiManager = (WifiManager) mFlyBootService
				.getSystemService(Context.WIFI_SERVICE);
		return mWifiManager.isWifiEnabled();
	}

	public boolean setWifiState(boolean enable)
	{
		// TODO Auto-generated method stub
		WifiManager mWifiManager = (WifiManager) mFlyBootService
				.getSystemService(Context.WIFI_SERVICE);
		LIDBG_PRINT("FlyBootService setWifiState:"+enable+"\n");
		if (enable)
		{
			return mWifiManager.setWifiEnabled(true);
		} else
		{
			return mWifiManager.setWifiEnabled(false);
		}
	}
/////////////////////////////alarm added below/////////////////////////////////////////
	protected long[][] cmdPara =
	{
			// repearAlarm/intervalDate/absolutelyHour/absolutelyMinutes/absolutelySeconds/repeatIntervalTimeInMillis
			// factory para,cmdParabase = 0
			{ 1, 6, 23, 0, 0, 24 * 60 * 60 * 1000 },
			{ 1, 1, 3, 0, 0, 24 * 60 * 60 * 1000 },
			// debug para below up,cmdParabase = 2
			{ 1, 0, 22, 0, 0, 24 * 60 * 60 * 1000 },
			{ 1, 1, 6, 0, 0, 1 * 60 * 60 * 1000 }, };
	protected long oldTimes;
	private int cmdParabase = 0;
	protected int loopCount = 0;
	private long previousACCOffTime = SystemClock.elapsedRealtime();
	private PendingIntent peration;
	private AlarmManager mAlarmManager;
	private BroadcastReceiver mAlarmBroadcast = new BroadcastReceiver()
	{
		private long interval = 0;

		@Override
		public void onReceive(Context arg0, Intent arg1)
		{
			// TODO Auto-generated method stub
			loopCount++;
			String msg = arg1.getStringExtra("msg");
			interval = SystemClock.elapsedRealtime() - oldTimes;
			oldTimes = SystemClock.elapsedRealtime();
			String log = msg + "\n" + getCurrentTimeString() + "\nloopCount:"
					+ loopCount + "/" + "interval:" + interval / 1000 + "S\n";
			LIDBG_PRINT(log);
			if (loopCount == 1)
			{
				setAndaddAlarmAtTtime(cmdPara[cmdParabase + 1][0],
						cmdPara[cmdParabase + 1][1],
						cmdPara[cmdParabase + 1][2],
						cmdPara[cmdParabase + 1][3],
						cmdPara[cmdParabase + 1][4],
						cmdPara[cmdParabase + 1][5]);
			}
			if (loopCount >= 2)
			{
				handleRebootEvent();
			}
		}
	};

	private void setAndaddAlarmAtTtime(long repearAlarm, long intervalDate,
			long absolutelyHour, long absolutelyMinutes,
			long absolutelySeconds, long repeatIntervalTimeInMillis)
	{
		// TODO Auto-generated method stub
		LIDBG_PRINT("salarm.=====add new alarm=======\nrepeatIntervalTimeInMillis:"
						+ repeatIntervalTimeInMillis / 1000 + "S\n"+"cmdParabase:"+cmdParabase+ "\n");
		oldTimes = SystemClock.elapsedRealtime();

		Intent intent = new Intent("LIDBG_ALARM_REBOOT");
		intent.putExtra("msg", "salarm=========LIDBG_ALARM_REBOOT===========\n");
		peration = PendingIntent.getBroadcast(this, 0, intent, 0);

		// get AlarmManager
		mAlarmManager = (AlarmManager) this
				.getSystemService(Context.ALARM_SERVICE);
		mAlarmManager.cancel(peration);
		if (repearAlarm == 1)
		{
			mAlarmManager.setRepeating(
					AlarmManager.ELAPSED_REALTIME_WAKEUP,
					SystemClock.elapsedRealtime()
							+ getFutureCalenderTimeInMillis(intervalDate,
									absolutelyHour, absolutelyMinutes,
									absolutelySeconds),
					repeatIntervalTimeInMillis, peration);
		} else
		{
			mAlarmManager.set(
					AlarmManager.ELAPSED_REALTIME_WAKEUP,
					SystemClock.elapsedRealtime()
							+ getFutureCalenderTimeInMillis(intervalDate,
									absolutelyHour, absolutelyMinutes,
									absolutelySeconds), peration);
		}

		registerReceiver(mAlarmBroadcast,
				new IntentFilter("LIDBG_ALARM_REBOOT"));
	}

	protected String getCurrentTimeString()
	{
		// TODO Auto-generated method stub
		DateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date curDate = new Date(System.currentTimeMillis());
		return df.format(curDate);
	}

	protected void handleRebootEvent()
	{
		// TODO Auto-generated method stub
		long intervalTimesS = (SystemClock.elapsedRealtime() - previousACCOffTime) / 1000;
		Date curDate = new Date(System.currentTimeMillis());
		int curHours = curDate.getHours();
		int accState = SystemProperties.getInt("persist.lidbg.acc.status", 0);// o:ACC on , 1:ACC off
		String logString ="salarm.handleRebootEvent"
			+"\ncurHours:"+ curHours
			+"\npurposeHours:"+cmdPara[cmdParabase + 1][2]
			+"\naccState:"+accState
			+"\ncmdParabase:"+cmdParabase
			+ "\nintervalTimesS:" + intervalTimesS	+ "\n";
		LIDBG_PRINT(logString);
		if (curHours == cmdPara[cmdParabase + 1][2]&&accState==1&& intervalTimesS > 2 * 60 * 60)// 
		{
			//if(cmdParabase==0)
				writeToFile("/dev/lidbg_misc0","flyaudio:reboot lidbg_sevendays_timeout");
			//else
			//	LIDBG_PRINT("salarm:should be reboot but in debug mode.");
		}
	}

	private long getFutureCalenderTimeInMillis(long intervalDate,
			long absolutelyHour, long absolutelyMinutes, long absolutelySeconds)
	{
		// TODO Auto-generated method stub
		DateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
		Date curDate = new Date(System.currentTimeMillis());

		Calendar calendar = Calendar.getInstance();
		TimeZone tm = TimeZone.getTimeZone("GMT");
		calendar.setTimeZone(tm);
		calendar.clear();

		String currentTime = df.format(curDate);

		curDate.setDate(curDate.getDate() + (int) intervalDate);
		curDate.setHours((int) absolutelyHour);
		curDate.setMinutes((int) absolutelyMinutes);
		curDate.setSeconds((int) absolutelySeconds);

		calendar.setTime(curDate);
		String futureTime = df.format(curDate);
		long intervalTime = calendar.getTimeInMillis()
				- System.currentTimeMillis();

		String logString = "\nsalarm.currentTime:" + currentTime + "\nfutureTime:"
				+ futureTime + "\nintervalTime:" + intervalTime / 1000
				+ "\ncurDate.getHours():" + curDate.getHours()+"\n";
		LIDBG_PRINT(logString);
		return intervalTime;
	}
}

