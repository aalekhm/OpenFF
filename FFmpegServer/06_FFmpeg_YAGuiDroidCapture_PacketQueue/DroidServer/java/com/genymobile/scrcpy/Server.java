package com.genymobile.scrcpy;

import android.os.SystemClock;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import java.io.IOException;
import android.os.IBinder;
import android.graphics.Rect;
import java.util.List;
import android.view.Surface;
import java.nio.ByteBuffer;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import java.io.FileDescriptor;
import android.util.Size;
import android.os.Build;
import java.nio.charset.StandardCharsets;
import android.system.Os;
import android.system.ErrnoException;
import android.system.OsConstants;

public class Server
{
	private static final String SOCKET_NAME = "scrcpy";
	private static final int DEVICE_NAME_FIELD_LENGTH = 64;
	static LocalSocket videoSocket;
	static LocalSocket controlSocket;
	private static FileDescriptor videoFd;
	private static final ByteBuffer headerBuffer = ByteBuffer.allocate(12);
	private static final int NO_PTS = -1;
	private static long ptsOrigin;
	private static boolean sendFrameMeta = true;
	
	private static int DEVICE_WIDTH = 1080;
	private static int DEVICE_HEIGHT = 2160;
	
	private static final Class<?> CLASS;
	static {
		try {
			CLASS = Class.forName("android.view.SurfaceControl");
		} 
		catch (ClassNotFoundException e) {
			throw new AssertionError(e);
		}
	}

	private static final int DEFAULT_I_FRAME_INTERVAL = 10; // seconds
    private static final int REPEAT_FRAME_DELAY_US = 100_000; // repeat after 100ms
    private static final String KEY_MAX_FPS_TO_ENCODER = "max-fps-to-encoder";
	
    private static MediaFormat createFormat(int bitRate, int maxFps/*, List<CodecOption> codecOptions*/) {
        MediaFormat format = new MediaFormat();
        format.setString(MediaFormat.KEY_MIME, MediaFormat.MIMETYPE_VIDEO_AVC);
        format.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
        // must be present to configure the encoder, but does not impact the actual frame rate, which is variable
        format.setInteger(MediaFormat.KEY_FRAME_RATE, 60);
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, DEFAULT_I_FRAME_INTERVAL);
        // display the very first frame, and recover from bad quality when no new frames
        format.setLong(MediaFormat.KEY_REPEAT_PREVIOUS_FRAME_AFTER, REPEAT_FRAME_DELAY_US); // µs
        if (maxFps > 0) {
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

	public static MediaCodec createCodec() throws IOException {
		return MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
	}

    private static IBinder createDisplay(String name, boolean secure) {
		try {
            return (IBinder) CLASS.getMethod("createDisplay", String.class, boolean.class).invoke(null, name, secure);
        } catch (Exception e) {
System.out.println("ERRRRRR");
            throw new AssertionError(e);
        }
    }

    private static void configure(MediaCodec codec, MediaFormat format) {
        codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
    }
	
	private static void setSize(MediaFormat format, int width, int height) {
        format.setInteger(MediaFormat.KEY_WIDTH, width);
        format.setInteger(MediaFormat.KEY_HEIGHT, height);
    }

    public static void openTransaction() {
        try {
            CLASS.getMethod("openTransaction").invoke(null);
        } catch (Exception e) {
System.out.println("ERRRRRR");
            throw new AssertionError(e);
        }
    }

    public static void closeTransaction() {
        try {
            CLASS.getMethod("closeTransaction").invoke(null);
        } catch (Exception e) {
System.out.println("ERRRRRR");
            throw new AssertionError(e);
        }
    }

	//class SurfaceControl
	//{
		public static void setDisplaySurface(IBinder displayToken, Surface surface) {
			try {
				CLASS.getMethod("setDisplaySurface", IBinder.class, Surface.class).invoke(null, displayToken, surface);
			} catch (Exception e) {
System.out.println("ERRRRRR");
				throw new AssertionError(e);
			}
		}
	
		public static void setDisplayProjection(IBinder displayToken, int orientation, Rect layerStackRect, Rect displayRect) {
			try {
				CLASS.getMethod("setDisplayProjection", IBinder.class, int.class, Rect.class, Rect.class)
						.invoke(null, displayToken, orientation, layerStackRect, displayRect);
			} catch (Exception e) {
System.out.println("ERRRRRR");
				throw new AssertionError(e);
			}
		}
	
		public static void setDisplayLayerStack(IBinder displayToken, int layerStack) {
			try {
				CLASS.getMethod("setDisplayLayerStack", IBinder.class, int.class).invoke(null, displayToken, layerStack);
			} catch (Exception e) {
System.out.println("ERRRRRR");
				throw new AssertionError(e);
			}
		}
		
		public static void destroyDisplay(IBinder displayToken) {
			try {
				CLASS.getMethod("destroyDisplay", IBinder.class).invoke(null, displayToken);
			} catch (Exception e) {
System.out.println("ERRRRRR");
				throw new AssertionError(e);
			}
		}
	
	
		private static void setDisplaySurface(IBinder display, Surface surface, int orientation, Rect deviceRect, Rect displayRect, int layerStack) {
			openTransaction();
			try {
				setDisplaySurface(display, surface);
				setDisplayProjection(display, orientation, deviceRect, displayRect);
				setDisplayLayerStack(display, layerStack);
			} finally {
				closeTransaction();
			}
		
		}
	//}
	
	private static boolean encode(MediaCodec codec, FileDescriptor fd) throws IOException {
        boolean eof = false;
        MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();

boolean consumeRotationChange = false;
        while (!consumeRotationChange && !eof) {
            int outputBufferId = codec.dequeueOutputBuffer(bufferInfo, -1);
            eof = (bufferInfo.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0;
            try {
                if (consumeRotationChange) {
                    // must restart encoding with new size
                    break;
                }
				
                if (outputBufferId >= 0) {
                    ByteBuffer codecBuffer = codec.getOutputBuffer(outputBufferId);

//System.out.println("codecBuffer  = "+codecBuffer.toString());
                    if (sendFrameMeta) {
                        writeFrameMeta(fd, bufferInfo, codecBuffer.remaining());
                    }

					writeFully(fd, codecBuffer);
                }
            } 
			catch(Exception e)
			{
System.out.println("ERRRRRR");
			}
			finally {
                if (outputBufferId >= 0) {
                    codec.releaseOutputBuffer(outputBufferId, false);
                }
            }
        }

        return !eof;
    }

    public static void writeFully(FileDescriptor fd, ByteBuffer from) throws IOException {
        // ByteBuffer position is not updated as expected by Os.write() on old Android versions, so
        // count the remaining bytes manually.
        // See <https://github.com/Genymobile/scrcpy/issues/291>.
        int remaining = from.remaining();
        while (remaining > 0) {
            try {
                int w = Os.write(fd, from);
                if (true && w < 0) {
System.out.println("ERRRRRR");
                    // w should not be negative, since an exception is thrown on error
                    throw new AssertionError("Os.write() returned a negative value (" + w + ")");
                }
                remaining -= w;
            } catch (ErrnoException e) {
System.out.println("ERRRRRR");
                if (e.errno != OsConstants.EINTR) {
                    throw new IOException(e);
                }
            }
        }
    }

    public static void writeFully(FileDescriptor fd, byte[] buffer, int offset, int len) throws IOException {
        writeFully(fd, ByteBuffer.wrap(buffer, offset, len));
    }
	
	private static void writeFrameMeta(FileDescriptor fd, MediaCodec.BufferInfo bufferInfo, int packetSize) throws IOException {
        headerBuffer.clear();

        long pts;
        if ((bufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
            pts = NO_PTS; // non-media data packet
        } else {
            if (ptsOrigin == 0) {
                ptsOrigin = bufferInfo.presentationTimeUs;
            }
            pts = bufferInfo.presentationTimeUs - ptsOrigin;
        }

        headerBuffer.putLong(pts);
        headerBuffer.putInt(packetSize);
        headerBuffer.flip();
        writeFully(fd, headerBuffer);
    }

    public static void main(String... args) 
	{
		System.out.print("Hello, ");
		//SystemClock.sleep(1000);
        System.out.println("World!!!");
		
		videoSocket = new LocalSocket();
		controlSocket = new LocalSocket();
		try {
			videoSocket.connect(new LocalSocketAddress(SOCKET_NAME));
			controlSocket.connect(new LocalSocketAddress(SOCKET_NAME));
		}
		catch(IOException e)
		{
System.out.println("Error connecting to "+SOCKET_NAME+", e = "+e.toString());
			return;
		}
		
		videoFd = videoSocket.getFileDescriptor();
		Size videoSize = new Size(DEVICE_WIDTH, DEVICE_HEIGHT);
		{
			String deviceName = Build.MODEL;
			int width = videoSize.getWidth();
			int height = videoSize.getHeight();
			
			byte[] buffer = new byte[DEVICE_NAME_FIELD_LENGTH + 4];

			byte[] deviceNameBytes = deviceName.getBytes(StandardCharsets.UTF_8);
			int len = deviceName.length();//StringUtils.getUtf8TruncationIndex(deviceNameBytes, DEVICE_NAME_FIELD_LENGTH - 1);
System.out.println("len "+len);
			System.arraycopy(deviceNameBytes, 0, buffer, 0, len);
			// byte[] are always 0-initialized in java, no need to set '\0' explicitly

			buffer[DEVICE_NAME_FIELD_LENGTH] = (byte) (width >> 8);
			buffer[DEVICE_NAME_FIELD_LENGTH + 1] = (byte) width;
			buffer[DEVICE_NAME_FIELD_LENGTH + 2] = (byte) (height >> 8);
			buffer[DEVICE_NAME_FIELD_LENGTH + 3] = (byte) height;
			
			try{
				writeFully(videoFd, buffer, 0, buffer.length);
System.out.println("buffeLength = "+buffer.length+", buffer = "+buffer+", width = "+width+", height = "+height);
				for(int i = 0; i < buffer.length; i++)
					System.out.print(""+(char)buffer[i]);
System.out.println("");
			}
			catch(Exception e)
			{
System.out.println("Error sending Handshake to "+SOCKET_NAME+", e = "+e.toString());
				return;
			}
		}
	
		MediaFormat mediaFormat = createFormat(8000000, 0);
		System.out.println("mediaFormat = "+mediaFormat);
		try {
			MediaCodec mediaCodec = createCodec();
			System.out.println("mediaCodec = "+mediaCodec);
			
			IBinder display = createDisplay("scrcpy", true);
			System.out.println("display = "+display);
			
            Rect contentRect = new Rect(0, 0, DEVICE_WIDTH, DEVICE_HEIGHT);
			Rect videoRect = new Rect(0, 0, DEVICE_WIDTH, DEVICE_HEIGHT);
			Rect unlockedVideoRect = new Rect(0, 0, DEVICE_WIDTH, DEVICE_HEIGHT);
			int videoRotation = 0;
			int layerStack = 0;
			
			//setSize(mediaFormat, videoRect.width(), videoRect.height());
			{
				mediaFormat.setInteger(MediaFormat.KEY_WIDTH, videoRect.width());
				mediaFormat.setInteger(MediaFormat.KEY_HEIGHT, videoRect.height());
			}
			mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
			Surface surface = mediaCodec.createInputSurface();
			setDisplaySurface(display, surface, videoRotation, contentRect, unlockedVideoRect, layerStack);
			mediaCodec.start();
			try {
				boolean alive = encode(mediaCodec, videoFd);
				// do not call stop() on exception, it would trigger an IllegalStateException
				mediaCodec.stop();
			} finally {
System.out.println("In finally!");
				destroyDisplay(display);
				mediaCodec.release();
				surface.release();
			}

		}
		catch(Exception e)
		{
System.out.println("In catch");
		}
		finally {
		}
    }	
}