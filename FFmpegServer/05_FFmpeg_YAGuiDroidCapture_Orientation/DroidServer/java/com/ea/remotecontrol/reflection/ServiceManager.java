package com.ea.remotecontrol.reflection;

import android.content.Context;
import android.os.IBinder;
import android.os.IInterface;

import java.lang.reflect.Method;

public final class ServiceManager
{
	private final Method getServiceMethod;
	
	private DisplayManager 	m_DisplayManager;
	private PowerManager 	m_PowerManager;
	private InputManager 	m_InputManager;
	private WindowManager 	m_WindowManager;
	
	public ServiceManager()
	{
		try
		{
			getServiceMethod = Class.forName("android.os.ServiceManager").getDeclaredMethod("getService", String.class);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			throw new AssertionError(e);
		}
	}
	
	private IInterface getService(String sService, String sType)
	{
		try
		{
			IBinder binder = (IBinder)getServiceMethod.invoke(null, sService);
			Method asInterface = Class.forName(sType + "$Stub").getMethod("asInterface", IBinder.class);
			return (IInterface) asInterface.invoke(null, binder);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			throw new AssertionError(e);
		}
	}
	
	public DisplayManager getDisplayManager()
	{
		if(m_DisplayManager == null)
		{
			IInterface displayServiceInterface = getService(Context.DISPLAY_SERVICE, "android.hardware.display.IDisplayManager");
			m_DisplayManager = new DisplayManager(displayServiceInterface);
		}
		
		return m_DisplayManager;
	}

	public PowerManager getPowerManager()
	{
		if(m_PowerManager == null)
		{
			IInterface powerServiceInterface = getService(Context.POWER_SERVICE, "android.os.IPowerManager");
			m_PowerManager = new PowerManager(powerServiceInterface);
		}
		
		return m_PowerManager;
	}

	public InputManager getInputManager()
	{
		if(m_InputManager == null)
		{
			IInterface inputServiceInterface = getService(Context.INPUT_SERVICE, "android.hardware.input.IInputManager");
			m_InputManager = new InputManager(inputServiceInterface);
		}
		
		return m_InputManager;
	}

	public WindowManager getWindowManager()
	{
		if(m_WindowManager == null)
		{
			IInterface windowServiceInterface = getService(Context.WINDOW_SERVICE, "android.view.IWindowManager");
			m_WindowManager = new WindowManager(windowServiceInterface);
		}
		
		return m_WindowManager;
	}
}