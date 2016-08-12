APP_PLATFORM := android-10

# c++_static gnustl_static 
APP_STL :=  gnustl_static

APP_ABI := armeabi armeabi-v7a x86 arm64-v8a x86_64

#-std=c++11
APP_CPPFLAGS := -frtti -fsigned-char

# APP_CPPFLAGS := -frtti -std=c++11 -fsigned-char
#APP_LDFLAGS := -latomic

#USE_ARM_MODE := 1

APP_OPTIM := release
	