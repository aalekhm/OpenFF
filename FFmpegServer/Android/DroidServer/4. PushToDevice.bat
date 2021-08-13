echo STEP 4 - PUSHING 'APK' to 'DEVICE'
echo Pushing '%APK%' to device @ '/data/local/tmp/'

adb.exe push %APK% /data/local/tmp/
REM adb.exe push Test.apk /data/local/tmp/