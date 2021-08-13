package com.ea.remotecontrol.common;

import android.os.IBinder;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.view.Surface;
import android.graphics.Rect;
import java.io.IOException;
import java.io.InputStream;
import java.io.DataInputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.KeyCharacterMap;
import java.io.EOFException;

import com.ea.remotecontrol.reflection.SurfaceManager;
import com.ea.remotecontrol.reflection.ServiceManager;
import com.ea.remotecontrol.reflection.InputManager;
import com.ea.remotecontrol.reflection.WindowManager;
import com.ea.remotecontrol.common.IO;
import com.ea.remotecontrol.common.LogD;

import java.util.concurrent.atomic.AtomicBoolean;

public class ScreenEncoder implements Device.RotationListener
{
	private Surface 			m_Surface = null;
	private MediaCodec 			m_MediaCodec = null;
	private IBinder 			m_Display = null;
	private ServiceManager		m_ServiceManager = null;
	private InputManager		m_InputManager = null;
	private WindowManager		m_WindowManager = null;
	
	private Thread				m_StreamThread = null;
	private Thread				m_InputCaptureThread = null;

	private ByteBuffer			m_SWriteInfo = null;
	private ByteBuffer			m_SPointerInfo = null;
	private ByteBuffer			m_SKeyInfo = null;

	private static boolean		m_bSendFrameMeta = false;
	private static boolean		m_bRotationChange = false;
	private static int			m_iCurrentRotation = 0;

	private static final int	NO_PTS = -1;
	private static long			ptsOrigin;

	private static final int	BUFFER_INFO_SIZE 		= 2;
	private static final int	BUFFER_MOUSEDATA_SIZE 	= 10;
	private static final int	BUFFER_KEYDATA_SIZE 	= 9;
	
	private static final int 	HANDSHAKE_DEVICENAME_SIZE = 64;
	private static final int 	HANDSHAKE_DEVICEWIDTH_SIZE = 4;
	private static final int 	HANDSHAKE_DEVICEHEIGHT_SIZE = 4;

	private static final int	DEFAULT_I_FRAME_INTERVAL = 10; // seconds
    private static final int	REPEAT_FRAME_DELAY_US = 100_000; // repeat after 100ms
    private static final String	KEY_MAX_FPS_TO_ENCODER = "max-fps-to-encoder";

	public ScreenEncoder()
	{
	}

    @Override
    public void onRotationChanged(int iRotation) 
	{
		LogD.LogConsole("ScreenEncoder::onRotationChanged == "+iRotation);

		m_iCurrentRotation = iRotation;
		m_bRotationChange = true;
    }
	
	public void streamScreen(	String sSocketName, 
								ServiceManager serviceManager, 
								InputStream pInputStream, 
								OutputStream pOutputStream, 
								Device pDevice, 
								int iRotation, 
								int iLayerStack
	) {
		boolean bAlive = true;

		this.m_ServiceManager = serviceManager;
		this.m_InputManager = m_ServiceManager.getInputManager();
		this.m_WindowManager = m_ServiceManager.getWindowManager();

		pDevice.setRotationListener(this);

		do 
		{
			try
			{
				DataInputStream pDataInputStream = new DataInputStream(pInputStream);

				int iWidth = pDevice.getWidth();
				int iHeight = pDevice.getHeight();

				// 1. Send Handshake
				LogD.LogConsole("Sending Handshake from "+pDevice.getModelName()+" with Width = "+iWidth+" and Height = "+iHeight);
				sendHandshake(pOutputStream, pDevice.getModelName(), iWidth, iHeight);

				// 2. Display
				m_Display = SurfaceManager.createDisplay(sSocketName, true);

				// 3. MediaFormat
				MediaFormat mediaFormat = createFormat(8000000, 0);
				{
					mediaFormat.setInteger(MediaFormat.KEY_WIDTH, iWidth);
					mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, iHeight);
				}

				// 4. MediaCodec
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

				// 5. Start Input Capture Thread
				captureInputStream(pDataInputStream);

				// 6. Stream Surface to Network
				bAlive = encodeMediaAndFlush(m_MediaCodec, pOutputStream);
			}
			catch(Exception e)
			{
				LogD.LogConsole("Exception in ScreenGrabber::streamScreen()");
			
				e.printStackTrace();
				throw new AssertionError(e);
			}
			finally 
			{
				LogD.LogConsole("Cleaning up resources!");
				
				// 1. Destroy the created Display
				SurfaceManager.destroyDisplay(m_Display);

				// 2. Stop & Release the MediaCodec
				m_MediaCodec.stop(); // do not call stop() on exception, it would trigger an IllegalStateException
				m_MediaCodec.release();

				// 3. Release the Surface
				m_Surface.release();

				// 4. Reset Variables
				m_bRotationChange = false;
				m_bSendFrameMeta = false;

				// 5. Clear Input Stream Buffers
				m_SWriteInfo.clear();
				m_SPointerInfo.clear();
				m_SKeyInfo.clear();
			}
		}
		while(bAlive);
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

	public static void sendHandshake(OutputStream outputStream, String sDeviceName, int iWidth, int iHeight) throws IOException
	{
		byte[] sMagic = {'D', 'R', 'O', 'I', 'D', '\0', '\0', '\0'};
		IO.writeBytes(outputStream, sMagic);

		int iSize = HANDSHAKE_DEVICENAME_SIZE + 
					HANDSHAKE_DEVICEWIDTH_SIZE + 
					HANDSHAKE_DEVICEHEIGHT_SIZE;
					
		ByteBuffer pHandshakeBuffer = ByteBuffer.allocate(iSize);
		pHandshakeBuffer.order(ByteOrder.LITTLE_ENDIAN);
		pHandshakeBuffer.mark();
		
		pHandshakeBuffer.put(sDeviceName.getBytes());
		pHandshakeBuffer.position(HANDSHAKE_DEVICENAME_SIZE);
		
		pHandshakeBuffer.putInt(iWidth);
		pHandshakeBuffer.putInt(iHeight);
		pHandshakeBuffer.flip();
		
		//LogD.LogConsole("Handshake => size = "+iSize);
		//LogD.LogConsole("Handshake => Data => "+ new String(pHandshakeBuffer.array(), StandardCharsets.UTF_8));
		outputStream.write(pHandshakeBuffer.array());
		
		outputStream.flush();
	}

	private static void writeFrameMeta(OutputStream outputStream, MediaCodec.BufferInfo pBufferInfo, int packetSize) throws IOException 
	{
		byte[] sMagic = {'D', 'A', 'T', 'A', '\0', '\0', '\0', '\0'};
		IO.writeBytes(outputStream, sMagic);

		ByteBuffer sDataHeader = ByteBuffer.allocate(12);

        sDataHeader.clear();
		sDataHeader.order(ByteOrder.LITTLE_ENDIAN);

        long pts;
        if ((pBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) 
		{
            pts = NO_PTS; // non-media data packet
        } 
		else 
		{
            if (ptsOrigin == 0) 
			{
                ptsOrigin = pBufferInfo.presentationTimeUs;
            }
			
            pts = pBufferInfo.presentationTimeUs - ptsOrigin;
        }

        sDataHeader.putLong(pts);
        sDataHeader.putInt(packetSize);
		
        sDataHeader.flip();
		
        IO.writeFully(outputStream, sDataHeader);
		outputStream.flush();
    }

	private static boolean encodeMediaAndFlush(MediaCodec codec, OutputStream outputStream) throws IOException 
	{
        boolean bEOF = false;
		int iOutputBufferId = -1;
        MediaCodec.BufferInfo pBufferInfo = new MediaCodec.BufferInfo();

		while (!m_bRotationChange && !bEOF)
		{	
            try 
			{
				if (m_bSendFrameMeta)
				{
					iOutputBufferId = codec.dequeueOutputBuffer(pBufferInfo, -1);
					bEOF = (pBufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;

					if (iOutputBufferId >= 0) 
					{
					    ByteBuffer codecBuffer = codec.getOutputBuffer(iOutputBufferId);
						writeFrameMeta(outputStream, pBufferInfo, codecBuffer.remaining());
						
						IO.writeFully(outputStream, codecBuffer);
					}
				}
            } 
			catch(Exception e)
			{
				LogD.LogConsole("Exception in ScreenGrabber::encodeMediaAndFlush() = "+e.toString());
				LogD.LogConsole("Breaking out!");
				
				bEOF = true;
				break;
			}
			finally 
			{
                if (m_bSendFrameMeta && iOutputBufferId >= 0) 
				{
                    codec.releaseOutputBuffer(iOutputBufferId, false);
                }
            }
        }

        return !bEOF;
    }
	
	private void startStreamThread(final OutputStream outputStream)
	{
		m_StreamThread = new Thread()
		{
			public void run()
			{
				try
				{
					boolean alive = encodeMediaAndFlush(m_MediaCodec, outputStream);
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
	
	private void captureInputStream(final DataInputStream pDataInputStream)
	{
		if(m_InputCaptureThread == null)
		{
			m_SWriteInfo = ByteBuffer.allocate(BUFFER_INFO_SIZE);
			m_SPointerInfo = ByteBuffer.allocate(BUFFER_MOUSEDATA_SIZE);
			m_SKeyInfo = ByteBuffer.allocate(BUFFER_KEYDATA_SIZE);

			m_SWriteInfo.order(ByteOrder.LITTLE_ENDIAN);
			m_SPointerInfo.order(ByteOrder.LITTLE_ENDIAN);
			m_SKeyInfo.order(ByteOrder.LITTLE_ENDIAN);
		
			m_InputCaptureThread = new Thread()
			{
				public void run()
				{
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
							pDataInputStream.readFully(m_SWriteInfo.array());

							byte MAGIC = m_SWriteInfo.get();				
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
									pDataInputStream.readFully(m_SPointerInfo.array());

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
									pDataInputStream.readFully(m_SKeyInfo.array());
						
									byte iAction 		= m_SKeyInfo.get();
									int iAsciiKeyCode 	= m_SKeyInfo.getInt();
									int iMetaModifier 	= m_SKeyInfo.getInt();
						
									m_InputManager.injectKeyInput(iAction, iAsciiKeyCode, iMetaModifier);
								}
								break;
								case 'S': // Start Sending Frame Meta
								{
									LogD.LogConsole("Start Sending Frame Meta");

									m_bSendFrameMeta = true;
								}
								break;
								case 'F': // Freeze Rotation
								{
									LogD.LogConsole("Freeze Rotation");
									m_WindowManager.freezeRotation(m_iCurrentRotation);
								}
								break;
								case 'T': // Thaw Rotation
								{
									//android.net.Uri number = android.net.Uri.parse("tel:5551234");
									//android.content.Intent callIntent = new android.content.Intent(android.content.Intent.ACTION_CALL, number);

									LogD.LogConsole("Thaw Rotation");
									m_WindowManager.thawRotation();
								}
								break;
							}
						}
						catch(Exception e)
						{
							LogD.LogConsole("Exception in ScreenGrabber::captureInputStream()");

							e.printStackTrace();
							//throw new AssertionError(e);
						}
					}
					while(!bEOF);//m_bRotationChange);

					LogD.LogConsole("ENDING INPUT CAPTURE THREAD!");
				}
			};

			m_InputCaptureThread.start();
		}
	}
}