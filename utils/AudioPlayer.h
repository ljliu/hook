//
// Created by chehongliang on 19-1-28.
//

#ifndef UTILS_AUDIOPLAYER_H
#define UTILS_AUDIOPLAYER_H

#include <pthread.h>

#include "utils/NativeAudioBase.h"

enum PlayerType
{
    STREAMING,
    URI
};

enum PlayerEvent
{
    PLAY_COMPLETE,
};

typedef void (*PlayerCallbackFunc) (int evnet);

class AudioPlayer : public NativeAudioBase
{
public:

    AudioPlayer(PlayerType type);
    ~AudioPlayer();

    int createStreamingAudioPlayer(unsigned sampleRate, unsigned channels, unsigned bufferSize);
    int createUriAudioPlayer(const char* uri);

    void setPlayerCallback(PlayerCallbackFunc func) {
        mCallback = func;
    }

    int destoryAudioPlayer();

    int start();
    int stop();
    int setVolume(int volume);

    int write(char* buffer, int sizeBytes, bool blocked = false);

private:

    // For streaming player
    int obtainBuffer(char** buffer, bool blocked = false);
    int releaseBuffer(char* buffer, int sizeBytes);
    static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
    void doPlayerCallback(SLAndroidSimpleBufferQueueItf bq);

    // For uri player
    static void uriPlayerCallback(SLPlayItf caller, void *pContext, SLuint32 event);
    void doUriPlayerCallback(SLPlayItf caller, SLuint32 event);

    SLObjectItf mOutputMixObject;
    SLObjectItf mPlayerObject;
    SLPlayItf mPlayerPlay;
    SLVolumeItf mPlayerVolume;

    // For streaming player
    SLAndroidSimpleBufferQueueItf mPlayerBufferQueue;
    int mWriteBufIndex;
    int mReadBufIndex;
    int mDataFull;
    char* mBuffer;
    unsigned mBufferSize;
    pthread_mutex_t mLock;
    pthread_cond_t mCond;

    // For uri player
    PlayerCallbackFunc mCallback;

    PlayerType mType;
};

#endif //UTILS_AUDIOPLAYER_H
