//
// Created by chehongliang on 19-1-28.
//

#ifndef UTILS_NATIVE_AUDIOBASE_H
#define UTILS_NATIVE_AUDIOBASE_H 

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

class NativeAudioBase
{
public:
    NativeAudioBase();
    ~NativeAudioBase();

protected:
    SLObjectItf mEngineObject;
    SLEngineItf mEngineEngine;
};

#endif //UTILS_NATIVE_AUDIOBASE_H
