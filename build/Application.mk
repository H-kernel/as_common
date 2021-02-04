
APP_ABI := armeabi armeabi-v7a arm64-v8a x86 x86_64

APP_MODULES = hello-jni

# 指定目标Android平台的名称
APP_PLATFORM = android-26

# 是否支持C++标准库
APP_STL := stlport_static

# 为项目中的所有C++编译传递的标记
APP_CPPFLAGS := -frtti -fexceptions -std=c++11