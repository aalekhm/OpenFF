package com.ea.remotecontrol.reflection;

import android.os.IInterface;
import android.os.SystemClock;
import android.view.InputEvent;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.KeyCharacterMap;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import com.ea.remotecontrol.common.LogD;

public class InputManager
{
	private final IInterface 	m_InputServiceInterface;
	private final Method 		m_InjectInputEventMethod;

	public static final int INJECT_INPUT_EVENT_MODE_ASYNC = 0;
    public static final int INJECT_INPUT_EVENT_MODE_WAIT_FOR_RESULT = 1;
    public static final int INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH = 2;

	private final int ACTION_MOUSE_UP	= 0;
	private final int ACTION_MOUSE_DOWN	= 1;
	private final int ACTION_MOUSE_MOVE	= 2;
	
	private final int ACTION_KEY_UP		= 0;
	private final int ACTION_KEY_DOWN	= 1;

	public InputManager(IInterface inputServiceInterface)
	{
		this.m_InputServiceInterface = inputServiceInterface;
		
		try
		{
			m_InjectInputEventMethod = inputServiceInterface.getClass().getMethod("injectInputEvent", InputEvent.class, int.class);
		}
		catch(Exception e)
		{
			LogD.LogConsole("Exception in InputManager::InputManager()");
			
			e.printStackTrace();
			throw new AssertionError(e);
		}
	}
	
	public boolean injectInputEvent(InputEvent inputEvent, int iMode)
	{
		try
		{
			return (Boolean)m_InjectInputEventMethod.invoke(m_InputServiceInterface, inputEvent, iMode);
		}
		catch(InvocationTargetException | IllegalAccessException e)
		{
			LogD.LogConsole("Exception in InputManager::injectInputEvent()");
			
			e.printStackTrace();
			throw new AssertionError(e);
		}
	}
	
	public boolean injectMouseInput(int iAction, int iKey, int iPosX, int iPosY)
	{
		//LogD.LogConsole("MouseInputs:: iAction = "+iAction+", iKey = "+iKey+", iPosX = "+iPosX+", iPosY = "+iPosY);
		
		int iEvent = -1;
		switch(iAction)
		{
			case ACTION_MOUSE_UP:
			{
				iEvent = MotionEvent.ACTION_UP;
			}
			break;
			case ACTION_MOUSE_DOWN:
			{
				iEvent = MotionEvent.ACTION_DOWN;
			}
			break;
			case ACTION_MOUSE_MOVE:
			{
				iEvent = MotionEvent.ACTION_MOVE;
			}
			break;
		}
		
		long lCurrentTime = SystemClock.uptimeMillis();
		
		MotionEvent motionEvent = MotionEvent.obtain(lCurrentTime, lCurrentTime, iEvent, iPosX, iPosY, 0);
		motionEvent.setSource(InputDevice.SOURCE_TOUCHSCREEN);
		
		return injectInputEvent(motionEvent, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
	}
	
	public boolean injectKeyInput(int iAction, int iKeyCode, int iMetaModifier)
	{
		//LogD.LogConsole("KeyboardInputs:: iAction = "+iAction+", iAsciiKeyCode ====== "+(char)iAsciiKeyCode);
		//LogD.LogConsole("\tiMetaModifier = "+iMetaModifier);

		int iEvent = -1;
		switch(iAction)
		{
			case ACTION_KEY_UP:
			{
				iEvent = KeyEvent.ACTION_UP;
			}
			break;
			case ACTION_KEY_DOWN:
			{
				iEvent = KeyEvent.ACTION_DOWN;
			}
			break;
		}

		long lCurrentTime = SystemClock.uptimeMillis();
		
		KeyEvent keyEvent = new KeyEvent(	lCurrentTime, lCurrentTime, 
											iAction, 
											iKeyCode,
											0,//iRepeat, 
											iMetaModifier,
											KeyCharacterMap.VIRTUAL_KEYBOARD, 
											0, 0, 
											InputDevice.SOURCE_KEYBOARD);

		return injectInputEvent(keyEvent, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
    }
}