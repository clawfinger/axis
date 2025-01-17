/*
** This file is auto generated by https://github.com/axis-project/buildware.git
*/
#ifndef BUILDWARE_openssl_REDIRECT_H
#define BUILDWARE_openssl_REDIRECT_H

#if defined(_WIN32)

#  if defined(_WIN64)
#    include "win64/openssl/configuration.h"
#  else
#    include "win32/openssl/configuration.h"
#  endif

#elif defined(__APPLE__)
#  include <TargetConditionals.h>

#  if TARGET_IPHONE_SIMULATOR == 1
#    include "ios-x64/openssl/configuration.h"
#  elif TARGET_OS_IPHONE == 1
#    if defined(__arm64__)
#      include "ios-arm64/openssl/configuration.h"
#    elif defined(__arm__)
#      include "ios-arm/openssl/configuration.h"
#    endif
#  elif TARGET_OS_MAC == 1
#    include "mac/openssl/configuration.h"
#  endif

#elif defined(__ANDROID__)

#  if defined(__aarch64__) || defined(__arm64__)
#    include "android-arm64/openssl/configuration.h"
#  elif defined(__arm__)
#    include "android-arm/openssl/configuration.h"
#  else
#    include "android-x86/openssl/configuration.h"
#  endif

#elif defined(__linux__)
#  include "linux/openssl/configuration.h"
#else
#  error "unsupported platform"
#endif

#endif
