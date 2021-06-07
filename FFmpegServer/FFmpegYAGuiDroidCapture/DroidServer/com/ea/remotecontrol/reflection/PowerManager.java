package com.ea.remotecontrol.reflection;

import android.os.IInterface;
import java.lang.reflect.Method;
import android.os.Build;
import android.view.KeyEvent;
import com.ea.remotecontrol.reflection.ServiceManager;
import com.ea.remotecontrol.reflection.InputManager;
import com.ea.remotecontrol.common.LogD;

public class PowerManager
{
	private final IInterface 	m_PowerServiceInterface;
	private final Method 		m_IsScreenOnMethod;
	
	public PowerManager(IInterface powerServiceInterface)
	{
		this.m_PowerServiceInterface = powerServiceInterface;
		
		try 
		{
			String sMethodName = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT_WATCH ? "isInteractive" : "isScreenOn";
            m_IsScreenOnMethod = powerServiceInterface.getClass().getMethod(sMethodName);
        } 
		catch (NoSuchMethodException e) 
		{
			LogD.LogConsole("Exception in PowerManager::PowerManager()");
			
            e.printStackTrace();
            throw new AssertionError(e);
        }
	}
	
	public boolean isScreenOn() 
	{
        try 
		{
            return (Boolean) m_IsScreenOnMethod.invoke(m_PowerServiceInterface);
        } 
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in PowerManager::isScreenOn()");
			
            e.printStackTrace();
            throw new AssertionError(e);
        }
    }
	
	public void turnScreenOn(ServiceManager serviceManager) 
	{
        LogD.LogConsole("enter screen KEYCODE_POWER");
		
		InputManager inputManager = serviceManager.getInputManager();
		
		boolean result = inputManager.injectKeyInput(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_POWER, 0);
		
        //KeyEvent[] keyEvents = EventFactory.clickEvent(KeyEvent.KEYCODE_POWER);
        //boolean result = true;
        //for (KeyEvent keyEvent : keyEvents) 
		//{
        //    result = result & serviceManager.getInputManager().injectInputEvent(keyEvent, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
        //}
		
        LogD.LogConsole("key KEYCODE_POWER result = " + result);
    }
}