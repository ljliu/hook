//
// Created by chehongliang on 19-1-28.
//

#include <assert.h>
#include <stdio.h>

#include "NativeAudioBase.h"

NativeAudioBase::NativeAudioBase()
{
    SLresult result;
    // create engine
    result = slCreateEngine(&mEngineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);

    // realize the engine
    result = (*mEngineObject)->Realize(mEngineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the engine interface, which is needed in order to create other objects
    result = (*mEngineObject)->GetInterface(mEngineObject, SL_IID_ENGINE, &mEngineEngine);
    assert(SL_RESULT_SUCCESS == result);
}

NativeAudioBase::~NativeAudioBase()
{
    // destroy engine object, and invalidate all associated interfaces
    if (mEngineObject != NULL) {
        (*mEngineObject)->Destroy(mEngineObject);
        mEngineObject = NULL;
        mEngineEngine = NULL;
    }
}
