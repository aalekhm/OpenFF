echo STEP 2 - DEXING
echo dexing classes from '%DIR%' to 'classes.dex'
%ANDROID_HOME%/build-tools/29.0.3/dx --dex --output classes.dex %DIR%/*.class %DIR%/common/*.class %DIR%/reflection/*.class
REM %ANDROID_HOME%/build-tools/29.0.3/dx --dex --output classes.dex %DIR%/*.class
REM %ANDROID_HOME%/build-tools/29.0.3/dx --dex --output classes.dex com/ea/Test.class