package com.ea.remotecontrol.reflection;

import android.os.IInterface;
import com.ea.remotecontrol.common.DisplayInfo;
import com.ea.remotecontrol.common.Size;

public class DisplayManager
{
	private final IInterface m_DisplayServiceInterface;
	
	public DisplayManager(IInterface displayServiceInterface)
	{
		this.m_DisplayServiceInterface = displayServiceInterface;
	}
	
	public DisplayInfo getDisplayInfo()
	{
		try
		{
			Object displayInfo = m_DisplayServiceInterface.getClass().getMethod("getDisplayInfo", int.class).invoke(m_DisplayServiceInterface, 0);
			
			Class<?> clss = displayInfo.getClass();
			
			// Get the actual width, height & orientation of the device.
			int iWidth = clss.getDeclaredField("logicalWidth").getInt(displayInfo);
			int iHeight = clss.getDeclaredField("logicalHeight").getInt(displayInfo);
			int iRotation = clss.getDeclaredField("rotation").getInt(displayInfo);
			
			return new DisplayInfo(new Size(iWidth, iHeight), iRotation);
		}
		catch(Exception e)
		{
			e.printStackTrace();
            throw new AssertionError(e);
		}
	}
}