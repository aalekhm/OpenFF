echo STEP 2 - DEXING
echo dexing classes from '%CLASS_TO_COMPILE%' to 'classes.dex'

cd %TEMP_CLASSES_DIR%
%ANDROID_HOME%/build-tools/%ANDROID_BUILD_TOOLS_VERSION%/dx --dex --output %TEMP_OUT_DIR%/classes.dex %SRC_PACKAGE_DIR%/*.class %SRC_PACKAGE_DIR%/common/*.class %SRC_PACKAGE_DIR%/reflection/*.class
