package com.ea.remotecontrol.reflection;

import android.os.IBinder;
import android.view.Surface;
import android.graphics.Rect;
import com.ea.remotecontrol.common.LogD;

public class SurfaceManager
{
	private static final Class<?> CLASS;
	
	static 
	{
		try 
		{
			CLASS = Class.forName("android.view.SurfaceControl");
		} 
		catch (ClassNotFoundException e) 
		{
			throw new AssertionError(e);
		}
	}

	private SurfaceManager()
	{
	}
	
	public static IBinder createDisplay(String name, boolean secure) 
	{
		try 
		{
            return (IBinder) CLASS.getMethod("createDisplay", String.class, boolean.class).invoke(null, name, secure);
        } 
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in SurfaceManager::createDisplay()");
			e.printStackTrace();
			
            throw new AssertionError(e);
        }
    }
	
	public static void openTransaction()
	{
        try 
		{
            CLASS.getMethod("openTransaction").invoke(null);
        } 
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in SurfaceManager::openTransaction()");
			e.printStackTrace();

            throw new AssertionError(e);
        }
    }

	public static void setDisplaySurface(IBinder displayToken, Surface surface) 
	{
		try 
		{
			CLASS.getMethod("setDisplaySurface", IBinder.class, Surface.class).invoke(null, displayToken, surface);
		} 
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in SurfaceManager::setDisplaySurface()");
			e.printStackTrace();

			throw new AssertionError(e);
		}
	}

	public static void setDisplayProjection(IBinder displayToken, int orientation, Rect layerStackRect, Rect displayRect) 
	{
		try 
		{
			CLASS.getMethod("setDisplayProjection", IBinder.class, int.class, Rect.class, Rect.class)
					.invoke(null, displayToken, orientation, layerStackRect, displayRect);
		} 
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in SurfaceManager::setDisplayProjection()");
			e.printStackTrace();

			throw new AssertionError(e);
		}
	}

	public static void setDisplayLayerStack(IBinder displayToken, int layerStack) 
	{
		try 
		{
			CLASS.getMethod("setDisplayLayerStack", IBinder.class, int.class).invoke(null, displayToken, layerStack);
		} 
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in SurfaceManager::setDisplayLayerStack()");
			e.printStackTrace();

			throw new AssertionError(e);
		}
	}
	
	public static void closeTransaction() 
	{
        try 
		{
            CLASS.getMethod("closeTransaction").invoke(null);
        } 
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in SurfaceManager::closeTransaction()");
			e.printStackTrace();

            throw new AssertionError(e);
        }
    }

	public static void destroyDisplay(IBinder displayToken) 
	{
		try 
		{
			CLASS.getMethod("destroyDisplay", IBinder.class).invoke(null, displayToken);
		} 
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in SurfaceManager::destroyDisplay()");
			e.printStackTrace();

			throw new AssertionError(e);
		}
	}
}