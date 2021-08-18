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
	
	public DisplayInfo getDisplayInfo(int iDisplayId)
	{
		try
		{
			Object displayInfo = m_DisplayServiceInterface.getClass().getMethod("getDisplayInfo", int.class).invoke(m_DisplayServiceInterface, iDisplayId);
			
			// Class is a parameterizable class, hence you can use the syntax Class<T> where T is a type. By writing Class<?>, 
			// you're declaring a Class object which can be of any type (? is a wildcard). 
			// The Class type is a type that contains meta-information about a class.
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