@echo off
SET DIR=%1
SET CLASS=%2
SET APK=%3

echo arg[0] DIR=%DIR%
echo arg[1] CLASS=%CLASS%
echo arg[2] APK=%APK%
echo.

call "1. CompileSource.bat" %DIR% %CLASS%
echo.
call "2. DexIt.bat" %DIR% %CLASS% 
echo.
call "3. PackageAnAPK.bat" %APK%
echo.
call "4. PushToDevice.bat" %APK%
echo.
call "5. Executing.bat" %DIR% %CLASS% %APK%
echo.
