package com.ea.remotecontrol.common;

public class Size
{
	public final int m_iWidth;
	public final int m_iHeight;
	
	public Size(int iWidth, int iHeight)
	{
		this.m_iWidth = iWidth;
		this.m_iHeight = iHeight;
	}
	
	public int getWidth()
	{
		return m_iWidth;
	}

	public int getHeight()
	{
		return m_iHeight;
	}
	
	public String toString()
	{
		return 	"Size(" + 
				"width = " + m_iWidth +
				", " +
				"height = " + m_iHeight +
				")";
				
	}
}