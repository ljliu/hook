

#include <assert.h>
#include <string.h>


// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <errno.h>
#include <stdio.h>

#include "utils/AudioRecord.h"

#define LOG_TAG "AudioRecord"

#include "utils/LogUtils.h"

#define BUFFER_COUNT (8)

AudioRecord::AudioRecord(int sampleRate, int channel, int bufferSize) {
    SLresult result;

    // configure audio source
    SLDataLocator_IODevice loc_dev = {
        SL_DATALOCATOR_IODEVICE,
        SL_IODEVICE_AUDIOINPUT,
        SL_DEFAULTDEVICEID_AUDIOINPUT,
        NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    SLuint32 sample = SL_SAMPLINGRATE_16;
    if (sampleRate == 48000) {
        sample = SL_SAMPLINGRATE_48;
    }

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
        BUFFER_COUNT};
    SLDataFormat_PCM format_pcm = {
        SL_DATAFORMAT_PCM,
        2,
        sample,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
        SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*mEngineEngine)->CreateAudioRecorder(mEngineEngine,
                                                   &mRecorderObject,
                                                   &audioSrc,
                                                   &audioSnk,
                                                   1,
                                                   id,
                                                   req);
    ALOGE("result %d\n", result);
    assert(SL_RESULT_SUCCESS == result);

    // realize the audio recorder
    result = (*mRecorderObject)->Realize(mRecorderObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the record interface
    result = (*mRecorderObject)->GetInterface(mRecorderObject,
                                              SL_IID_RECORD,
                                              &mRecorderRecord);
    assert(SL_RESULT_SUCCESS == result);

    // get the buffer queue interface
    result = (*mRecorderObject)->GetInterface(mRecorderObject,
                                              SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                              &mRecorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);

    // register callback on the buffer queue
    result = (*mRecorderBufferQueue)->RegisterCallback(mRecorderBufferQueue,
                                                       bqRecorderCallback,
                                                       this);
    assert(SL_RESULT_SUCCESS == result);

    mBuffer = new char[bufferSize * BUFFER_COUNT];
    mSampleRate = sampleRate;
    mChannels = channel;
    mBufferSize = bufferSize;
    pthread_mutex_init(&mLock, NULL);
    pthread_cond_init(&mCond, NULL);
}

AudioRecord::~AudioRecord() {
    // destroy audio recorder object, and invalidate all associated interfaces
    if (mRecorderObject != NULL) {
        (*mRecorderObject)->Destroy(mRecorderObject);
        mRecorderObject = NULL;
        mRecorderRecord = NULL;
        mRecorderBufferQueue = NULL;
    }

    if (mBuffer != NULL) {
        delete [] mBuffer;
    }

    pthread_mutex_destroy(&mLock);
    pthread_cond_destroy(&mCond);
}



// this callback handler is called every time a buffer finishes recording
/*static*/
void AudioRecord::bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq,
                                     void *context) {
    AudioRecord *record = (AudioRecord *) context;
    record->doRecorderCallback(bq);
}

void AudioRecord::doRecorderCallback(SLAndroidSimpleBufferQueueItf bq) {
    assert(bq == mRecorderBufferQueue);

    pthread_mutex_lock(&mLock);

    // ALOGE("recv data");
    if (mWroteBufIndex == mReadBufIndex) {
        pthread_cond_signal(&mCond);
    }

    mWroteBufIndex++;
    mWroteBufIndex %= BUFFER_COUNT;

    if (mWroteBufIndex == mReadBufIndex) {
        ALOGD("full audio data DSP");
        mDataFull = 1;
    }
    pthread_mutex_unlock(&mLock);
    /*
    SLresult result =
        (*mRecorderBufferQueue)->Enqueue(mRecorderBufferQueue,
                                         buffer,
                                         mBufferSize);
    assert(SL_RESULT_SUCCESS == result);
    */
}

// set the recording state for the audio recorder
int AudioRecord::startRecording() {
    mWroteBufIndex = 0;
    mReadBufIndex = 0;
    mDataFull = 0;

    SLresult result;

    // in case already recording, stop recording and clear buffer queue
    result = (*mRecorderRecord)->SetRecordState(mRecorderRecord,
                                                SL_RECORDSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    result = (*mRecorderBufferQueue)->Clear(mRecorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // enqueue an empty buffer to be filled by the recorder

    for (int i = 0; i < BUFFER_COUNT; i++) {
        result = (*mRecorderBufferQueue)->Enqueue(mRecorderBufferQueue,
                                                  mBuffer + mBufferSize * i,
                                                  mBufferSize);
        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
        // which for this code example would indicate a programming error
        assert(SL_RESULT_SUCCESS == result);
    }

    // start recording
    result = (*mRecorderRecord)->SetRecordState(mRecorderRecord,
                                                SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);

    return 0;
}

int AudioRecord::stop() {
    SLresult result;

    // in case already recording, stop recording and clear buffer queue
    result = (*mRecorderRecord)->SetRecordState(mRecorderRecord,
                                                SL_RECORDSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    result = (*mRecorderBufferQueue)->Clear(mRecorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    ALOGD("AudioRecord stop");
    return 0;
}

int AudioRecord::obtainBuffer(char** buffer, bool blocked)
{
    pthread_mutex_lock(&mLock);

    while (mReadBufIndex == mWroteBufIndex && !mDataFull) {
        if (!blocked) {
            //ALOGD("buffer empty");
            pthread_mutex_unlock(&mLock);
            return 0;
        }
        //ALOGE("no data wait");
        pthread_cond_wait(&mCond, &mLock);
    }

    if (mDataFull) {
        mDataFull = 0;
    }

    *buffer = mBuffer + mBufferSize * mReadBufIndex;
    //ALOGE("has data %p", *buffer);
    mReadBufIndex++;
    mReadBufIndex %= BUFFER_COUNT;

    pthread_mutex_unlock(&mLock);

    return mBufferSize;
}

int AudioRecord::releaseBuffer(char* buffer)
{
    SLresult result = (*mRecorderBufferQueue)->Enqueue(mRecorderBufferQueue,
                                                       buffer,
                                                       mBufferSize);
    assert(SL_RESULT_SUCCESS == result);
    return 0;
}
