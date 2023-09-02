package com.ea.remotecontrol.common;

import android.util.Log;

public class LogD
{
	public static final String LOG_TAG = "[Java] AndroidRemoteControl";
	
	public static void LogConsole(String sString)
	{
		System.out.println(LOG_TAG+": "+sString);
	}

	public static void LogConsole(String sTag, String sString)
	{
		System.out.println(sTag+": "+sString);
	}

	public static void LogCat(String sString)
	{
		android.util.Log.d(LOG_TAG, sString);
	}
}
