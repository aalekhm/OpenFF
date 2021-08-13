set PACKAGE=%DIR:\=.%

echo STEP 5 - EXECUTING '%PACKAGE%.%CLASS%' from '%APK%' using app_process.

echo executing...
adb.exe shell CLASSPATH=/data/local/tmp/%APK% app_process / %PACKAGE%.%CLASS%
REM adb.exe shell CLASSPATH=/data/local/tmp/Test.apk app_process / com.ea.Test