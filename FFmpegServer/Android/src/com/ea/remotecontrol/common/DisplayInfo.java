package com.ea.remotecontrol.common;

public class DisplayInfo
{
	private final Size m_Size;
	private final int m_iRotation;
	
	public DisplayInfo(Size size, int iRotation)
	{
		this.m_Size = size;
		this.m_iRotation = iRotation;
	}
	
	public Size getSize()
	{
		return m_Size;
	}
	
	public int getRotation()
	{
		return m_iRotation;
	}
	
	public String toString() 
	{
        return "DisplayInfo(" +
                "size = " + m_Size +
                ", rotation = " + m_iRotation +
                ')';
    }
}