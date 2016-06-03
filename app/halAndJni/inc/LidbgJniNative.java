package com.android.mypftf99.app4haljni;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import android.content.Context;

public class LidbgJniNative
{

	public interface LidbgJniNativeClient
	{
		public void onTestCallBack(String msg);

		public void onAbnormalEvent(int value);
	}

	public enum DebugLevel
	{
		LevelTest, LevelDriversAbonomal
	};

	private static int sInitialized = 0;
	private Context gcontext;
	private LidbgJniNativeClient mclient = null;

	public native long nativeInit(int id);

	public native int nativeDestroy(long dev);

	public native int CameraSetPath(int id, String path);

	public native int CameraStartRecord(int id);

	public native int CameraStopRecord(int id);

	public native int setDebugLevel(int level);

	private void hal2jni2appCallBack(String msg)
	{
		if (mclient != null)
		{
			mclient.onTestCallBack(msg);
		}
	}

	private void driverAbnormalEvent(int value)
	{
		if (mclient != null)
		{
			mclient.onAbnormalEvent(value);
		}
	}

	// ////////////////////////////////////////////
	static
	{
		sInitialized = 0;
		try
		{
			System.load("/flysystem/lib/out/liblidbg_jni.so");
			sInitialized++;
		} catch (UnsatisfiedLinkError e)
		{
			printKernelMsg("fail loadLibrary:" + e.getMessage());
		}
	}

	public LidbgJniNative(Context context, LidbgJniNativeClient client)
	{
		// TODO Auto-generated constructor stub
		gcontext = context;
		mclient = client;
	}

	protected void finalize() throws Throwable
	{
		printKernelMsg("LidbgJniNative.finalize");
	}

	public static void printKernelMsg(String string)
	{
		// TODO Auto-generated method stub
		FileWrite("/dev/lidbg_msg", false, false, "fsclass:" + sInitialized
				+ "/" + string + "\n");
	}

	private static boolean FileWrite(String file_path, boolean creatit,
			boolean append, String write_str)
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
				} catch (IOException e)
				{
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			} else
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
		} catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return true;
	}
}
