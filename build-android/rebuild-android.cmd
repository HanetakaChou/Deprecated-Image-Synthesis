::
:: Copyright (C) YuqiaoZhang(HanetakaChou)
:: 
:: This program is free software: you can redistribute it and\or modify
:: it under the terms of the GNU Lesser General Public License as published
:: by the Free Software Foundation, either version 3 of the License, or
:: (at your option) any later version.
:: 
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU Lesser General Public License for more details.
:: 
:: You should have received a copy of the GNU Lesser General Public License
:: along with this program.  If not, see <https:\\www.gnu.org\licenses\>.
::

@echo off

:: SET ANDROID_NDK_ROOT=C:\Users\Public\Documents\android-sdk\ndk\25.2.9519653
:: SET ANDROID_SDK_ROOT=C:\Users\Public\Documents\android-sdk
:: SET JAVA_HOME=C:\Users\Public\Documents\jdk-17

call "%ANDROID_NDK_ROOT%\prebuilt\windows-x86_64\bin\make" -C "%~dp0." -f ".\APK.mk" clean
call "%ANDROID_NDK_ROOT%\prebuilt\windows-x86_64\bin\make" -C "%~dp0." -f ".\GLSL.mk" clean
call "%ANDROID_NDK_ROOT%\prebuilt\windows-x86_64\bin\make" -C "%~dp0." -f ".\GLSL.mk" "APP_DEBUG:=false"
call "%ANDROID_NDK_ROOT%\ndk-build" -C "%~dp0." NDK_PROJECT_PATH:=null "NDK_APPLICATION_MK:=.\Application.mk" "APP_BUILD_SCRIPT:=.\Android.mk" "NDK_OUT:=.\obj" "NDK_LIBS_OUT:=.\lib" "APP_DEBUG:=false" "APP_ABI:=arm64-v8a armeabi-v7a x86 x86_64" "APP_PLATFORM:=android-24" "APP_STL:=c++_static" -B
call "%ANDROID_NDK_ROOT%\prebuilt\windows-x86_64\bin\make" -C "%~dp0." -f ".\APK.mk" "ANDROID_SDK_DIR:=%ANDROID_SDK_ROOT%\." "JDK_DIR:=%JAVA_HOME%\." "ANDROID_SDK_BUILD_TOOLS_VERSION:=30.0.2" "APP_PLATFORM:=android-28"
