SET JAVA_SRC_CLASS=%1.java

echo STEP 1 - COMPILING
echo compiling '%SRC_PACKAGE_DIR%/%CLASS%'

cd java
javac -bootclasspath %ANDROID_HOME%/platforms/%ANDROID_BUILD_PLATFORM%/android.jar -d %TEMP_CLASSES_DIR% -source 1.7 -target 1.7 %SRC_PACKAGE_DIR%/%JAVA_SRC_CLASS%