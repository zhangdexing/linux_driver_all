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

public class LidbgCommenLogicService extends Service
{

    protected static final String ACCProperties = "persist.lidbg.acc.status";
    private LidbgCommenLogicService mLidbgCommenLogicService;
    private PendingIntent peration;
    protected int loopCount = 0;
    private Context mContext;
    private IMountService mMountService;

    @Override
    public void onCreate()
    {
        super.onCreate();
        mContext = getApplication();
        printKernelMsg("onCreate");

        mLidbgCommenLogicService = this;
        IntentFilter filter = new IntentFilter();
        filter.addAction("android.intent.action.BOOT_COMPLETED");
        filter.addAction("com.fly.lidbg.LidbgCommenLogic");
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        filter.setPriority(Integer.MAX_VALUE);
        mLidbgCommenLogicService.registerReceiver(myReceiver, filter);
        StorageManager mStorageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
        mStorageManager.registerListener(mStorageEventListener);
    }
    private final StorageEventListener mStorageEventListener = new StorageEventListener()
    {
        @Override
        public void onStorageStateChanged(String path, String oldState, String newState)
        {
            printKernelMsg("onStorageStateChanged :" + path + " " + oldState + " -> " + newState + "\n");
            if(Build.VERSION.SDK_INT >= 23 && Environment.MEDIA_MOUNTED.equals(newState) && !path.contains("emulated") && !path.contains("sdcard") && !path.contains("udisk"))
            {
                FileWrite("/dev/lidbg_misc0", false, false, "flyaudio:mv /storage/udisk /storage/udisk1 ");
                printKernelMsg("ln:" + path + " " + oldState + " -> " + newState + "\n");
                FileWrite("/dev/lidbg_misc0", false, false, "flyaudio:ln -s -f " + path + " /storage/udisk &");
            }
            if (Environment.MEDIA_MOUNTED.equals(newState))
            {
            }
            else if (Environment.MEDIA_MOUNTED.equals(oldState))
            {
            }
        }
    };
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
        StorageManager mStorageManager = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
        final StorageVolume[] volumes = mStorageManager.getVolumeList();
        for (StorageVolume volume : volumes)
        {
            if ( !volume.getPath().contains("emulated") && !volume.getPath().toUpperCase().contains("SDCARD"))
            {
                //printKernelMsg(mount+"/mountUmountUdiskUp6_0.find it->" + volume.getPath()+"/getId:"+volume.getId());
                // if(mount)
                //mStorageManager.mount(volume.getId());
                // else
                // mStorageManager.unmount(volume.getId());
            }
            else
            {
                //printKernelMsg(mount+"/mountUmountUdiskUp6_0.not find->" + volume.getPath()+"/getId:"+volume.getId());
            }
        }
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
                return;
            }
            else if (intent.getAction().equals(Intent.ACTION_SCREEN_ON))
            {
                return;
            }
            else if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF))
            {
                return;
            }
            else if (intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_ATTACHED) || intent.getAction().equals(UsbManager.ACTION_USB_DEVICE_DETACHED))
            {
                UsbDevice device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                printKernelMsg("usb event:[Manufacturer:" + device.getManufacturerName() + "/name:" + device.getDeviceName()  + "]\n");
                //+ "/InterfaceClass:" + device.getInterface(0).getInterfaceClass()
                return;
            }

            if (!intent.hasExtra("action"))
            {
                printKernelMsg("return:!intent.hasExtra(\"action\")\n");
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
                  + "\n");
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
    public boolean isFileExist(String file)
    {
        File kmsgfiFile = new File(file);
        return kmsgfiFile.exists();
    }
}

