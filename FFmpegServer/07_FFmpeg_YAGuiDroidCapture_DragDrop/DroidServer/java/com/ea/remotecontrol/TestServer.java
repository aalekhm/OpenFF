package com.ea.remotecontrol;

import android.net.LocalSocket;
import android.net.LocalServerSocket;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.IOException;
import android.util.Log;
import java.nio.charset.StandardCharsets;
	
class IO
{
   public static String streamToString(InputStream inputStream) throws IOException 
   {
      //creating an InputStreamReader object
      InputStreamReader isReader = new InputStreamReader(inputStream);
	  
      //Creating a BufferedReader object
      BufferedReader reader = new BufferedReader(isReader);
	  
      StringBuffer sb = new StringBuffer();
      String str;
      while((str = reader.readLine())!= null)
	  {
         sb.append(str);
      }
	  
	  return sb.toString();
   }
}

public class TestServer
{
	static final int HANDSHAKE_HEADER_SIZE = 8;
	static final int HANDSHAKE_DEVICENAME_SIZE = 64;
	static final int HANDSHAKE_DEVICEWIDTH_SIZE = 2;
	static final int HANDSHAKE_DEVICEHEIGHT_SIZE = 2;

	public static void sendHandshake(OutputStream outputStream) throws IOException
	{
		byte[] sHandshakeBuffer = new byte[	HANDSHAKE_HEADER_SIZE + 
											HANDSHAKE_DEVICENAME_SIZE + 
											HANDSHAKE_DEVICEWIDTH_SIZE + 
											HANDSHAKE_DEVICEHEIGHT_SIZE];
		
		sHandshakeBuffer[0] = 'D';
		sHandshakeBuffer[1] = 'R';
		sHandshakeBuffer[2] = 'O';
		sHandshakeBuffer[3] = 'I';
		sHandshakeBuffer[4] = 'D';
		
		String sDeviceName = "Google Pixel 3";
		System.arraycopy(sDeviceName.getBytes(), 0, sHandshakeBuffer, HANDSHAKE_HEADER_SIZE, sDeviceName.length());
		
		int iWidth = 480;
		int iHeight = 960;
		sHandshakeBuffer[HANDSHAKE_HEADER_SIZE + HANDSHAKE_DEVICENAME_SIZE] = (byte) iWidth;
		sHandshakeBuffer[HANDSHAKE_HEADER_SIZE + HANDSHAKE_DEVICENAME_SIZE + 1] = (byte) (iWidth >> 8);
		sHandshakeBuffer[HANDSHAKE_HEADER_SIZE + HANDSHAKE_DEVICENAME_SIZE + 2] = (byte) iHeight;
		sHandshakeBuffer[HANDSHAKE_HEADER_SIZE + HANDSHAKE_DEVICENAME_SIZE + 3] = (byte) (iHeight >> 8);
Log.d("ZZX", "Size of Handshake = "+sHandshakeBuffer.length);
Log.d("ZZX", "Sending Handshake = "+ new String(sHandshakeBuffer, StandardCharsets.UTF_8));
		outputStream.write(sHandshakeBuffer);
		outputStream.flush();
	}

    public static void start() 
	{
        new Thread(new Runnable() 
		{
            @Override
            public void run() 
			{
                //You can directly use the abstract name as the name of the socket
                LocalServerSocket serverSocket = null;
                try 
				{
                    serverSocket = new LocalServerSocket("scrcpy");
                    //blocking
                    LocalSocket client = serverSocket.accept();
                    Log.d("ZZX", "'scrcpy' connected to client");
					
					InputStream inputStream = client.getInputStream();
					OutputStream outputStream = client.getOutputStream();
					
					sendHandshake(outputStream);
					
                    while (true) 
					{
                        if (!client.isConnected()) 
						{
                            return;
                        }
                        
						int iSizeToRead = inputStream.read();
						Log.d("ZZX", "iSizeToRead = " + iSizeToRead);
						
						if(iSizeToRead == -1)
							break;
						
						byte[] buff = new byte[iSizeToRead];
						inputStream.read(buff, 0, iSizeToRead);
						
						String sReceived = new String(buff, StandardCharsets.UTF_8);
						Log.d("ZZX", "serverSocket recv =" + sReceived);
                    }
					
					System.out.println("'TestServer' exiting...");
                } 
				catch (IOException e) 
				{
                    e.printStackTrace();
                } 
				catch (Exception e) 
				{
                    e.printStackTrace();
                }
            }
        }).start();
    }
	
	public static void main(String[] args)
	{
		System.out.println("'TestServer' listening for connection on the device...");
		TestServer.start();
	}
}