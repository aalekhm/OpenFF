package com.ea.remotecontrol.common;

import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.io.IOException;
import java.nio.channels.WritableByteChannel;
import java.nio.channels.Channels;
import java.nio.charset.StandardCharsets;
import java.nio.ByteOrder;

public class IO
{
	static WritableByteChannel m_pWritableByteChannel = null;
	public static void writeFully(OutputStream outputStream, ByteBuffer byteBuffer) throws IOException 
	{
		// As the ByteBuffer 'from' is readonly, it has to be converted to a non-RO before transmission.
		// It will give you a channel given an OutputStream. 
		// With the WritableByteChannel adapter you can provide the ByteBuffer which will write it to the OutputStream.
		// https://stackoverflow.com/questions/579600/how-to-put-the-content-of-a-bytebuffer-into-an-outputstream
		if(m_pWritableByteChannel == null)
		{
			m_pWritableByteChannel = Channels.newChannel(outputStream);
		}

		//WritableByteChannel channel = Channels.newChannel(outputStream);
		m_pWritableByteChannel.write(byteBuffer);
		
		//LogD.LogConsole("writeFully => size = "+iSize);
		//LogD.LogConsole("writeFully => Data => "+ new String(byteBuffer.array(), StandardCharsets.UTF_8));
	}
	
    public static void writeFully(OutputStream outputStream, byte[] buffer, int offset, int len) throws IOException 
	{	
        writeFully(outputStream, ByteBuffer.wrap(buffer, offset, len));
    }

	public static void writeBytes(OutputStream outputStream, byte[] bHeader) throws IOException
	{
		int iSize = bHeader.length;

		ByteBuffer pByteBuffer = ByteBuffer.allocate(iSize);
		{
			pByteBuffer.order(ByteOrder.LITTLE_ENDIAN);
			pByteBuffer.mark();
			{
				pByteBuffer.put(bHeader, 0, iSize);
			}
			pByteBuffer.flip();

			writeFully(outputStream, pByteBuffer);
			outputStream.flush();
		}
	}
}