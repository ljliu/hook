//
// Created by chehongliang on 19-1-18.
//

#ifndef UTILS_MOBPIPELINE_H
#define UTILS_MOBPIPELINE_H

#include <vector>

#include <stdio.h>
#include <pthread.h>

#include "utils/AudioRecord.h"

// #define kMicNum (4)
#define kMicNum (6)
#define kOutNum (kMicNum * 2)

#define kEnergyWinLen 200

#define ENABLE_POST_AEC

// #define MOB_DUMP_AUDIO

typedef void (*speech_callback)(void* ud, char* buffer, int length);

class MobPipeline {
public:
    MobPipeline(speech_callback callback, void* ud);
    ~MobPipeline();

    int start();
    int stop();
    int GetEnergy(int index, int frame);
    int GetHotwordAngle(std::vector<double> frames);

    int GetMaxNoise(int angle, int frame_idx);
    void PostAEC(short* buffer, int noise_idx);

private:
    static void* run(void* arg);
    void doLoop();

    AudioRecord* mRecord = nullptr;

    void* mDspInst = nullptr;
    void* mPostDspInst = nullptr;

    pthread_t mThread;
    bool mLooping = false;

    speech_callback cb = nullptr;
    void* ud = nullptr;
    int mSerialFD = -1;

    unsigned int mFrameCount = 0;
    unsigned long mEnergyBuffer[kOutNum][kEnergyWinLen] = {0};
    int mLastMaxNoiseIdx = -1;
    int mLastMaxNoiseDur = 0;
    bool mNoiseSelected = false;

#ifdef MOB_DUMP_AUDIO
    FILE* mDumpMicFP = nullptr;
    FILE* mDumpCleanFP = nullptr;
#endif //MOB_DUMP_AUDIO
};

#endif // UTILS_MOBPIPELINE_H
