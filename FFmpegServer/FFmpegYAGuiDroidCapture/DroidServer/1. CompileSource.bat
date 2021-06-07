SET DIR=%1
SET JAVA=%2.java

echo STEP 1 - COMPILING
echo compiling '%DIR%/%CLASS%'
javac -source 1.7 -target 1.7 -cp %ANDROID_HOME%/platforms/android-29/android.jar;. %DIR%/%JAVA%
REM javac -source 1.7 -target 1.7 -cp %ANDROID_HOME%/platforms/android-29/android.jar com/ea/Test.java