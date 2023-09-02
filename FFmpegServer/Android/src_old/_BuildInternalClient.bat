@echo off
SET DIR=%1
SET CLASS=%2
SET APK=%3
SET DEST=%4

echo arg[0] DIR=%DIR%
echo arg[1] CLASS=%CLASS%
echo arg[2] APK=%APK%
echo arg[3] DEST=%DEST%
echo.

call "1. CompileSource.bat" %DIR% %CLASS%
echo.
call "2. DexIt.bat" %DIR% %CLASS% 
echo.
call "3. PackageAnAPK.bat" %APK%
echo.
call "6. CopyClientToDest" %DEST%
echo.