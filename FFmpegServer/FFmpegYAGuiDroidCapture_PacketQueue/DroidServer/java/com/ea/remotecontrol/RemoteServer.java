package com.ea.remotecontrol;

import android.os.SystemClock;
import java.io.IOException;
import java.nio.ByteBuffer;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Build;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.ByteOrder;

import com.ea.remotecontrol.reflection.ServiceManager;
import com.ea.remotecontrol.reflection.DisplayManager;
import com.ea.remotecontrol.reflection.PowerManager;
import com.ea.remotecontrol.common.ScreenGrabber;
import com.ea.remotecontrol.common.DisplayInfo;
import com.ea.remotecontrol.common.Size;
import com.ea.remotecontrol.common.IO;
import com.ea.remotecontrol.common.LogD;
	
public class RemoteServer
{
	private static final String 		SOCKET_NAME = "AndroidRemoteControl";
		
	private static int 					m_iWidth;
	private static int 					m_iHeight;
	
	private static LocalSocket 			m_LocalVideoSocket = null;
	private static LocalSocket 			m_LocalControlSocket = null;
	
	private static LocalServerSocket 	m_LocalServerSocket = null;
	private static LocalSocket 			m_Client = null;
	private static InputStream 			m_ServerInputStream = null;
	private static OutputStream 		m_ServerOutputStream = null;
	
	static final int 					HANDSHAKE_HEADER_SIZE = 8;
	static final int 					HANDSHAKE_DEVICENAME_SIZE = 64;
	static final int 					HANDSHAKE_DEVICEWIDTH_SIZE = 4;
	static final int 					HANDSHAKE_DEVICEHEIGHT_SIZE = 4;
	
	public static void sendHandshake(OutputStream outputStream, String sDeviceName, int iWidth, int iHeight) throws IOException
	{
		int iSize = HANDSHAKE_HEADER_SIZE + 
					HANDSHAKE_DEVICENAME_SIZE + 
					HANDSHAKE_DEVICEWIDTH_SIZE + 
					HANDSHAKE_DEVICEHEIGHT_SIZE;
					
		ByteBuffer pHandshakeBuffer = ByteBuffer.allocate(iSize);
		pHandshakeBuffer.order(ByteOrder.LITTLE_ENDIAN);
		pHandshakeBuffer.mark();
		
		pHandshakeBuffer.put((byte)'D');
		pHandshakeBuffer.put((byte)'R');
		pHandshakeBuffer.put((byte)'O');
		pHandshakeBuffer.put((byte)'I');
		pHandshakeBuffer.put((byte)'D');
		pHandshakeBuffer.position(HANDSHAKE_HEADER_SIZE);
		
		pHandshakeBuffer.put(sDeviceName.getBytes());
		pHandshakeBuffer.position(HANDSHAKE_HEADER_SIZE + HANDSHAKE_DEVICENAME_SIZE);
		
		pHandshakeBuffer.putInt(iWidth);
		pHandshakeBuffer.putInt(iHeight);
		pHandshakeBuffer.flip();
		
		//LogD.LogConsole("Handshake => size = "+iSize);
		//LogD.LogConsole("Handshake => Data => "+ new String(pHandshakeBuffer.array(), StandardCharsets.UTF_8));
		outputStream.write(pHandshakeBuffer.array());
		
		outputStream.flush();
	}

	public static boolean startServer(String sSocketName)
	{
		try 
		{
			LogD.LogConsole("\'"+SOCKET_NAME+"\'' listening for connection on the device...");
			m_LocalServerSocket = new LocalServerSocket(sSocketName);
			
			m_Client = m_LocalServerSocket.accept();
			LogD.LogConsole("\'"+SOCKET_NAME+"\'' connected to Client");
			
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

	public static boolean connectClient(String sSocketName)
	{
		m_LocalVideoSocket = new LocalSocket();
		//m_LocalControlSocket = new LocalSocket();
		
		try {
			m_LocalVideoSocket.connect(new LocalSocketAddress(sSocketName));
			LogD.LogConsole("Connected to Video Socket!");
			
			//m_LocalControlSocket.connect(new LocalSocketAddress(sSocketName));
			//LogD.LogConsole("Connected to Control Socket!");
			
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
		LogD.LogConsole("Android RemoteServer!");
		
		if(startServer(SOCKET_NAME))
		{
			ServiceManager serviceManager = new ServiceManager();
	        DisplayManager displayManager = serviceManager.getDisplayManager();
			PowerManager powerManager = serviceManager.getPowerManager();
	        DisplayInfo displayInfo = displayManager.getDisplayInfo();
			ScreenGrabber screenGrabber = new ScreenGrabber();

			if(!powerManager.isScreenOn())
			{
				powerManager.turnScreenOn(serviceManager);
			}

	        Size size = displayInfo.getSize();
			m_iWidth = size.getWidth();
			m_iHeight = size.getHeight();
			
			LogD.LogConsole(displayInfo.toString());

			try 
			{
				sendHandshake(m_ServerOutputStream, Build.MODEL, m_iWidth, m_iHeight);
				screenGrabber.initialize(SOCKET_NAME, m_iWidth, m_iHeight, 0, 0);
				screenGrabber.start(serviceManager, m_ServerInputStream, m_ServerOutputStream);
			}
			catch(Exception e)
			{
				LogD.LogConsole("Exception in RemoteServer::main() = "+e.toString());
			}
		}
		
		LogD.LogConsole("Good Bye!");
    }	
}