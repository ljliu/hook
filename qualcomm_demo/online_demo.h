// Copyright 2019 Mobvoi Inc. All Rights Reserved.
// Author: ljliu@mobvoi.com (Lijie Liu)

#ifndef QUALCOMM_DEMO_ONLINE_DEMO_H_
#define QUALCOMM_DEMO_ONLINE_DEMO_H_

#include <time.h>

#include <string>
#include <vector>

#include "third_party/mobvoisds/include/speech_sds.h"
#include "utils/AudioPlayer.h"
#include "utils/MobPipeline.h"

namespace mobvoi {
namespace sds {
namespace {

#define DISALLOW_COPY_AND_ASSIGN(cls)                             \
  cls(const cls&);                                                \
  void operator=(const cls&)

#define HANDLE_PARAM_ERROR(param, op_str, ret_val)                \
  do {                                                            \
    int __ec__ = (param)[MOBVOI_SDS_ERROR_CODE].AsInt();          \
    if (__ec__ != MOBVOI_SDS_SUCCESS) {                           \
      std::cerr << "Error " << (op_str)                           \
                << ": Error code: " << __ec__                     \
                << " (" << Resource::GetErrorDesc(__ec__) << ")"  \
                << std::endl;                                     \
      return (ret_val);                                           \
    }                                                             \
  } while (false)

class SdsDemo {
 public:
  SdsDemo();
  ~SdsDemo();

  bool Run(int argc, char* argv[]);

  static void SpeechCallback(void* inst, char* buffer, int length);

 private:
  bool ParseCmdArgs(int argc, char* argv[]);
  bool InitEnv();
  bool GetService();
  bool RunMainLoop();
  bool ReleaseService();
  bool ReleaseEnv();

  bool SetServiceParam();
  bool SetHotwordServiceParam();
  bool SetAsrServiceParam();
  bool SetTtsServiceParam();
  bool BuildServiceModel();
  bool StartHotword();
  bool StopHotword();
  bool StartAsr();
  bool StopAsr();
  void WaitOnHotwordDetectedFlag();
  void WaitOnStoppedFlag();
  void WaitOnFlag(bool* flag);
  bool PlayTtsAudio(const std::string& text);
  std::string GetTtsText();
  std::string ParseTtsText(const std::string& result);
  void ClearResult();

  void ShowUsage(const std::string& exe);
  void ShowPrompt();

  bool FeedSpeech(const Buf& buf);
  void SetFinalTrans(const std::string& final_trans);
  void SetResult(const std::string& result);
  void SetErrorCode(int ec);
  void SetHotwordDetectedFlag();
  void SelectOneBF(DblVec frames);
  void SetStoppedFlag();
  void SetExitFlag();
  bool StopOnFinalTranscript();

  enum SpeechTarget {
    kToNowhere,
    kToHotword,
    kToAsr,
  };

 private:
  class EventHandler;

 private:
  std::string     exe_;
  std::string     base_dir_;
  std::string     asr_type_;
  std::string     lang_             = "zh_cn";
  bool            solo_             = false;

  int             doa_index_        = 0;
  int             hotword_index_    = 0;
  MobPipeline*    dsp_              = nullptr;
  SpeechSDS*      sds_              = nullptr;
  Service*        hotword_          = nullptr;
  Service*        asr_              = nullptr;
  Service*        tts_              = nullptr;
  AudioPlayer*    audio_player_     = nullptr;

  EventHandler*   event_handler_    = nullptr;

  SpeechTarget    speech_target_;

  pthread_mutex_t mutex_;
  pthread_cond_t  cond_;
  std::string     final_trans_;
  std::string     result_;
  int             error_code_       = 0;
  bool            hotword_detected_ = false;
  bool            stopped_          = false;
  bool            exit_app_         = false;

  DISALLOW_COPY_AND_ASSIGN(SdsDemo);
};

}
}  // namespace sds
}  // namespace mobvoi

#endif  // QUALCOMM_DEMO_ONLINE_DEMO_H_
