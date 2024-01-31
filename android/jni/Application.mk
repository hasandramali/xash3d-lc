CFLAGS_OPT :=  -O3 -DNDEBUG -fomit-frame-pointer -funsafe-math-optimizations -ftree-vectorize -fgraphite-identity -floop-interchange -floop-block -funsafe-loop-optimizations -finline-limit=1024 -fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables
CFLAGS_OPT_ARM := -mthumb -mfpu=neon -mcpu=cortex-a9 -pipe -mvectorize-with-neon-quad -DVECTORIZE_SINCOS
CFLAGS_OPT_ARMv5 :=-march=armv6 -mfpu=vfp -marm -pipe
CFLAGS_OPT_X86 := -mtune=atom -march=atom -mssse3 -mfpmath=sse -funroll-loops -pipe -DVECTORIZE_SINCOS
CFLAGS_HARDFP := -D_NDK_MATH_NO_SOFTFP=1 -mhard-float -mfloat-abi=hard -DLOAD_HARDFP -DSOFTFP_LINK
APPLICATIONMK_PATH = $(call my-dir)

XASH3D_CONFIG := $(APPLICATIONMK_PATH)/mod_config.mk

APP_PLATFORM := android-9
APP_ABI := x86 armeabi-v7a-hard
APP_MODULES := server client
