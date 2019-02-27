//
// Created by chehongliang on 19-1-28.
//

#include <assert.h>

#include "AudioPlayer.h"

#define LOG_TAG "AudioPlayer"
#include "LogUtils.h"

#define BUFFER_COUNT (3)

AudioPlayer::AudioPlayer(PlayerType type) :
    mOutputMixObject(NULL),
    mPlayerObject(NULL),
    mPlayerPlay(NULL),
    mPlayerVolume(NULL),
    mPlayerBufferQueue(NULL),
    mWriteBufIndex(0),
    mReadBufIndex(0),
    mDataFull(0),
    mBuffer(NULL),
    mCallback(NULL),
    mType(type)
{
}

AudioPlayer::~AudioPlayer()
{
}

int AudioPlayer::destoryAudioPlayer()
{
  if (mType == STREAMING) {
    pthread_mutex_destroy(&mLock);
    pthread_cond_destroy(&mCond);
  }

  // destroy buffer queue audio player object, and invalidate all associated interfaces
  if (mPlayerObject != NULL) {
    (*mPlayerObject)->Destroy(mPlayerObject);
    mPlayerObject = NULL;
    mPlayerPlay = NULL;
    mPlayerBufferQueue = NULL;
    mPlayerVolume = NULL;
  }

  // destroy output mix object, and invalidate all associated interfaces
  if (mOutputMixObject != NULL) {
    (*mOutputMixObject)->Destroy(mOutputMixObject);
    mOutputMixObject = NULL;
  }

  if (mBuffer) {
    delete [] mBuffer;
  }

  return 0;
}

int AudioPlayer::createStreamingAudioPlayer(unsigned sampleRate, unsigned channels, unsigned bufferSize)
{
  SLresult result;

  // create output mix
  result = (*mEngineEngine)->CreateOutputMix(mEngineEngine, &mOutputMixObject, 0, NULL, NULL);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // realize the output mix
  result = (*mOutputMixObject)->Realize(mOutputMixObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // configure audio sink
  SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, mOutputMixObject};
  SLDataSink audioSnk = {&loc_outmix, NULL};

  SLuint32 ch = SL_SPEAKER_FRONT_CENTER;
  if (channels == 2) {
    ch = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
  }

  SLuint32 sample = SL_SAMPLINGRATE_16;
  if (sampleRate == 48000) {
    sample = SL_SAMPLINGRATE_48;
  }

  // configure audio source
  SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, BUFFER_COUNT};
  SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, (SLuint32)channels, sample,
    SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
    ch, SL_BYTEORDER_LITTLEENDIAN};
  SLDataSource audioSrc = {&loc_bufq, &format_pcm};

  // create audio player
  const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
  const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
  result = (*mEngineEngine)->CreateAudioPlayer(mEngineEngine, &mPlayerObject, &audioSrc, &audioSnk,
      2, ids, req);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // realize the player
  result = (*mPlayerObject)->Realize(mPlayerObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // get the buffer queue interface
  result = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_BUFFERQUEUE,
      &mPlayerBufferQueue);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // register callback on the buffer queue
  result = (*mPlayerBufferQueue)->RegisterCallback(mPlayerBufferQueue, bqPlayerCallback, this);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // get the play interface
  result = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_PLAY, &mPlayerPlay);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // get the volume interface
  result = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_VOLUME, &mPlayerVolume);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  mBuffer = new char[bufferSize * BUFFER_COUNT];
  mBufferSize = bufferSize;

  pthread_mutex_init(&mLock, NULL);
  pthread_cond_init(&mCond, NULL);

  return 0;
}

int AudioPlayer::createUriAudioPlayer(const char* uri)
{
  SLresult result;

  // create output mix
  result = (*mEngineEngine)->CreateOutputMix(mEngineEngine, &mOutputMixObject, 0, NULL, NULL);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // realize the output mix
  result = (*mOutputMixObject)->Realize(mOutputMixObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // configure audio sink
  SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, mOutputMixObject};
  SLDataSink audioSnk = {&loc_outmix, NULL};

  // configure audio source
  // (requires the INTERNET permission depending on the uri parameter)
  SLDataLocator_URI loc_uri = {SL_DATALOCATOR_URI, (SLchar *) uri};
  SLDataFormat_MIME format_mime = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
  SLDataSource audioSrc = {&loc_uri, &format_mime};

  // create audio player
  const SLInterfaceID ids[2] = {SL_IID_SEEK, SL_IID_VOLUME};
  const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
  result = (*mEngineEngine)->CreateAudioPlayer(mEngineEngine, &mPlayerObject, &audioSrc,
      &audioSnk, 2, ids, req);
  // note that an invalid URI is not detected here, but during prepare/prefetch on Android,
  // or possibly during Realize on other platforms
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // realize the player
  result = (*mPlayerObject)->Realize(mPlayerObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // get the play interface
  result = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_PLAY, &mPlayerPlay);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  // get the volume interface
  result = (*mPlayerObject)->GetInterface(mPlayerObject, SL_IID_VOLUME, &mPlayerVolume);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  result = (*mPlayerPlay)->SetCallbackEventsMask(mPlayerPlay, SL_PLAYEVENT_HEADATEND);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;
  result = (*mPlayerPlay)->RegisterCallback(mPlayerPlay, uriPlayerCallback, this);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;

  return 0;
}

/*static*/ void AudioPlayer::uriPlayerCallback(SLPlayItf caller, void *pContext, SLuint32 event)
{
  AudioPlayer* player = (AudioPlayer*) pContext;
  player->doUriPlayerCallback(caller, event);
}

void AudioPlayer::doUriPlayerCallback(SLPlayItf caller, SLuint32 event)
{
  if (mCallback == NULL) {
    return;
  }
  if (SL_PLAYEVENT_HEADATEND & event) {
    mCallback(PLAY_COMPLETE);
  } else {
    ALOGD("ignored event %d", event);
  }
}

int AudioPlayer::start()
{
  // set the player's state to playing
  SLresult result = (*mPlayerPlay)->SetPlayState(mPlayerPlay, SL_PLAYSTATE_PLAYING);
  if (SL_RESULT_SUCCESS != result) {
    return -1;
  }
  return 0;
}

int AudioPlayer::stop()
{
  // set the player's state to playing
  SLresult result = (*mPlayerPlay)->SetPlayState(mPlayerPlay, SL_PLAYSTATE_STOPPED);
  if (SL_RESULT_SUCCESS != result) {
    return -1;
  }
  return 0;
}

// this callback handler is called every time a buffer finishes playing
/*static*/ void AudioPlayer::bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
  AudioPlayer* player = (AudioPlayer*) context;
  player->doPlayerCallback(bq);
}

void AudioPlayer::doPlayerCallback(SLAndroidSimpleBufferQueueItf bq)
{
  assert(bq == mPlayerBufferQueue);

  // ALOGD("player callback");
  pthread_mutex_lock(&mLock);
  if (mDataFull) {
    mDataFull = 0;
    pthread_cond_signal(&mCond);
  }
  mReadBufIndex++;
  mReadBufIndex %= BUFFER_COUNT;
  pthread_mutex_unlock(&mLock);
}

int AudioPlayer::obtainBuffer(char** buffer, bool blocked)
{
  pthread_mutex_lock(&mLock);

  while (mDataFull) {
    if (!blocked) {
      ALOGD("no buffer");
      pthread_mutex_unlock(&mLock);
      return 0;
    }
    // ALOGE("buffer full wait");
    pthread_cond_wait(&mCond, &mLock);
  }

  *buffer = mBuffer + mBufferSize * mWriteBufIndex;
  mWriteBufIndex++;
  mWriteBufIndex %= BUFFER_COUNT;

  if (mWriteBufIndex == mReadBufIndex) {
    mDataFull = 1;
  }

  pthread_mutex_unlock(&mLock);

  return mBufferSize;
}

int AudioPlayer::releaseBuffer(char* buffer, int sizeBytes)
{
  SLresult result = (*mPlayerBufferQueue)->Enqueue(mPlayerBufferQueue, buffer, sizeBytes);
  assert(SL_RESULT_SUCCESS == result);
  (void)result;
  return 0;
}

int AudioPlayer::write(char* buffer, int sizeBytes, bool blocked)
{
  int size = sizeBytes;
  while(size > 0) {
    char* bufTemp;
    int bufSize = obtainBuffer(&bufTemp, blocked);
    if (bufSize <= 0) {
      ALOGD("write() no buffer");
      break;
    }

    int bytesToCopy = size > bufSize ? bufSize : size;

    memcpy(bufTemp, buffer, bytesToCopy);

    // ALOGD("enqueue buffer %p, %d", bufTemp, bytesToCopy);
    releaseBuffer(bufTemp, bytesToCopy);

    size -= bytesToCopy;
    buffer += bytesToCopy;
  }
  return (sizeBytes - size);
}
