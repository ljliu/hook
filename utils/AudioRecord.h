//
// Created by chehongliang on 19-1-18.
//

#ifndef UTILS_AUDIORECORD_H
#define UTILS_AUDIORECORD_H

#include <pthread.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "utils/NativeAudioBase.h"

class AudioRecord : public NativeAudioBase {

public:
    AudioRecord(int sampleRate, int channel, int bufferSize);
    ~AudioRecord();

    int startRecording();
    int stop();
    int obtainBuffer(char ** buffer, bool blocked = false);
    int releaseBuffer(char* buffer);

private:
    static void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    void doRecorderCallback(SLAndroidSimpleBufferQueueItf bq);

    int mSampleRate;
    int mChannels;
    int mBufferSize;
    char* mBuffer;

    int mWroteBufIndex;
    int mReadBufIndex;
    int mDataFull;

    pthread_mutex_t mLock;
    pthread_cond_t mCond;

    // recorder interfaces
    SLObjectItf mRecorderObject;
    SLRecordItf mRecorderRecord;
    SLAndroidSimpleBufferQueueItf mRecorderBufferQueue;

};

#endif // UTILS_AUDIORECORD_H
