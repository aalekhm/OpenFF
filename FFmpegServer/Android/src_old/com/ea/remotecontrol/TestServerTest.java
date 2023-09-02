package com.ea.remotecontrol;

import java.net.Socket;
import java.net.InetSocketAddress;
import java.io.OutputStream;
import java.io.IOException;

public class TestServerTest
{
	private static void socket() 
	{
		//Open socket client
		Socket client = new Socket();
		
		//Use adb to forward port to 8888
		try 
		{
			//blocking
			client.connect(new InetSocketAddress("127.0.0.1", 8888));
			System.out.println("Connected successfully!!");
			
			OutputStream outputStream = client.getOutputStream();
			String sExit = "Exit";
			String sString = "Hello World";
			while(true)
			{
				outputStream.write(sString.length());
				outputStream.write("Hello World".getBytes());
				outputStream.flush();
				
				try
				{
					Thread.sleep(1000);
				}
				catch(Exception e)
				{
				}
				
				if(sExit == "E")
					break;
			}
			
			outputStream.close();
		} 
		catch (IOException e) 
		{
			e.printStackTrace();
		}
	}
	
	public static void main(String[] args)
	{
		System.out.println("'TestServerTest' connecting to 'TestServer'...");
		TestServerTest.socket();
	}
}