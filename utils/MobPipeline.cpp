//
// Created by chehongliang on 19-1-18.
//

#include "utils/MobPipeline.h"

#include <unistd.h>
#include <iostream>

#include "third_party/mobvoidsp/include/mobvoi_dsp.h"
#include "utils/wav_header.h"

#define LOG_TAG "MobPipeline"
#include "utils/LogUtils.h"
#include "utils/mobvoi_serial.h"

// #define DETECT_PROCESS_TIME

static unsigned int calculate_energy(const short* buffer, int len) {
  unsigned int sum = 0;
  const short* end = buffer + len;
  const short* buff = buffer;
  while (buff < end) {
    sum += (*buff) * (*buff);
    buff++;
  }

  return sum;
}

MobPipeline::MobPipeline(speech_callback callback, void* userdata) :
    mDspInst(NULL),
    mPostDspInst(NULL),
    cb(callback),
    ud(userdata)
{
  ALOGD("MobPipeline constructer");
  //10 ms
  mRecord = new AudioRecord(48000, 2, 10 * 48 * 2 * 2);

  memset(mEnergyBuffer, 0, sizeof(mEnergyBuffer));
}

MobPipeline::~MobPipeline()
{
  ALOGD("MobPipeline destructer");
  if (mRecord != NULL) {
    delete mRecord;
    mRecord = NULL;
  }
}

int MobPipeline::start()
{
  if (mRecord == NULL) {
    return -1;
  }

  mSerialFD = open_serial("/dev/ttyUSB0", 115200, 8, 1, 'N');

  mDspInst = mobvoi_uplink_init(10, 16000, kMicNum, 16000, 0, kOutNum);
  const char* path = "/sdcard/mobvoi/dsp";
  mobvoi_uplink_process_ctl(mDspInst, SET_UPLINK_CONFIG_DIR, (void*)path);

#ifdef ENABLE_POST_AEC
  // mobvoi_uplink_process_ctl(mDspInst, SET_UPLINK_DUMP, 0);

  int post_channel = kOutNum / 2 + 1;
  mPostDspInst =
      mobvoi_uplink_init(10, 16000, post_channel, 16000, 1, post_channel);
  path = "/sdcard/mobvoi/dsp_post";
  mobvoi_uplink_process_ctl(mPostDspInst, SET_UPLINK_CONFIG_DIR, (void*) path);
  mobvoi_uplink_process_ctl(mPostDspInst, RESUME_AEC, (void*) 0);
  mobvoi_uplink_process_ctl(mPostDspInst, RESUME_AEC, (void*) 1);
#endif

  mLooping = true;
  if (pthread_create(&mThread, NULL, run, this) != 0) {
    ALOGE("can not create thread");
    return -1;
  }

  ALOGD("start record %p", mRecord);
  return mRecord->startRecording();
}

int MobPipeline::stop()
{
  if (mRecord == NULL) {
    return -1;
  }

  if (mSerialFD >= 0) {
    close(mSerialFD);
  }

  mLooping = false;
  pthread_join(mThread, NULL);

  if (mDspInst != NULL) {
    mobvoi_uplink_cleanup(mDspInst);
    mDspInst = NULL;
  }

#ifdef ENABLE_POST_AEC
  if (mPostDspInst != NULL) {
    mobvoi_uplink_cleanup(mPostDspInst);
    mPostDspInst = NULL;
  }
#endif

  ALOGD("stop record %p", mRecord);
  return mRecord->stop();
}

int MobPipeline::GetEnergy(int index, int frame) {
  int f = (frame - kEnergyWinLen * 4 / 10) % kEnergyWinLen;
  int sum = 0;
  for (int i = f; i < (f + kEnergyWinLen / 2); i++) {
    sum += (mEnergyBuffer[index][i % kEnergyWinLen] / 160);
  }

  return sum;
}

int MobPipeline::GetHotwordAngle(std::vector<double> frames)
{
  int index = 0;
  int energy = 0;
  for (int i = 0; i < frames.size(); i++) {
    if (frames[i] != 0) {
      int e = GetEnergy(i, frames[i]);
      std::cout << i << " " << e << std::endl;
      if (e > energy) {
        index = i;
        energy = e;
      }
    }
  }

  if (mSerialFD >= 0) {
    send_command(mSerialFD, 2, CMD_SET_LED_ON, index);
  }

  std::cout << "SelectOneBF: max " << index << std::endl;

  mob_doa_result p;
  int frame = frames[index];
  p.offset = mFrameCount > frame ? (mFrameCount - frame) : 0;
  int ret = mobvoi_uplink_process_ctl(mDspInst, GET_DOA_RESULT, &p);
  if (ret != MOB_DSP_ERROR_NONE)
    return INVALID_ANGLE;

  ALOGD("%s: %d", __FUNCTION__, (int)p.angle);

  return p.angle;
}

int MobPipeline::GetMaxNoise(int angle, int frame_idx) {
#if kOutNum == 12
  int ii = -1;
  if (angle >= 0 && angle < 15) {
    ii = 0;
  } else if (angle >= 15 && angle < 45) {
    ii = 1;
  } else if (angle >= 45 && angle < 75) {
    ii = 2;
  } else if (angle >= 75 && angle < 105) {
    ii = 3;
  } else if (angle >= 105 && angle < 135) {
    ii = 4;
  } else if (angle >= 135 && angle < 165) {
    ii = 5;
  } else if (angle >= 165 && angle < 195) {
    ii = 6;
  } else if (angle >= 195 && angle < 225) {
    ii = 7;
  } else if (angle >= 225 && angle < 255) {
    ii = 8;
  } else if (angle >= 255 && angle < 285) {
    ii = 9;
  } else if (angle >= 285 && angle < 315) {
    ii = 10;
  } else if (angle >= 315 && angle < 345) {
    ii = 11;
  } else if (angle >= 345 && angle <= 360) {
    ii = 0;
  }
#elif kOutNum == 8
  if (angle >= 0 && angle < 23) {
    ii = 0;
  } else if (angle >= 23 && angle < 68) {
    ii = 1;
  } else if (angle >= 68 && angle < 113) {
    ii = 2;
  } else if (angle >= 113 && angle < 158) {
    ii = 3;
  } else if (angle >= 158 && angle < 203) {
    ii = 4;
  } else if (angle >= 203 && angle < 248) {
    ii = 5;
  } else if (angle >= 248 && angle < 293) {
    ii = 6;
  } else if (angle >= 293 && angle < 338) {
    ii = 7;
  } else if (angle >= 338 && angle <= 360) {
    ii = 0;
  }
#endif
  // std::cout << "ii: " << ii << ", last: " << mLastMaxNoiseIdx
  //           << ", energy: " << mEnergyBuffer[ii][frame_idx]
  //           << std::endl;
  if (mEnergyBuffer[ii][frame_idx] > 6000000UL &&
      (mLastMaxNoiseIdx == ii || ii == (mLastMaxNoiseIdx + 1) % kOutNum ||
       mLastMaxNoiseIdx == (ii + 1) % kOutNum)) {
    if (mLastMaxNoiseDur < kEnergyWinLen * 2) {
      mLastMaxNoiseDur++;
    }

    if (mLastMaxNoiseDur >= kEnergyWinLen) {
      mNoiseSelected = true;
      return mLastMaxNoiseIdx;
    }
  } else {
    if (mLastMaxNoiseDur > 0) {
      mLastMaxNoiseDur--;
    }

    if (mNoiseSelected && mLastMaxNoiseDur > 0) {
      return mLastMaxNoiseIdx;
    }

    mNoiseSelected = false;
    mLastMaxNoiseIdx = ii;
    mLastMaxNoiseDur = 0;
  }

  return -1;
}

void MobPipeline::PostAEC(short* buffer, int noise_idx) {
  mobvoi_uplink_send_ref_frames(mPostDspInst,
      buffer + 160 * noise_idx, 160, 1, 0);

  int post_channel = kOutNum / 2 + 1;
  for (int i = 0; i < post_channel; i++) {
    mobvoi_uplink_send_mic_frames_per_channel(mPostDspInst,
        buffer + 160 * ((noise_idx + kOutNum / 4 + i) % kOutNum),
        160, i, 0);
  }

  short out[kOutNum / 2 + 1][160] = {0};
  mobvoi_uplink_process(mPostDspInst,
      NULL, 160 * post_channel, post_channel, 0, (short*) out, post_channel);

  for (int i = 0; i < post_channel; i++) {
    memcpy(buffer + 160 * ((noise_idx + kOutNum / 4 + i) % kOutNum),
           out[i],
           160 * sizeof(short));
  }
}

static inline int64_t current_timestamp() {
  struct timeval te;
  gettimeofday(&te, NULL);
  int64_t milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
  return milliseconds;
}

void MobPipeline::doLoop()
{
  //16k * 12channels * 10ms;
  short* cleanBuffer = new short[10 * 16 * kOutNum];

#ifdef MOB_DUMP_AUDIO
  wav_header micWav;
  wav_header_init(&micWav);
  wav_header cleanWav;
  wav_header_init(&cleanWav);

  //wav_header header;
  //wav_header_init(&header);
  mDumpMicFP = fopen("/sdcard/dump/mic_audio.wav", "wb");
  fwrite(&micWav, sizeof(wav_header), 1, mDumpMicFP);
  mDumpCleanFP = fopen("/sdcard/dump/clean_audio.wav", "wb");
  fwrite(&cleanWav, sizeof(wav_header), 1, mDumpCleanFP);

  int dataBytes = 0;
#endif

  while(mLooping) {
    char* buffer;
    int size = mRecord->obtainBuffer(&buffer, true);
    if (size <= 0) {
      continue;
    }

#ifdef MOB_DUMP_AUDIO
    dataBytes += size;
    fwrite(buffer, size, 1, mDumpMicFP);
#endif

#ifdef DETECT_PROCESS_TIME
    uint64_t start = current_timestamp();
#endif

    int processedSamples =
        mobvoi_uplink_process(mDspInst,
                              (const short*)buffer,
                              size >> 1,
                              6, // 6 * 16000 == 2 * 48000
                              0,
                              cleanBuffer,
                              kOutNum);

    int cur_frame = mFrameCount % kEnergyWinLen;
    for (int i = 0; i < kOutNum; i++) {
      mEnergyBuffer[i][cur_frame] =
          calculate_energy(cleanBuffer + 160 * i, 160);
    }

#ifdef ENABLE_POST_AEC
    static int last_noise = -2;
    mob_doa_result res;
    res.offset = 0;
    int ret = mobvoi_uplink_process_ctl(mDspInst, GET_DOA_RESULT, &res);
    if (ret == MOB_DSP_ERROR_NONE) {
      int noise_idx = GetMaxNoise((int)res.angle, cur_frame);
      if (last_noise != noise_idx) {
        last_noise = noise_idx;
        std::cout << "Noise channel: " << noise_idx
                  << ", energy: " << mEnergyBuffer[noise_idx][cur_frame]
                  << ", doa: " << (int)res.angle
                  << std::endl;
      }
      if (noise_idx >= 0 && noise_idx < kOutNum) {
        PostAEC(cleanBuffer, noise_idx);
      }
    }
#endif

#ifdef DETECT_PROCESS_TIME
    uint64_t end1 = current_timestamp();
#endif

    cb(ud, (char*)cleanBuffer, 160 * 2 * kOutNum);

#ifdef DETECT_PROCESS_TIME
    uint64_t end2 = current_timestamp();

    if (end2 - start > 10) {
      std::cout << "Process too long: " << (end2 - start)
                << ", " << (end1 - start) << std::endl;
    }
#endif

    mFrameCount++;

#ifdef MOB_DUMP_AUDIO
    int samplesPerChannel = processedSamples / kOutNum;
    for (int i = 0; i < samplesPerChannel; i++) {
      for (int j = 0; j < kOutNum; j++) {
        fwrite(cleanBuffer + i + samplesPerChannel * j , 2, 1, mDumpCleanFP);
      }
    }
#endif

    mRecord->releaseBuffer(buffer);
  }

  delete [] cleanBuffer;

#ifdef MOB_DUMP_AUDIO
  printf("dataBytes %d\n", dataBytes);
  int dataBytesPerChannel = dataBytes / kMicNum;
  micWav.num_channels = kMicNum;
  micWav.sample_rate = 16000;
  micWav.byte_rate = 16000 * kMicNum * 2;
  micWav.bit_depth = 16;
  micWav.sample_alignment = micWav.num_channels * micWav.bit_depth / (short)8;
  micWav.wav_size =
      (dataBytesPerChannel * micWav.num_channels + sizeof(wav_header) - 8);
  micWav.data_bytes = dataBytesPerChannel * micWav.num_channels;
  rewind(mDumpMicFP);
  fwrite(&micWav, sizeof(wav_header), 1, mDumpMicFP);

  cleanWav.num_channels = kOutNum;
  cleanWav.sample_rate = 16000;
  cleanWav.byte_rate = 16000 * kOutNum * 2;
  cleanWav.bit_depth = 16;
  cleanWav.sample_alignment =
      cleanWav.num_channels * cleanWav.bit_depth / (short)8;
  cleanWav.wav_size =
      (dataBytesPerChannel * cleanWav.num_channels + sizeof(wav_header) - 8);
  cleanWav.data_bytes = dataBytesPerChannel * cleanWav.num_channels;
  rewind(mDumpCleanFP);
  fwrite(&cleanWav, sizeof(wav_header), 1, mDumpCleanFP);
  if (mDumpMicFP) {
    fclose(mDumpMicFP);
  }
  if (mDumpCleanFP) {
    fclose(mDumpCleanFP);
  }
#endif
}

/*static*/ void* MobPipeline::run(void *arg)
{
  // pthread_setschedprio(pthread_self(), sched_get_priority_max(SCHED_RR));

  MobPipeline* pipeline = (MobPipeline*)arg;
  pipeline->doLoop();
  return NULL;
}
