package com.android.mypftf99.app4haljni;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import android.content.Context;

public class LidbgIICNative
{

	public interface LidbgIICNativeClient
	{
		public void onAbnormalEvent(int value);
	}

	private static int sInitialized = 0;
	private Context gcontext;
	private LidbgIICNativeClient mclient = null;

	public boolean IICSendDate(String path, int slaveAddr, int[] bufArr, int len)
	{
		int[] ret =
		{ -1, -1 };
		ret[0] = IICOpen(path);
		if (ret[0] > 0)
		{
			ret[1] = IICWrite(ret[0], slaveAddr, bufArr, len);
			IICClose(ret[0]);
		}
		return (ret[0] > 0 || ret[1] > 0);
	}

	public boolean IICReadDate(String path, int slaveAddr, int reg,
			int[] bufArr, int len)
	{
		int[] ret =
		{ -1, -1 };
		ret[0] = IICOpen(path);
		if (ret[0] > 0)
		{
			bufArr[0] = reg;
			ret[1] = IICRead(ret[0], slaveAddr, bufArr, len);
			IICClose(ret[0]);
		}
		return (ret[0] > 0 || ret[1] > 0);
	}

	public native int IICOpen(String path);

	private native int IICRead(int fd, int slaveAddr, int[] bufArr, int len);

	private native int IICWrite(int fd, int slaveAddr, int[] bufArr, int len);

	public native int IICClose(int fd);

	// ////////////////////////////////////////////
	static
	{
		try
		{
			System.load("/flysystem/lib/out/liblidbg_jni.so");
			sInitialized++;
		} catch (UnsatisfiedLinkError e)
		{
			printKernelMsg("fail loadLibrary:" + e.getMessage());
		}
	}

	public LidbgIICNative(Context context, LidbgIICNativeClient client)
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
		FileWrite("/dev/lidbg_msg", false, false, "LidbgIICNative:"
				+ sInitialized + "/" + string + "\n");
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
