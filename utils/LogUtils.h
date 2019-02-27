//
// Created by chehongliang on 19-1-18.
//

#ifndef MYAPPLICATION_LOGUTILS_H
#define MYAPPLICATION_LOGUTILS_H

#include <android/log.h>

#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define ALOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

#endif //MYAPPLICATION_LOGUTILS_H
