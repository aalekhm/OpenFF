@echo off

REM *********************** VARIABLE INITIALIZATION ***********************
SET CUR_DIR=%CD%

SET TEMP_DIR=%CUR_DIR%\temp
SET TEMP_CLASSES_DIR=%TEMP_DIR%\classes
SET TEMP_OUT_DIR=%TEMP_DIR%\out

SET SRC_PACKAGE_DIR=%1
SET CLASS_TO_COMPILE=%2
SET APK=%TEMP_OUT_DIR%\%3
SET DEST=%CUR_DIR%\%4

SET ANDROID_BUILD_TOOLS_VERSION=30.0.3
SET ANDROID_BUILD_PLATFORM=android-30
SET AIDL_DIR=%CUR_DIR%\aidl

echo arg[0] SRC_PACKAGE_DIR=%DIR%
echo arg[1] CLASS_TO_COMPILE=%CLASS_TO_COMPILE%
echo arg[2] APK=%APK%
echo arg[3] DEST=%DEST%
echo arg[4] TEMP_CLASSES_DIR=%TEMP_CLASSES_DIR%

REM *********************** DELETE TEMP ***********************
if exist %TEMP_DIR%\ (
	echo Yes
	rmdir /Q /S %TEMP_DIR%
) 

REM *********************** CREATE TEMP ***********************
mkdir %TEMP_DIR%
mkdir %TEMP_CLASSES_DIR%
mkdir %TEMP_OUT_DIR%

REM *********************** START BUILD ***********************

REM *********************** CONVERT AIDL TO JAVA SRC ***********************
call "0. JavaToAidl.bat"
echo.
cd %CUR_DIR%

REM *********************** COMPILE JAVA SRC ***********************
call "1. CompileSource.bat" %CLASS_TO_COMPILE%
echo.
cd %CUR_DIR%

REM *********************** DEX JAVA CLASSES ***********************
call "2. DexIt.bat"
echo.
cd %CUR_DIR%

REM *********************** PACKAGE DEX CLASSED TO APK ***********************
call "3. PackageAnAPK.bat"
echo.
cd %CUR_DIR%

REM *********************** COPY APK TO DESTINATION ***********************
call "6. CopyClientToDest" %APK% %DEST%
echo.
cd %CUR_DIR%