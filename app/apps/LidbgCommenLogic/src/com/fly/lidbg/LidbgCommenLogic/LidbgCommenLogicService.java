//package com.mypftf.callmessage;

package com.fly.lidbg.LidbgCommenLogic;

import android.os.PowerManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.os.storage.StorageEventListener;
import android.content.ComponentName;
import android.os.Build;
import android.os.Environment;
import android.os.storage.IMountService;
import android.os.ServiceManager;
import android.os.IBinder;
import android.os.RemoteException;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.bluetooth.BluetoothAdapter;

import java.io.File;
import java.io.FileOutputStream;
import java.util.List;
import java.io.IOException;
import java.util.List;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PermissionInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.media.MediaPlayer;

import java.util.ArrayList;

public class LidbgCommenLogicService extends Service
{

    protected static final String ACCProperties = "persist.lidbg.acc.status";
    protected static final String GrantDone = "persist.lidbg.grant.done";
    private LidbgCommenLogicService mLidbgCommenLogicService;
    private PendingIntent peration;
    protected int loopCount = 0;
    private Context mContext;
    private IMountService mMountService;
    public static String ISRDecodeAction = "cn.flyaudio.updateapp";
    private String[] mWhitePermissionList = null;

    String mDangerlPermission[] =
    {
    };

    @Override
    public void onCreate()
    {
        super.onCreate();
        mContext = getApplication();
        printKernelMsg("onCreate");

        mLidbgCommenLogicService = this;

        if(isFileExist("/flysystem/flytheme/config/appWhitePermissionList.conf"))
        {
            mWhitePermissionList = FileReadList("/flysystem/flytheme/config/appWhitePermissionList.conf", "\n");
            printKernelMsg("use flytheme");
        }
        else
        {
            mWhitePermissionList = FileReadList("/flysystem/lib/out/appWhitePermissionList.conf", "\n");
            printKernelMsg("use default");
        }

        DUMP();

        IntentFilter filter = new IntentFilter();
        filter.addAction("android.intent.action.BOOT_COMPLETED");
        filter.addAction("com.fly.lidbg.LidbgCommenLogic");
        //filter.addAction(Intent.ACTION_SCREEN_OFF);
        //filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.addAction(Intent.ACTION_MEDIA_SCANNER_STARTED);
        filter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        filter.addAction(ISRDecodeAction);
        filter.setPriority(Integer.MAX_VALUE);
        mLidbgCommenLogicService.registerReceiver(myReceiver, filter);

        IntentFilter mIntentFilter = new IntentFilter();
        mIntentFilter.addDataScheme("package");
        mIntentFilter.addAction(Intent.ACTION_PACKAGE_ADDED);
        mIntentFilter.addAction(Intent.ACTION_PACKAGE_REPLACED);
        //mIntentFilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        mIntentFilter.setPriority(Integer.MAX_VALUE);
        mLidbgCommenLogicService.registerReceiver(mPackageReceiver, mIntentFilter);

        IntentFilter mMediaFilter = new IntentFilter();
        mMediaFilter.addDataScheme("file");
        mMediaFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        mMediaFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        mMediaFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
        mMediaFilter.addAction(Intent.ACTION_MEDIA_CHECKING);
        mMediaFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        mMediaFilter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
        mMediaFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTABLE);
        mMediaFilter.setPriority(Integer.MAX_VALUE);
        mLidbgCommenLogicService.registerReceiver(mMediaReceiver, mMediaFilter);

        StorageManager mStorageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
        mStorageManager.registerListener(mStorageEventListener);
        playMusic("/flysystem/lib/out/welcome.mp3");
    }
    private final StorageEventListener mStorageEventListener = new StorageEventListener()
    {
        @Override
        public void onStorageStateChanged(String path, String oldState, String newState)
        {
            printKernelMsg("onStorageStateChanged :" + path + " " + oldState + " -> " + newState + "\n");
            if(Build.VERSION.SDK_INT >= 23 && Environment.MEDIA_MOUNTED.equals(newState) && !path.contains("emulated") && !path.contains("sdcard") && !path.endsWith("udisk"))
            {
                printKernelMsg("skip:ln:" + path + " " + oldState + " -> " + newState + "\n");
                //FileWrite("/dev/lidbg_misc0", false, false, "flyaudio:mv /storage/udisk /storage/bl_udisk1 ");
                //printKernelMsg("ln:" + path + " " + oldState + " -> " + newState + "\n");
                //FileWrite("/dev/lidbg_misc0", false, false, "flyaudio:ln -s -f " + path + " /storage/udisk &");
            }
            if (Environment.MEDIA_MOUNTED.equals(newState))
            {
                if( !path.contains("emulated") )
                {
                    printKernelMsg("conf_check:" + path + "\n");
                    FileWrite("/dev/lidbg_misc0", false, false, "conf_check:" + path);
                }
                else
                    printKernelMsg("conf_check:.ignore." + path + "\n");
            }
            else if (Environment.MEDIA_MOUNTED.equals(oldState))
            {
            }
        }
    };
    public void DUMP()
    {
        if (mWhitePermissionList != null)
        {
            for (int i = 0; i < mWhitePermissionList.length; i++)
            {
                printKernelMsg(i + ".PEM->" + mWhitePermissionList[i] + "\n");
            }
        }
        else
            printKernelMsg("mWhitePermissionList = null\n");
    }
    protected void msleep(int i)
    {
        // TODO Auto-generated method stub
        try
        {
            Thread.sleep(i);
        }
        catch (InterruptedException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }


    protected void formatSdcard1Below6_0()
    {
        if(Build.VERSION.SDK_INT >= 23)
        {
            printKernelMsg("Build.VERSION.SDK_INT >=23.return" );
            return;
        }
        StorageVolume mStorageVolume = getVolume("sdcard1");
        if(mStorageVolume != null)
        {
            printKernelMsg("formatSdcard1Below6_0.find it->" + mStorageVolume.getPath());
            Intent intent = new Intent("com.android.internal.os.storage.FORMAT_ONLY");
            intent.setComponent(new ComponentName("android", "com.android.internal.os.storage.ExternalStorageFormatter"));
            intent.putExtra(StorageVolume.EXTRA_STORAGE_VOLUME, mStorageVolume);
            startService(intent);
        }
        else
        {
            printKernelMsg("formatSdcard1Below6_0.not find it");
        }
    }

    protected StorageVolume getVolume(String info)
    {
        StorageManager mStorageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
        final StorageVolume[] volumes = mStorageManager.getVolumeList();
        for (StorageVolume volume : volumes)
        {
            if ( volume.getPath().contains(info))
            {
                printKernelMsg("getVolume.find it->" + volume.getPath());
                return volume;
            }
            else
            {
                printKernelMsg("getVolume.not find->" + volume.getPath());
            }
        }
        return null;
    }


    private synchronized IMountService getMountService()
    {
        if (mMountService == null)
        {
            IBinder service = ServiceManager.getService("mount");
            if (service != null)
            {
                mMountService = IMountService.Stub.asInterface(service);
            }
            else
            {
                printKernelMsg("Can't get mount service");
            }
        }
        return mMountService;
    }

    protected void mountUmountSdcard1Below6_0(boolean mount)
    {
        if(Build.VERSION.SDK_INT >= 23)
        {
            printKernelMsg("Build.VERSION.SDK_INT >=23.return" );
            return;
        }
        IMountService mountService = getMountService();
        StorageVolume mStorageVolume = getVolume("sdcard1");
        if(mStorageVolume != null && mountService != null)
        {
            printKernelMsg("mountUmountSdcard1Below6_0.find it->" + mStorageVolume.getPath() + "/mount:" + mount);
            if(mount)
            {
                try
                {
                    mountService.mountVolume(mStorageVolume.getPath());
                }
                catch (RemoteException e)
                {
                    printKernelMsg("mountVolume.error:" + e.getMessage());
                }
            }
            else
            {
                try
                {
                    mountService.unmountVolume(mStorageVolume.getPath(), true, false);
                }
                catch (RemoteException e)
                {
                    printKernelMsg("unmountVolume.error:" + e.getMessage());
                }
            }
        }
        else
        {
            printKernelMsg("mountUmountSdcard1Below6_0:mStorageVolume/mountService:" + (mStorageVolume == null) + "|" + (mountService == null));
        }
    }


    protected void mountUmountUdiskUp6_0(boolean mount)
    {
        if(Build.VERSION.SDK_INT < 23)
        {
            printKernelMsg("Build.VERSION.SDK_INT <23.return" );
            return;
        }
        printKernelMsg("mountUmountUdiskUp6_0.skip" );
        /*
                try
                {
                StorageManager mStorageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
                final StorageVolume[] volumes = mStorageManager.getVolumeList();
                for (StorageVolume volume : volumes)
                {
                    if ( !volume.getPath().contains("emulated") && !volume.getPath().toUpperCase().contains("SDCARD"))
                    {
                        String mVolumeState = mStorageManager.getVolumeState(volume.getPath());
                        printKernelMsg(mount + "/mountUmountUdiskUp6_0.find it->" + volume.getPath() + "  getId:" + volume.getId() + "  getPath:" + volume.getPath() + "  mVolumeState:" + mVolumeState);
                        if(mount)
                        {
        		if(!Environment.MEDIA_MOUNTED.equals(mVolumeState))
        		mStorageManager.mount(volume.getId());
                        }
                        else
                         mStorageManager.unmount(volume.getId());
                    }
                    else
                    {
                        printKernelMsg(mount+"/mountUmountUdiskUp6_0.not target volume.retry->" + volume.getPath()+"  getId:"+volume.getId());
                    }
                }
                }
                catch (Exception e)
                {
                    printKernelMsg("mountUmountUdiskUp6_0.error:" + e.getMessage());
                }
        */
    }

    protected void wakeUpSystem()
    {
        PowerManager fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        printKernelMsg("wakeUpSystem");
        fbPm.wakeUp(SystemClock.uptimeMillis());
    }

    protected void goToSleep()
    {
        PowerManager fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        printKernelMsg("goToSleep");
        fbPm.goToSleep(SystemClock.uptimeMillis());
    }
    protected void reboot()
    {
        PowerManager fbPm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        fbPm.reboot("lidbg_logic_reboot_test");
    }

    protected void resetNetWork()
    {
        // TODO Auto-generated method stub
        printKernelMsg("data disable");
        FileWrite("/dev/lidbg_misc0", false, false,
                  "flyaudio:svc data disable &");
        msleep(5000);
        FileWrite("/dev/lidbg_misc0", false, false,
                  "flyaudio:svc data enable &");
        printKernelMsg("data enable");
    }

    protected String getCurrentTimeString()
    {
        // TODO Auto-generated method stub
        DateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        Date curDate = new Date(System.currentTimeMillis());
        return df.format(curDate);
    }

    private BroadcastReceiver mPackageReceiver = new BroadcastReceiver()
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            if (intent == null)
            {
                printKernelMsg("err.return:intent == null \n");
                return;
            }
            printKernelMsg("mPackageReceiver:[" + intent.getAction() + "]\n");
            if (intent.getAction().equals(Intent.ACTION_PACKAGE_ADDED))//|| intent.getAction().equals(Intent.ACTION_PACKAGE_REPLACED)
            {
                String packageName = intent.getData().getSchemeSpecificPart();
                if(isInList(mWhitePermissionList, packageName))
                    grantPackagePermissions(packageName, PermissionInfo.PROTECTION_DANGEROUS);
                else
                    printKernelMsg("ignore:" + packageName);
                return;
            }
        }
    };
    private BroadcastReceiver mMediaReceiver = new BroadcastReceiver()
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            if (intent == null)
            {
                printKernelMsg("err.return:intent == null \n");
                return;
            }
            printKernelMsg("mMediaReceiver:[" + intent.getAction() + "][" + intent.getData().getPath() + "]\n");
	   if(intent.getAction().equals("android.intent.action.MEDIA_MOUNTED")&&intent.getData().getPath().contains("sdcard1")&&isFileExist(intent.getData().getPath()+"/autoupdate.lidbg"))
	   {
	   	String log = getCurrentTimeString()+":start auto update\r\n";
		FileDelete(intent.getData().getPath()+"/autoupdate.lidbg");
		printKernelMsg(log);
		FileWrite(intent.getData().getPath()+"/autoupdate.log", true, true, log);
		FileWrite("/dev/lidbg_drivers_dbg0", false, false, "appcmd *158#115");
	   }
        }
    };


    // am broadcast -a com.fly.lidbg.LidbgCommenLogic --ei action 0
    private BroadcastReceiver myReceiver = new BroadcastReceiver()
    {
        private String mparaString;

        @Override
        public void onReceive(Context context, Intent intent)
        {
            if (intent == null)
            {
                printKernelMsg("err.return:intent == null \n");
                return;
            }
            printKernelMsg("BroadcastReceiver:[" + intent.getAction() + "]\n");
            if (intent.getAction().equals(
                        "android.intent.action.BOOT_COMPLETED"))
            {
                int mGrantDone = SystemProperties.getInt(GrantDone, 0);
                if(mGrantDone == 0)
                    grantWhiteListPermissions();
                else
                    printKernelMsg("ignore grantWhiteListPermissions\n");
                return;
            }
            else if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED) || intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_DETACHED))
            {
                //UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                //printKernelMsg("usb event:[Manufacturer:" + device.getManufacturerName() + "/name:" + device.getDeviceName()  + "]\n");
                //+ "/InterfaceClass:" + device.getInterface(0).getInterfaceClass()
                return;
            }
            else if (intent.getAction().equals(Intent.ACTION_AIRPLANE_MODE_CHANGED))
            {
                printKernelMsg("airplaneModeEnabled=" + intent.getBooleanExtra("state", false) + "\n");
                return;
            }
            else if (intent.getAction().equals(ISRDecodeAction))
            {
                printKernelMsg("get ISRDecodeAction\n");
                FileWrite("/dev/flydev0", false, false, "ISRDecodeAction");
                return;
            }
            else if (intent.getAction().equals(BluetoothAdapter.ACTION_STATE_CHANGED))
            {
                //printKernelMsg("BluetoothState="+intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, 0)+"\n");
                switch(intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, 0))
                {
                case BluetoothAdapter.STATE_TURNING_ON:
                    printKernelMsg("BluetoothState=STATE_TURNING_ON\n");
                    break;
                case BluetoothAdapter.STATE_ON:
                    printKernelMsg("BluetoothState=STATE_ON\n");
                    break;
                case BluetoothAdapter.STATE_TURNING_OFF:
                    printKernelMsg("BluetoothState=STATE_TURNING_OFF\n");
                    break;
                case BluetoothAdapter.STATE_OFF:
                    printKernelMsg("BluetoothState=STATE_OFF\n");
                    break;
                }
                return;
            }

            if (!intent.hasExtra("action"))
            {
                return;
            }
            int action = intent.getExtras().getInt("action");
            printKernelMsg("action:" + action + "\n");
            switch (action)
            {
            case 0:
                printKernelMsg("formatSdcard1Below6_0\n");
                formatSdcard1Below6_0();
                break;
            case 1:
                printKernelMsg("mountUmountUdiskUp6_0.mount\n");
                mountUmountUdiskUp6_0(true);
                break;
            case 2:
                printKernelMsg("mountUmountUdiskUp6_0.unmount\n");
                mountUmountUdiskUp6_0(false);
                break;
            case 3:
                printKernelMsg("wakeUpSystem\n");
                wakeUpSystem();
                break;
            case 4:
                printKernelMsg("goToSleep\n");
                goToSleep();
                break;
            case 5:
                printKernelMsg("mountUmountSdcard1Below6_0.mount\n");
                mountUmountSdcard1Below6_0(true);
                break;
            case 6:
                printKernelMsg("mountUmountSdcard1Below6_0.unmount\n");
                mountUmountSdcard1Below6_0(false);
                break;

            case 7:
                if (intent.hasExtra("paraString"))
                {
                    mparaString = intent.getExtras().getString("paraString");
                    printKernelMsg("start app :" + startAPP(mparaString));
                }
                else
                {
                    printKernelMsg("start app fail");
                }
                break;
            case 8:
                printKernelMsg("grantWhiteListPermissions:");
                grantWhiteListPermissions();
                break;
            case 9:
                if (intent.hasExtra("paraString"))
                {
                    mparaString = intent.getExtras().getString("paraString");
                    printKernelMsg("grantPackagePermissions:" + mparaString);
                    grantPackagePermissions("com.baidu.BaiduMap",	PermissionInfo.PROTECTION_DANGEROUS);
                }
                else
                {
                    printKernelMsg("grantPackagePermissions: fail");
                }
                break;
            case 10:
                printKernelMsg("reboot");
                reboot();
                break;
            case 11:
                playMusic("/flysystem/lib/out/welcome.mp3");
                break;
            default:
                printKernelMsg("unkown:" + action + "\n");
                break;
            }
        }
    };

    public String startAPP(String appPackageName)
    {
        try
        {
            Intent intent = mContext.getPackageManager()
                            .getLaunchIntentForPackage(appPackageName);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            mContext.startActivity(intent);
            return "success:" + appPackageName;
        }
        catch (Exception e)
        {
            return "error:" + e.getMessage();
        }
    }

    @Override
    public void onDestroy()
    {
        // TODO Auto-generated method stub
        super.onDestroy();
        printKernelMsg("onDestroy");
    }

    @Override
    public IBinder onBind(Intent intent)
    {
        // TODO Auto-generated method stub
        return null;
    }

    public void printKernelMsg(String string)
    {
        FileWrite("/dev/lidbg_msg", false, false, "LidbgCommenLogic: " + string
                  + (string.endsWith("\n") ? "" : "\n"));
    }

    public boolean FileWrite(String file_path, boolean creatit, boolean append,
                             String write_str)
    {
        // TODO Auto-generated method stub
        if (file_path == null | write_str == null)
        {
            return false;
        }
        File mFile = new File(file_path);
        if (!mFile.exists())
        {
            if (creatit)
            {
                try
                {
                    mFile.createNewFile();
                }
                catch (IOException e)
                {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            else
            {
                return false;
            }
        }
        try
        {
            FileOutputStream fout = new FileOutputStream(
                mFile.getAbsolutePath(), append);
            byte[] bytes = write_str.getBytes();
            fout.write(bytes);
            fout.close();
        }
        catch (IOException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return true;
    }
public void FileDelete(String fileName)
{
	// TODO Auto-generated method stub
	File mFile = new File(fileName);
	if (mFile.isFile())
	{
		mFile.delete();
		return;
	}
	if (mFile.isDirectory())
	{
		File[] childFile = mFile.listFiles();
		if (childFile != null && childFile.length >= 0)
		{
			for (File f : childFile)
			{
				if (f.isDirectory())
				{
					FileDelete(f.getAbsolutePath());
				} else
				{
					f.delete();
				}

			}
		}
	}
	mFile.delete();
}
    public boolean isFileExist(String file)
    {
        File kmsgfiFile = new File(file);
        return kmsgfiFile.exists();
    }

    private boolean grantWhiteListPermissions()
    {
        // TODO Auto-generated method stub
        if (mWhitePermissionList != null)
        {
            printKernelMsg("grantWhiteListPermissions");
            FileWrite("/dev/lidbg_misc0", false, false, "flyaudio:echo LidbgCommenLogic.grant_start > /dev/lidbg_msg");
            for (int i = 0; i < mWhitePermissionList.length; i++)
            {
                grantPackagePermissions(mWhitePermissionList[i], PermissionInfo.PROTECTION_DANGEROUS);
            }
            FileWrite("/dev/lidbg_misc0", false, false, "flyaudio:echo LidbgCommenLogic.grant_stop > /dev/lidbg_msg");
            FileWrite("/dev/lidbg_misc0", false, false, "flyaudio:setprop persist.lidbg.grant.done 1");
        }
        return false;
    }

    private boolean grantPackagePermissions(String pkg, int protectionDangerous)
    {
        // TODO Auto-generated method stub
        if(Build.VERSION.SDK_INT < 23)
            return false;
        String[] permission = getPackagePermissions(pkg, protectionDangerous);
        if (permission != null)
        {
            for (int i = 0; i < permission.length; i++)
            {
                String cmd = "pm grant " + pkg + " " + permission[i];
                printKernelMsg(i + "/" + permission.length + "=" + pkg + " " + permission[i]);
                FileWrite("/dev/lidbg_misc0", false, false, "flyaudio:" + cmd);
            }
            return true;
        }
        return false;
    }
    public boolean isPermissionLevel(String permission, int PermissionLevel)
    {
        try
        {
            PermissionInfo info = mContext.getPackageManager()
                                  .getPermissionInfo(permission, 0);
            return info.protectionLevel == PermissionLevel;
        }
        catch (NameNotFoundException nnfe)
        {
            // printKernelMsg("No such permission: " + permission + "/"+ nnfe.getMessage());
        }
        return false;
    }

    public String[] getPackagePermissions(String name, int PermissionLevel)
    {
        // TODO Auto-generated method stub
        PackageManager pm = mContext.getPackageManager();
        PackageInfo installedPackages;
        try
        {
            installedPackages = pm.getPackageInfo(name,
                                                  PackageManager.GET_PERMISSIONS);
            String[] requestedPermissions = installedPackages.requestedPermissions;
            if (requestedPermissions != null)
            {
                if (PermissionLevel == -1)
                {
                    return requestedPermissions;
                }
                ArrayList<String> NewStrings = new ArrayList<String>();
                for (String requestedPermission : requestedPermissions)
                {
                    if (isPermissionLevel(requestedPermission, PermissionLevel) || isInList(mDangerlPermission, requestedPermission))
                    {
                        NewStrings.add(requestedPermission);
                    }
                }
                String[] resultStrings = new String[NewStrings.size()];
                for (int i = 0; i < NewStrings.size(); i++)
                {
                    resultStrings[i] = NewStrings.get(i);
                }
                if (NewStrings.size() > 0)
                {
                    return resultStrings;
                }
                else
                {
                    return null;
                }
            }
        }
        catch (Exception e)
        {
            // TODO Auto-generated catch block
            printKernelMsg("getPackagePermissions-error:" + e.getMessage());
        }
        return null;
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

    public void playMusic(String fileName)
    {
        MediaPlayer mediaplayer = new MediaPlayer();
        printKernelMsg("fuplay: " + fileName);
        try
        {
            mediaplayer.reset();
            mediaplayer.setDataSource(fileName);
            mediaplayer.setLooping(false);
            mediaplayer.prepare();
            mediaplayer.start();
        }
        catch (Exception e)
        {
            printKernelMsg("fuplay: " + e.getMessage());
        }
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
        }
        catch (FileNotFoundException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        catch (IOException e)
        {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        return res;
    }
    public String printStringArray(String[] items, String head)
    {
        String logString = "";
        for (int i = 0; i < items.length; i++)
        {
            logString += i + head + items[i] + "\n";
            printKernelMsg(logString);
        }
        return logString;
    }
    private boolean isInList( String[] mList, String packageName)
    {
        if ((mList != null))
        {
            for (String processName : mList)
            {
                if (processName != null && processName.equals(packageName))
                {
                    return true;
                }
            }
        }
        return false;
    }
}

