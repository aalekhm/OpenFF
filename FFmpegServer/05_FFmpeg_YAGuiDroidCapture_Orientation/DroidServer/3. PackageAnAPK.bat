echo STEP 3 - PACKAGING in a APK
echo Packaging 'classes.dex' in '%APK%'

cd %TEMP_OUT_DIR%
jar cvf %APK% classes.dex

REM jar cvf Test.apk classes.dex