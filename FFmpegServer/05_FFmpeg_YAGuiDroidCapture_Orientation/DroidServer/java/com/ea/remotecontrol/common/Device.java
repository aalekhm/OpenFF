package com.ea.remotecontrol.common;

import android.os.Build;
import android.view.IRotationWatcher;

import com.ea.remotecontrol.reflection.ServiceManager;
import com.ea.remotecontrol.reflection.DisplayManager;
import com.ea.remotecontrol.reflection.PowerManager;
import com.ea.remotecontrol.reflection.WindowManager;

import com.ea.remotecontrol.common.DisplayInfo;
import com.ea.remotecontrol.common.Size;
import com.ea.remotecontrol.common.LogD;

public final class Device
{
	public interface RotationListener
	{
		void onRotationChanged(int iRotation);
	}

	private RotationListener			m_RotationListener; 
	public static final ServiceManager	SERVICE_MANAGER = new ServiceManager();

	DisplayInfo							m_pDisplayInfo;
	DisplayManager						m_pDisplayManager;
	PowerManager						m_pPowerManager;
	WindowManager						m_pWindowManager;

	private int 						m_iWidth;
	private int 						m_iHeight;
	private int 						m_iRotation;
	private String						m_sModel;

	public Device() 
	{
		int iDisplayId = 0;

	    m_pDisplayManager	= SERVICE_MANAGER.getDisplayManager();
		m_pPowerManager		= SERVICE_MANAGER.getPowerManager();
		m_pWindowManager	= SERVICE_MANAGER.getWindowManager();
	    m_pDisplayInfo		= m_pDisplayManager.getDisplayInfo(iDisplayId);
		
	    Size sSize = m_pDisplayInfo.getSize();
		m_iWidth = sSize.getWidth();
		m_iHeight = sSize.getHeight();
		m_iRotation = m_pWindowManager.getRotation();

		m_sModel = Build.MODEL;

		if(!m_pPowerManager.isScreenOn())
		{
			m_pPowerManager.turnScreenOn(SERVICE_MANAGER);
		}

        m_pWindowManager.registerRotationWatcher(new IRotationWatcher.Stub() 
		{
            @Override
            public void onRotationChanged(int iRotation)
			{
                synchronized (Device.this) 
				{
					LogD.LogConsole("onRotationChanged:: iRotation = "+iRotation);

					flip(iRotation);

                    // notify
                    if (m_RotationListener != null) 
					{
                        m_RotationListener.onRotationChanged(m_iRotation);
                    }
                }
            }
        }, iDisplayId);
	}

    public synchronized void setRotationListener(RotationListener pRotationListener) 
	{
        m_RotationListener = pRotationListener;
    }

	public String getModelName()
	{
		return m_sModel;
	}

	public int getWidth()
	{
		return m_iWidth;
	}

	public int getHeight()
	{
		return m_iHeight;
	}

	public void flip(int iRotation)
	{
		boolean bOrientationChanged = ((m_iRotation + iRotation) % 2 != 0);
		if(bOrientationChanged)
		{
			int iTempW = m_iWidth;
			m_iWidth = m_iHeight;
			m_iHeight = iTempW;
		}

		m_iRotation = iRotation;
	}
}