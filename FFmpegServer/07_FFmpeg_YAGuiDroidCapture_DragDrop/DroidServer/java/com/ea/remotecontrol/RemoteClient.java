package com.ea.remotecontrol;

import android.os.SystemClock;
import java.io.IOException;
import java.nio.ByteBuffer;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.ByteOrder;
import java.net.Socket;
import java.net.InetSocketAddress;

import com.ea.remotecontrol.common.Device;
import com.ea.remotecontrol.common.ScreenEncoder;
import com.ea.remotecontrol.common.IO;
import com.ea.remotecontrol.common.LogD;
	
public class RemoteClient
{
	private static final String 		SOCKET_NAME = "AndroidRemoteControl";
		
	private static int 					m_iWidth;
	private static int 					m_iHeight;
	
	private static Socket 				m_LocalVideoSocket = null;
	private static LocalSocket 			m_LocalControlSocket = null;
	
	private static LocalServerSocket 	m_LocalServerSocket = null;
	private static LocalSocket 			m_Client = null;
	private static InputStream 			m_ServerInputStream = null;
	private static OutputStream 		m_ServerOutputStream = null;

	private static Device				m_pDevice;
	
	public static boolean startServer(String sSocketName)
	{
		try 
		{
			LogD.LogConsole("\'"+SOCKET_NAME+"\' listening for connection on the device...");
			m_LocalServerSocket = new LocalServerSocket(sSocketName);
			
			m_Client = m_LocalServerSocket.accept();
			LogD.LogConsole("\'"+SOCKET_NAME+"\' connected to Client");
			
			m_ServerInputStream = m_Client.getInputStream();
			m_ServerOutputStream = m_Client.getOutputStream();
			
			return true;
		}
		catch (Exception e) 
		{
			LogD.LogConsole("Exception in RemoteServer::startServer() = "+e.toString());
			
			e.printStackTrace();
		}
		
		return false;
	}

	public static boolean connectClient(String sSocketName, String sIPAddress, int iPort)
	{
		m_LocalVideoSocket = new Socket();
		
		try 
		{
			LogD.LogConsole("Connecting to Video Socket Server!");
			m_LocalVideoSocket.connect(new InetSocketAddress(sIPAddress, iPort));
			LogD.LogConsole("Connected to Video Socket Server!");

			m_ServerInputStream = m_LocalVideoSocket.getInputStream();
			m_ServerOutputStream = m_LocalVideoSocket.getOutputStream();
		}
		catch(IOException e)
		{
			LogD.LogConsole("Exception in RemoteClient::connectClient() = "+e.toString());
			return false;
		}
	
		return true;
	}

    public static void main(String... args) 
	{
		LogD.LogConsole("Android RemoteClient! "+args.length);
		
		/*
		LogD.LogConsole("Getting SmsManager!");
		try
		{
			//android.telephony.SmsManager sm = android.telephony.SmsManager.getDefault();
			//LogD.LogConsole("Got SmsManager! "+sm);
			//if(sm != null)
			//{
			//	LogD.LogConsole("Got SmsManager!");
			//	sm.sendTextMessage(	"+917780562154", null,
			//						"Hello World!",
			//						null, null);
			//	LogD.LogConsole("Sms Sent!");
			//}
				
			android.telephony.TelephonyManager  tm =(android.telephony.TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);  
         
			//Calling the methods of TelephonyManager the returns the information  
			String IMEINumber=tm.getDeviceId();  
			String subscriberID=tm.getDeviceId();  
			String SIMSerialNumber=tm.getSimSerialNumber();  
			String networkCountryISO=tm.getNetworkCountryIso();  
			String SIMCountryISO=tm.getSimCountryIso();  
			String softwareVersion=tm.getDeviceSoftwareVersion();  
			String voiceMailNumber=tm.getVoiceMailNumber();  

			LogD.LogConsole("IMEINumber = "+IMEINumber);

		}
		catch(Exception e)
		{
			LogD.LogConsole("Exception while sending sms "+e);
		}
		*/

		String sIPAddress = args[0];
		int iPort = Integer.parseInt(args[1]);

		LogD.LogConsole("Android RemoteClient! "+sIPAddress+" : "+iPort);

		if(connectClient(SOCKET_NAME, sIPAddress, iPort))
		{
			m_pDevice = new Device();
			ScreenEncoder screenEncoder = new ScreenEncoder();
			
			try 
			{
				//screenEncoder.initialize(SOCKET_NAME, iWidth, iHeight, 0, 0);
				//screenEncoder.start(m_pDevice.SERVICE_MANAGER, m_ServerInputStream, m_ServerOutputStream);

				screenEncoder.streamScreen(SOCKET_NAME, m_pDevice.SERVICE_MANAGER, m_ServerInputStream, m_ServerOutputStream, m_pDevice, 0, 0);
			}
			catch(Exception e)
			{
				LogD.LogConsole("Exception in RemoteClient::main() = "+e.toString());
			}
		}
		
		LogD.LogConsole("Good Bye!");
    }	
}