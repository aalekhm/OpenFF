package com.ea.remotecontrol.common;

import android.os.IBinder;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.view.Surface;
import android.graphics.Rect;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.KeyCharacterMap;

import com.ea.remotecontrol.reflection.SurfaceManager;
import com.ea.remotecontrol.reflection.ServiceManager;
import com.ea.remotecontrol.reflection.InputManager;
import com.ea.remotecontrol.common.IO;
import com.ea.remotecontrol.common.LogD;

public class ScreenGrabber
{
	private Surface 		m_Surface = null;
	private MediaCodec 		m_MediaCodec = null;
	private IBinder 		m_Display = null;
	private ServiceManager	m_ServiceManager = null;
	private InputManager	m_InputManager = null;
	
	private static final ByteBuffer headerBuffer = ByteBuffer.allocate(16);
	private static boolean m_bSendFrameMeta = true;
	private static final int NO_PTS = -1;
	private static long ptsOrigin;

	private static final int DEFAULT_I_FRAME_INTERVAL = 10; // seconds
    private static final int REPEAT_FRAME_DELAY_US = 100_000; // repeat after 100ms
    private static final String KEY_MAX_FPS_TO_ENCODER = "max-fps-to-encoder";

	public ScreenGrabber()
	{
	}
	
	public void initialize(String sSocketName, int iWidth, int iHeight, int iRotation, int iLayerStack)
	{
		try
		{
			m_Display = SurfaceManager.createDisplay(sSocketName, true);
			
			MediaFormat mediaFormat = createFormat(8000000, 0);
			{
				mediaFormat.setInteger(MediaFormat.KEY_WIDTH, iWidth);
				mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, iHeight);
			}
		
			m_MediaCodec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
			{
				m_MediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
				m_Surface = m_MediaCodec.createInputSurface();
				
				setDisplaySurface(	m_Display, 
									m_Surface, 
									iRotation, 
									new Rect(0, 0, iWidth, iHeight), 
									new Rect(0, 0, iWidth, iHeight), 
									iLayerStack);
				m_MediaCodec.start();
			}
		}
		catch(Exception e)
		{
			LogD.LogConsole("Exception in ScreenGrabber::initialize()");
			
			e.printStackTrace();
			throw new AssertionError(e);
		}
	}
	
    private static MediaFormat createFormat(int bitRate, int maxFps/*, List<CodecOption> codecOptions*/) 
	{
        MediaFormat format = new MediaFormat();
        format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_AVC);
        format.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
		
        // must be present to configure the encoder, but does not impact the actual frame rate, which is variable
        format.setInteger(MediaFormat.KEY_FRAME_RATE, 60);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, DEFAULT_I_FRAME_INTERVAL);
		
        // display the very first frame, and recover from bad quality when no new frames
        format.setLong(MediaFormat.KEY_REPEAT_PREVIOUS_FRAME_AFTER, REPEAT_FRAME_DELAY_US); // µs
        if (maxFps > 0) 
		{
            // The key existed privately before Android 10:
            // <https://android.googlesource.com/platform/frameworks/base/+/625f0aad9f7a259b6881006ad8710adce57d1384%5E%21/>
            // <https://github.com/Genymobile/scrcpy/issues/488#issuecomment-567321437>
            format.setFloat(KEY_MAX_FPS_TO_ENCODER, maxFps);
        }

        //if (codecOptions != null) {
        //    for (CodecOption option : codecOptions) {
        //        setCodecOption(format, option);
        //    }
        //}

        return format;
    }
	
	private static void setDisplaySurface(	IBinder display, 
											Surface surface, 
											int orientation, 
											Rect deviceRect, 
											Rect displayRect, 
											int layerStack)
	{
		SurfaceManager.openTransaction();
		try 
		{
			SurfaceManager.setDisplaySurface(display, surface);
			SurfaceManager.setDisplayProjection(display, orientation, deviceRect, displayRect);
			SurfaceManager.setDisplayLayerStack(display, layerStack);
		} 
		finally 
		{
			SurfaceManager.closeTransaction();
		}
	}
	
	private static void writeFrameMeta(OutputStream outputStream, MediaCodec.BufferInfo bufferInfo, int packetSize) throws IOException 
	{
        headerBuffer.clear();
		headerBuffer.order(ByteOrder.LITTLE_ENDIAN);

        long pts;
        if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) 
		{
            pts = NO_PTS; // non-media data packet
        } 
		else 
		{
            if (ptsOrigin == 0) 
			{
                ptsOrigin = bufferInfo.presentationTimeUs;
            }
			
            pts = bufferInfo.presentationTimeUs - ptsOrigin;
        }

		headerBuffer.put((byte)'D');
		headerBuffer.put((byte)'A');
		headerBuffer.put((byte)'T');
		headerBuffer.put((byte)'A');
		
        headerBuffer.putLong(pts);
        headerBuffer.putInt(packetSize);
		
        headerBuffer.flip();
		
        IO.writeFully(outputStream, headerBuffer);
		outputStream.flush();
    }

	private static boolean encode(MediaCodec codec, OutputStream outputStream) throws IOException 
	{
        boolean eof = false;
        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();

		boolean consumeRotationChange = false;
        while (!consumeRotationChange && !eof) 
		{
            int outputBufferId = codec.dequeueOutputBuffer(bufferInfo, -1);
            eof = (bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;
            try 
			{
                if (consumeRotationChange) 
				{
                    // must restart encoding with new size
                    break;
                }
				
                if (outputBufferId >= 0) 
				{
                    ByteBuffer codecBuffer = codec.getOutputBuffer(outputBufferId);
                    if (m_bSendFrameMeta) 
					{
                        writeFrameMeta(outputStream, bufferInfo, codecBuffer.remaining());
                    }
					
					IO.writeFully(outputStream, codecBuffer);
                }
            } 
			catch(Exception e)
			{
				LogD.LogConsole("Exception in ScreenGrabber::encode() = "+e.toString());
				LogD.LogConsole("Breaking out!");
				
				break;
			}
			finally 
			{
                if (outputBufferId >= 0) 
				{
                    codec.releaseOutputBuffer(outputBufferId, false);
                }
            }
        }

        return !eof;
    }
	
	public void start(ServiceManager serviceManager, InputStream inputStream, OutputStream outputStream)
	{
		this.m_ServiceManager = serviceManager;
		this.m_InputManager = m_ServiceManager.getInputManager();
		
		startStreamThread(outputStream);
		captureInput(inputStream);
	}
	
	private Thread m_StreamThread = null;
	private void startStreamThread(final OutputStream outputStream)
	{
		m_StreamThread = new Thread()
		{
			public void run()
			{
				try
				{
					boolean alive = encode(m_MediaCodec, outputStream);
				}
				catch(Exception e)
				{
					LogD.LogConsole("Exception in ScreenGrabber::startStreamThread()");
					
					e.printStackTrace();
					throw new AssertionError(e);
				}
				finally 
				{
					LogD.LogConsole("Cleaning up resources!");
					
					// do not call stop() on exception, it would trigger an IllegalStateException
					m_MediaCodec.stop();

					SurfaceManager.destroyDisplay(m_Display);
					m_MediaCodec.release();
					m_Surface.release();
				}
			}
		};
		
		m_StreamThread.start();
	}
	
	private static final int BUFFER_INFO_SIZE 		= 2;
	private static final int BUFFER_MOUSEDATA_SIZE 	= 10;
	private static final int BUFFER_KEYDATA_SIZE 	= 9;
	
	private final ByteBuffer m_SWriteInfo = ByteBuffer.allocate(BUFFER_INFO_SIZE);
	private final ByteBuffer m_SPointerInfo = ByteBuffer.allocate(BUFFER_MOUSEDATA_SIZE);
	private final ByteBuffer m_SKeyInfo = ByteBuffer.allocate(BUFFER_KEYDATA_SIZE);
	
	private void captureInput(InputStream inputStream)
	{
		m_SWriteInfo.order(ByteOrder.LITTLE_ENDIAN);
		m_SPointerInfo.order(ByteOrder.LITTLE_ENDIAN);
		m_SKeyInfo.order(ByteOrder.LITTLE_ENDIAN);
		
		boolean bEOF = false;
		do
		{
			try
			{
				////////////////////////////////////////////////////////////////////////////////////////////////
				//struct SWriteInfo
				//{
				//	char		MAGIC;		// 'K' - Keyboard Input
				//							// 'M' - Mouse Input
				//	int8_t		m_iSize;	// Size of the next data to follow
				//};
				////////////////////////////////////////////////////////////////////////////////////////////////

				m_SWriteInfo.clear();
				inputStream.read(m_SWriteInfo.array(), 0, BUFFER_INFO_SIZE);
				
				byte MAGIC = m_SWriteInfo.get();
				byte iSize = m_SWriteInfo.get();
				
				//LogD.LogConsole("MAGIC = "+(char)MAGIC);
				switch(MAGIC)
				{
					case 'Q': // Quit Signal
					{
						bEOF = true;
					}
					break;
					case 'M': // Mouse Inputs
					{
						////////////////////////////////////////////////////////////////////////////////////////////////
						//struct SPointerInfo
						//{
						//	uint8_t		m_iAction;
						//	uint8_t		m_iKey;
						//	uint32_t	m_iPosX;
						//	uint32_t	m_iPosY;
						//};
						////////////////////////////////////////////////////////////////////////////////////////////////
						
						m_SPointerInfo.clear();
						inputStream.read(m_SPointerInfo.array(), 0, BUFFER_MOUSEDATA_SIZE);
						
						byte iAction 	= m_SPointerInfo.get();
						byte iKey 		= m_SPointerInfo.get();
						int iPosX 		= m_SPointerInfo.getInt();
						int iPosY 		= m_SPointerInfo.getInt();
						
						boolean bInjectResult = m_InputManager.injectMouseInput(iAction, iKey, iPosX, iPosY);						
					}
					break;
					case 'K': // Keyboard Inputs
					{
						////////////////////////////////////////////////////////////////////////////////////////////////
						//struct SKeyInfo
						//{
						//	uint8_t		m_iAction;
						//	uint32_t	m_iKey;
						//};
						////////////////////////////////////////////////////////////////////////////////////////////////
						
						m_SKeyInfo.clear();
						inputStream.read(m_SKeyInfo.array(), 0, BUFFER_KEYDATA_SIZE);
						
						byte iAction 		= m_SKeyInfo.get();
						int iAsciiKeyCode 	= m_SKeyInfo.getInt();
						int iMetaModifier 	= m_SKeyInfo.getInt();
						
						m_InputManager.injectKeyInput(iAction, iAsciiKeyCode, iMetaModifier);
					}
					break;
				}
			}
			catch(Exception e)
			{
				LogD.LogConsole("Exception in ScreenGrabber::captureInput()");
				e.printStackTrace();
				throw new AssertionError(e);
			}
		}
		while(!bEOF);
	}
}