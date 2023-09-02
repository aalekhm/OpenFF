
echo STEP 0 - Generating Java from Aidl...

cd %AIDL_DIR%
%ANDROID_HOME%/build-tools/%ANDROID_BUILD_TOOLS_VERSION%/aidl -o %CUR_DIR%/java android/view/IRotationWatcher.aidl
%ANDROID_HOME%/build-tools/%ANDROID_BUILD_TOOLS_VERSION%/aidl -o %CUR_DIR%/java android/content/IOnPrimaryClipChangedListener.aidl