APP_PLATFORM := android-21
# libc++ has cleaner static initialization than stlport; avoids SIGBUS on
# Samsung kernels with strict alignment enforcement (stlport had unaligned
# accesses in its global constructors).
APP_STL := c++_static
APP_OPTIM        := release
# Build both 32-bit (ARMv7) and 64-bit (ARM64) so modern Android devices
# (which require 64-bit libraries) can install the APK.
APP_ABI          := armeabi-v7a arm64-v8a
