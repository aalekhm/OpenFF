package com.ea.remotecontrol.reflection;

import android.os.IInterface;
import android.view.IRotationWatcher;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import com.ea.remotecontrol.common.LogD;

public class WindowManager
{
	private final IInterface 	m_WindowServiceInterface;

    private Method              m_GetRotationMethod;
    private Method              m_FreezeRotationMethod;
    private Method              m_IsRotationFrozenMethod;
    private Method              m_ThawRotationMethod;

	public WindowManager(IInterface windowServiceInterface)
	{
		this.m_WindowServiceInterface = windowServiceInterface;
	}
	
    private Method getRotationMethod() throws NoSuchMethodException
    {
        if(m_GetRotationMethod == null)
        {
            Class<?> cls = m_WindowServiceInterface.getClass();
            try
            {
                // method changed since this commit:
                // https://android.googlesource.com/platform/frameworks/base/+/8ee7285128c3843401d4c4d0412cd66e86ba49e3%5E%21/#F2
                m_GetRotationMethod = cls.getMethod("getDefaultDisplayRotation");
            }
            catch(NoSuchMethodException nsme)
            {
                // old version
                m_GetRotationMethod = cls.getMethod("getRotation");
            }
        }

        return m_GetRotationMethod;
    }

    private Method getFreezeRotationMethod() throws NoSuchMethodException
    {
        if(m_FreezeRotationMethod == null)
        {
            Class<?> cls = m_WindowServiceInterface.getClass();
            m_FreezeRotationMethod = cls.getMethod("freezeRotation", int.class);
        }

        return m_FreezeRotationMethod;
    }

    private Method getIsRotationFrozenMethod() throws NoSuchMethodException
    {
        if(m_IsRotationFrozenMethod == null)
        {
            Class<?> cls = m_WindowServiceInterface.getClass();
            m_IsRotationFrozenMethod = cls.getMethod("isRotationFrozen");
        }

        return m_IsRotationFrozenMethod;
    }

    private Method getThawRotationMethod() throws NoSuchMethodException
    {
        if(m_ThawRotationMethod == null)
        {
            Class<?> cls = m_WindowServiceInterface.getClass();
            m_ThawRotationMethod = cls.getMethod("thawRotation");
        }

        return m_ThawRotationMethod;
    }

    public void registerRotationWatcher(IRotationWatcher rotationWatcher, int displayId) 
    {
        try 
        {
            Class<?> cls = m_WindowServiceInterface.getClass();
            try 
            {
                // display parameter added since this commit:
                // https://android.googlesource.com/platform/frameworks/base/+/35fa3c26adcb5f6577849fd0df5228b1f67cf2c6%5E%21/#F1
                cls.getMethod("watchRotation", IRotationWatcher.class, int.class).invoke(m_WindowServiceInterface, rotationWatcher, displayId);
            }
            catch (NoSuchMethodException e) 
            {
                // old version
                cls.getMethod("watchRotation", IRotationWatcher.class).invoke(m_WindowServiceInterface, rotationWatcher);
            }
        }
        catch (Exception e)
        {
            throw new AssertionError(e);
        }
    }

    public int getRotation()
    {
        try
        {
            Method rotationMethod = getRotationMethod();
            return (int)rotationMethod.invoke(m_WindowServiceInterface);
        }
        catch(InvocationTargetException | IllegalAccessException | NoSuchMethodException e) 
        {
            LogD.LogConsole("Could not invoke method WindowManager::getRotation "+e);
            return 0;
        }
    }

    public void freezeRotation(int iRotation)
    {
        try
        {
            Method freezeMethod = getFreezeRotationMethod();
            freezeMethod.invoke(m_WindowServiceInterface, iRotation);
        }
        catch(InvocationTargetException | IllegalAccessException | NoSuchMethodException e)
        {
            LogD.LogConsole("Could not invoke method WindowManager::freezeRotation "+e);
        }
    }

    public boolean isRotationFrozen()
    {
        try
        {
            Method isRotationFrozenMethod = getIsRotationFrozenMethod();
            return (boolean)isRotationFrozenMethod.invoke(m_WindowServiceInterface);
        }
        catch(InvocationTargetException | IllegalAccessException | NoSuchMethodException e)
        {
            LogD.LogConsole("Could not invoke method WindowManager::isRotationFrozen "+e);
            return false;
        }
    }

    public void thawRotation()
    {
        try
        {
            Method thawRotationMethod = getThawRotationMethod();
            thawRotationMethod.invoke(m_WindowServiceInterface);
        }
        catch(InvocationTargetException | IllegalAccessException | NoSuchMethodException e)
        {
            LogD.LogConsole("Could not invoke method WindowManager::thawRotation "+e);
        }
    }
}