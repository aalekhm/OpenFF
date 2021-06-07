package com.ea.remotecontrol.common;

import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.io.IOException;
import java.nio.channels.WritableByteChannel;
import java.nio.channels.Channels;
import java.nio.charset.StandardCharsets;

public class IO
{
	public static void writeFully(OutputStream outputStream, ByteBuffer byteBuffer) throws IOException 
	{
		// As the ByteBuffer 'from' is readonly, it has to be converted to a non-RO before transmission.
		// It will give you a channel given an OutputStream. 
		// With the WritableByteChannel adapter you can provide the ByteBuffer which will write it to the OutputStream.
		// https://stackoverflow.com/questions/579600/how-to-put-the-content-of-a-bytebuffer-into-an-outputstream
		WritableByteChannel channel = Channels.newChannel(outputStream);
		channel.write(byteBuffer);
		
		//LogD.LogConsole("writeFully => size = "+iSize);
		//LogD.LogConsole("writeFully => Data => "+ new String(byteBuffer.array(), StandardCharsets.UTF_8));
	}
	
    public static void writeFully(OutputStream outputStream, byte[] buffer, int offset, int len) throws IOException 
	{	
        writeFully(outputStream, ByteBuffer.wrap(buffer, offset, len));
    }
}