// Copyright 2018 Mobvoi Inc. All Rights Reserved.

#include <pthread.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <string>

#include "qualcomm_demo/online_demo.h"
#include "third_party/picojson/picojson.h"

namespace mobvoi {
namespace sds {
namespace {

class Resource {
 public:
  static void SetLanguage(const std::string& lang) {
    lang_ = lang;
  }

  static std::string NoSpeech() {
    if ("zh_cn" == lang_ || "zh_hk" == lang_) {
      return "抱歉，问问没有听清，您能再说一次吗？";
    } else {
      return "Sorry. I'm afraid I didn't catch you. Could you repeat?";
    }
  }

  static std::string NoSupport() {
    if ("zh_cn" == lang_ || "zh_hk" == lang_) {
      return "对不起，小问目前不支持这个功能。";
    } else {
      return "Sorry. I'm afraid I don't support the function.";
    }
  }

  static std::string ErrorParsingMsg() {
    if ("zh_cn" == lang_ || "zh_hk" == lang_) {
      return "对不起，识别结果解析遇到一些问题。请重试。";
    } else {
      return "Sorry. Error happens parsing the result. Please try again.";
    }
  }

  static std::string GetGreetingText() {
    static struct {
      std::string lang;
      std::string greeting;
    } greetings[] = {
      { "zh_cn", "恭候多时！" },
      { "zh_hk", "恭候多时！" },
      { "en_us", "Hello!"     },
    };

    for (const auto& entry : greetings) {
      if (entry.lang == lang_) {
        return entry.greeting;
      }
    }

    return "";
  }

  static std::string GetHotword() {
    static struct {
      std::string lang;
      std::string hotword;
    } hotwords[] = {
      { "zh_cn", "Hi小问" },
      { "zh_hk", "你好大众" },
      { "en_us", "OK Tico"  },
    };

    for (const auto& entry : hotwords) {
      if (entry.lang == lang_) {
        return entry.hotword;
      }
    }

    return "";
  }

  static std::string GetLoadingText() {
    static struct {
      std::string lang;
      std::string prompt;
    } hotwords[] = {
      { "zh_cn", "加载中……"    },
      { "zh_hk", "加载中……"    },
      { "en_us", "Loading ..." },
    };

    for (const auto& entry : hotwords) {
      if (entry.lang == lang_) {
        return entry.prompt;
      }
    }

    return "";
  }

  static std::string GetExitWord() {
    static struct {
      std::string lang;
      std::string word;
    } exit_words[] = {
      { "zh_cn", "退出" },
      { "zh_hk", "退出" },
      { "en_us", "Quit" },
    };

    for (const auto& entry : exit_words) {
      if (entry.lang == lang_) {
        return entry.word;
      }
    }

    return "";
  }

  static std::string GetErrorDesc(int ec) {
    static struct {
      int         error_code;
      const char* lang;
      const char* error_desc;
    } desc_table[] = {
      { MOBVOI_SDS_ERR_NETWORK_ERROR,  "zh_cn", "网络错误"               },
      { MOBVOI_SDS_ERR_NETWORK_ERROR,  "zh_hk", "网络错误"               },
      { MOBVOI_SDS_ERR_NETWORK_ERROR,  "en_us", "Networking error"       },
      { MOBVOI_SDS_ERR_SERVER_ERROR,   "zh_cn", "服务器端错误"           },
      { MOBVOI_SDS_ERR_SERVER_ERROR,   "zh_hk", "服务器端错误"           },
      { MOBVOI_SDS_ERR_SERVER_ERROR,   "en_us", "Internal server error"  },
      { MOBVOI_SDS_ERR_NO_SPEECH,      "zh_cn", "您好像没说话"           },
      { MOBVOI_SDS_ERR_NO_SPEECH,      "zh_hk", "您好像没说话"           },
      { MOBVOI_SDS_ERR_NO_SPEECH,      "en_us", "No speech detected"     },
      { MOBVOI_SDS_ERR_GARBAGE,        "zh_cn", "无效结果"               },
      { MOBVOI_SDS_ERR_GARBAGE,        "zh_hk", "无效结果"               },
      { MOBVOI_SDS_ERR_GARBAGE,        "en_us", "Bad recognition result" },
      { MOBVOI_SDS_ERR_LICENSE_DENIED, "zh_cn", "SDS 服务被拒绝"         },
      { MOBVOI_SDS_ERR_LICENSE_DENIED, "zh_hk", "SDS 服务被拒绝"         },
      { MOBVOI_SDS_ERR_LICENSE_DENIED, "en_us", "SDS service denied"     },
    };

    for (const auto& entry : desc_table) {
      if (entry.error_code == ec && entry.lang == lang_) {
        return entry.error_desc;
      }
    }

    return "";
  }

 private:
  static std::string lang_;
};

std::string Resource::lang_ = "zh_cn";  // NOLINT

class SdsDemo::EventHandler : public CallBackBase {
 public:
  explicit EventHandler(SdsDemo* demo)
      : demo_(demo) {
  }

  void CallBack(const Parameter& param) override {
    using Handler = void (EventHandler::*)(const Parameter& param);
    static struct {
      std::string cb_type;
      Handler     handler;
    } disp_table[] = {
      { MOBVOI_SDS_CB_HOTWORD,            &EventHandler::OnHotword           },
      { MOBVOI_SDS_CB_LOCAL_SILENCE,      &EventHandler::OnLocalSilence      },
      { MOBVOI_SDS_CB_REMOTE_SILENCE,     &EventHandler::OnRemoteSilence     },
      { MOBVOI_SDS_CB_PARTIAL_TRANSCRIPT, &EventHandler::OnPartialTranscript },
      { MOBVOI_SDS_CB_FINAL_TRANSCRIPT,   &EventHandler::OnFinalTranscript   },
      { MOBVOI_SDS_CB_NLU,                &EventHandler::OnNlu               },
      { MOBVOI_SDS_CB_RESULT,             &EventHandler::OnResult            },
      { MOBVOI_SDS_CB_VOLUME,             &EventHandler::OnVolume            },
      { MOBVOI_SDS_CB_ERROR,              &EventHandler::OnError             },
    };

    const std::string& cb_type = param[MOBVOI_SDS_CB_TYPE].AsString();

    for (const auto& entry : disp_table) {
      if (entry.cb_type == cb_type) {
        (this->*entry.handler)(param);
        return;
      }
    }

    std::cerr << "Error: Unknown callback type: " << cb_type << std::endl;
  }

 private:
  void OnHotword(const Parameter& param) {
    std::string result = param[MOBVOI_SDS_CB_INFO].AsString();
    int index = param[MOBVOI_SDS_CB_MULTI_HOTWORD_INDEX].AsInt();
    DblVec frames = param[MOBVOI_SDS_CB_DETECTED_FRAMES].AsDblVec();
    DblVec confs = param[MOBVOI_SDS_CB_DETECTED_CONFIDENCES].AsDblVec();

    std::cout << "Detected hotword: " << result
              << std::bitset<32>(index) << std::endl;

    std::cout << "frames: ";
    for (int i = 0; i < frames.size(); i++) {
      if (frames[i] != 0) {
        std::cout << i << "_" << frames[i] << ", ";
      }
    }

    std::cout << std::endl << "confidences: ";
    for (int i = 0; i < confs.size(); i++) {
      if (confs[i] > 0.00001) {
        std::cout << i << "_" << confs[i] << ", ";
      }
    }
    std::cout << std::endl;

    demo_->SetHotwordDetectedFlag();
    demo_->SelectOneBF(frames);
    std::cout << ">>> " << MOBVOI_SDS_CB_HOTWORD << ": "
              << Resource::GetHotword() << std::endl;
  }

  void OnLocalSilence(const Parameter& param) {
    std::cout << ">>> " << MOBVOI_SDS_CB_LOCAL_SILENCE << std::endl;
  }

  void OnRemoteSilence(const Parameter& param) {
    std::cout << ">>> " << MOBVOI_SDS_CB_REMOTE_SILENCE << std::endl;
  }

  void OnPartialTranscript(const Parameter& param) {
    std::string cb_info = param[MOBVOI_SDS_CB_INFO].AsString();
    std::cout << ">>> " << "MOBVOI_SDS_CB_PARTIAL_TRANSCRIPT: " << cb_info
              << std::endl;
  }

  void OnFinalTranscript(const Parameter& param) {
    std::string cb_info = param[MOBVOI_SDS_CB_INFO].AsString();
    std::cout << ">>> " << "MOBVOI_SDS_CB_FINAL_TRANSCRIPT: " << cb_info
              << std::endl;

    demo_->SetFinalTrans(cb_info);

    std::string bye = Resource::GetExitWord();
    std::transform(cb_info.begin(), cb_info.end(), cb_info.begin(), ::tolower);
    std::transform(bye.begin(), bye.end(), bye.begin(), ::tolower);
    if (cb_info == bye) {
      demo_->SetExitFlag();
    }

    if (demo_->StopOnFinalTranscript()) {
      demo_->SetStoppedFlag();
    }
  }

  void OnNlu(const Parameter& param) {
    std::string cb_info = param[MOBVOI_SDS_CB_INFO].AsString();
    std::cout << ">>> " << "MOBVOI_SDS_CB_NLU: " << cb_info << std::endl;
  }

  void OnResult(const Parameter& param) {
    std::string cb_info = param[MOBVOI_SDS_CB_INFO].AsString();
    std::cout << ">>> " << "MOBVOI_SDS_CB_RESULT: " << cb_info << std::endl;

    demo_->SetResult(cb_info);

    demo_->SetStoppedFlag();
  }

  void OnVolume(const Parameter& param) {
    std::string cb_info = param[MOBVOI_SDS_CB_INFO].AsString();
    std::cout << ">>> " << "MOBVOI_SDS_CB_VOLUME: " << cb_info << std::endl;
  }

  void OnError(const Parameter& param) {
    int error_code = param[MOBVOI_SDS_ERROR_CODE].AsInt();
    std::cout << ">>> " << "MOBVOI_SDS_CB_ERROR: error code: " << error_code
              << ": " << Resource::GetErrorDesc(error_code) << std::endl;
    demo_->SetErrorCode(error_code);
    demo_->SetStoppedFlag();
  }

 private:
  SdsDemo* demo_;

  DISALLOW_COPY_AND_ASSIGN(EventHandler);
};

SdsDemo::SdsDemo()
    : speech_target_(kToNowhere),
      mutex_(PTHREAD_MUTEX_INITIALIZER),
      cond_(PTHREAD_COND_INITIALIZER) {
  sds_ = SpeechSDS::MakeInstance();
  event_handler_ = new EventHandler(this);
  dsp_ = new MobPipeline(SpeechCallback, (void*)this);
  audio_player_ = new AudioPlayer(STREAMING);
  audio_player_->createStreamingAudioPlayer(16000, 1, 16 * 2 * 80);
}

SdsDemo::~SdsDemo() {
  pthread_mutex_destroy(&mutex_);
  pthread_cond_destroy(&cond_);
  SpeechSDS::DestroyInstance(sds_);
  delete event_handler_;
  delete dsp_;
  audio_player_->destoryAudioPlayer();
  delete audio_player_;
}

void SdsDemo::SpeechCallback(void* inst, char* buffer, int length) {
  SdsDemo* demo = (SdsDemo*) inst;
  demo->FeedSpeech(Buf(buffer, length));
}

bool SdsDemo::Run(int argc, char* argv[]) {
  if (!ParseCmdArgs(argc, argv)) {
    return false;
  }

  if (!InitEnv()) {
    std::cerr << "Failed initializing environment" << std::endl;
    return false;
  }

  if (!GetService()) {
    std::cerr << "Failed getting service(s)" << std::endl;
    return false;
  }

  if (!RunMainLoop()) {
    std::cerr << "Failed running main loop" << std::endl;
    return false;
  }

  if (!ReleaseService()) {
    std::cerr << "Failed releasing service(s)" << std::endl;
    return false;
  }

  if (!ReleaseEnv()) {
    std::cerr << "Failed releasing environment" << std::endl;
  }

  return true;
}

bool SdsDemo::ParseCmdArgs(int argc, char* argv[]) {
  if (argc < 2) {
    ShowUsage(argv[0]);
    return false;
  }

  exe_ = argv[0];

  const char* path = argv[1];
  struct stat info;
  if (stat(path, &info) != 0) {
    std::cerr << "Error: Cannot access " << path << std::endl;
    return false;
  } else if (!(info.st_mode & S_IFDIR)) {
    std::cerr << "Error: " << path << " is not a directory" << std::endl;
    return false;
  }
  base_dir_ = path;

  if (argc > 2) {
    std::string type = argv[2];
    if (type == "offline_asr") {
      asr_type_ = MOBVOI_SDS_OFFLINE_ASR;
    } else if (type == "offline_recognizer") {
      asr_type_ = MOBVOI_SDS_OFFLINE_RECOGNIZER;
    } else if (type == "online_onebox") {
      asr_type_ = MOBVOI_SDS_ONLINE_ONEBOX;
    } else if (type == "mixed") {
      asr_type_ = MOBVOI_SDS_MIXED_RECOGNIZER;
    } else if (type == "offline_contact") {
      asr_type_ = MOBVOI_SDS_OFFLINE_CONTACT;
    } else if (type == "mixed_contact_only") {
      asr_type_ = MOBVOI_SDS_MIXED_CONTACT_ONLY;
    } else {
      std::cerr << "Error: Invalid ASR type: " << type << std::endl;
      // ShowUsage(argv[0]);
      // return false;
    }
  }

  if (argc > 3) {
    std::string lang = argv[3];
    if (lang != "zh_cn" && lang != "zh_hk" && lang != "en_us") {
      std::cerr << "Error: " << lang << " is not a valid language" << std::endl;
      ShowUsage(argv[0]);
      return false;
    }
    lang_ = lang;
  } else {
    lang_ = "zh_cn";
  }
  Resource::SetLanguage(lang_);

  if (argc > 4) {
    if (0 == strcasecmp("solo", argv[4])) {
      solo_ = true;
    }
  }

  return true;
}

bool SdsDemo::InitEnv() {
  Parameter params(MOBVOI_SDS_INIT);
  params[MOBVOI_SDS_BASE_DIR] = base_dir_;
  params[MOBVOI_SDS_LANGUAGE] = lang_;

  return sds_->Init(params);
}

bool SdsDemo::GetService() {
  hotword_ = sds_->GetService(MOBVOI_SDS_MULTI_HOTWORD);
  tts_ = sds_->GetService(MOBVOI_SDS_ONLINE_TTS);
  if (!asr_type_.empty()) {
    asr_ = sds_->GetService(asr_type_);
  }

  return hotword_ != nullptr && tts_ != nullptr &&
         (asr_type_.empty() || asr_ != nullptr);
}

bool SdsDemo::RunMainLoop() {
  if (!SetServiceParam() || !BuildServiceModel()) {
    return false;
  }

  dsp_->start();

  if (!StartHotword()) {
    return false;
  }

  while (!exit_app_) {
    ShowPrompt();
    WaitOnHotwordDetectedFlag();
    PlayTtsAudio(Resource::GetGreetingText());

    if (asr_ != nullptr) {
      if (!StartAsr()) {
        return false;
      }

      WaitOnStoppedFlag();

      if (!StopAsr()) {
        return false;
      }

      PlayTtsAudio(GetTtsText());
    }

    ClearResult();
  }

  if (!StopHotword()) {
    return false;
  }

  dsp_->stop();

  return true;
}

bool SdsDemo::ReleaseService() {
  bool ret = true;

  if (asr_ != nullptr && !sds_->ReleaseService(asr_)) {
    std::cout << "Failed releasing service " << asr_type_ << std::endl;
    ret = false;
  } else {
    asr_ = nullptr;
  }

  if (!sds_->ReleaseService(hotword_)) {
    std::cout << "Failed releasing service " << MOBVOI_SDS_HOTWORD << std::endl;
    ret = false;
  } else {
    hotword_ = nullptr;
  }

  if (!sds_->ReleaseService(tts_)) {
    std::cout << "Failed releasing service " << MOBVOI_SDS_MIXED_TTS
              << std::endl;
    ret = false;
  } else {
    tts_ = nullptr;
  }

  return ret;
}

bool SdsDemo::ReleaseEnv() {
  return sds_->CleanUp();
}

bool SdsDemo::SetServiceParam() {
  if (!SetHotwordServiceParam()) {
    return false;
  }

  if (asr_ != nullptr && !SetAsrServiceParam()) {
    return false;
  }

  if (!SetTtsServiceParam()) {
    return false;
  }

  return true;
}

bool SdsDemo::SetHotwordServiceParam() {
  Parameter param(MOBVOI_SDS_SET_PARAM);
  param[MOBVOI_SDS_CALLBACK] = event_handler_;
  param[MOBVOI_SDS_MULTI_HOTWORD_BAN_TIME] = 600;
  param[MOBVOI_SDS_MULTI_HOTWORD_WINDOW_SIZE] = 300;
  param[MOBVOI_SDS_MULTI_HOTWORD_NUM] = solo_ ? 1 : kOutNum;
  param[MOBVOI_SDS_MULTI_HOTWORD_THREAD_NUM] = solo_ ? 1 : 4;
  Parameter result = hotword_->Invoke(param);
  HANDLE_PARAM_ERROR(result, "setting hotword parameter", false);
  return true;
}

bool SdsDemo::SetAsrServiceParam() {
  Parameter params(MOBVOI_SDS_SET_PARAM);

  params[MOBVOI_SDS_CALLBACK] = event_handler_;

  Parameter result = asr_->Invoke(params);
  HANDLE_PARAM_ERROR(result, "setting ASR parameter", false);

  return true;
}

bool SdsDemo::SetTtsServiceParam() {
  Parameter tts_param(MOBVOI_SDS_SET_PARAM);

  if ("zh_cn" == lang_) {
    tts_param[MOBVOI_SDS_LANGUAGE] = "Mandarin";
    tts_param[MOBVOI_SDS_SPEAKER]  = "cissy";
  } else if ("zh_hk" == lang_) {
    tts_param[MOBVOI_SDS_LANGUAGE] = "Cantonese";
    tts_param[MOBVOI_SDS_SPEAKER]  = "dora";
  } else if ("en_us" == lang_) {
    tts_param[MOBVOI_SDS_LANGUAGE] = "English";
    tts_param[MOBVOI_SDS_SPEAKER]  = "angela";
  }

  Parameter result = tts_->Invoke(tts_param);
  HANDLE_PARAM_ERROR(result, "setting TTS parameter", false);

  return true;
}

bool SdsDemo::BuildServiceModel() {
  if (MOBVOI_SDS_ONLINE_ONEBOX == asr_type_ || asr_ == nullptr) {
    return true;
  }

  Parameter params(MOBVOI_SDS_BUILD_MODEL);

  Parameter result = asr_->Invoke(params);
  HANDLE_PARAM_ERROR(result, "building model", false);

  return true;
}

bool SdsDemo::StartHotword() {
  Parameter params(MOBVOI_SDS_START);
  Parameter result = hotword_->Invoke(params);
  HANDLE_PARAM_ERROR(result, "starting hotword detection", false);

  return true;
}

bool SdsDemo::StopHotword() {
  Parameter params(MOBVOI_SDS_STOP);
  Parameter result = hotword_->Invoke(params);
  HANDLE_PARAM_ERROR(result, "stopping hotword detection", false);

  speech_target_ = kToNowhere;

  return true;
}

bool SdsDemo::StartAsr() {
  Parameter start_asr(MOBVOI_SDS_START);
  Parameter result = asr_->Invoke(start_asr);
  HANDLE_PARAM_ERROR(result, "starting ASR", false);

  speech_target_ = kToAsr;

  return true;
}

bool SdsDemo::StopAsr() {
  Parameter stop_asr(MOBVOI_SDS_STOP);
  Parameter result = asr_->Invoke(stop_asr);
  HANDLE_PARAM_ERROR(result, "stopping ASR", false);

  speech_target_ = kToNowhere;

  return true;
}

void SdsDemo::WaitOnHotwordDetectedFlag() {
  WaitOnFlag(&hotword_detected_);
}

void SdsDemo::WaitOnStoppedFlag() {
  WaitOnFlag(&stopped_);
}

void SdsDemo::WaitOnFlag(bool* flag) {
  pthread_mutex_lock(&mutex_);
  *flag = false;
  while (false == *flag) {
    pthread_cond_wait(&cond_, &mutex_);
  }
  pthread_mutex_unlock(&mutex_);
}

bool SdsDemo::PlayTtsAudio(const std::string& text) {
  // Start TTS service
  Parameter start(MOBVOI_SDS_START);
  Parameter result = tts_->Invoke(start);
  HANDLE_PARAM_ERROR(result, "starting TTS", false);

  // Start TTS synthesis
  Parameter feed(MOBVOI_SDS_FEED_TEXT);
  feed[MOBVOI_SDS_TEXT] = text;
  result = tts_->Invoke(feed);
  HANDLE_PARAM_ERROR(result, "feeding text for TTS", false);

  // Play TTS audio
  Parameter read(MOBVOI_SDS_READ);
  char buf[640];
  read[MOBVOI_SDS_AUDIO_BUF] = Buf(buf, sizeof(buf));
  audio_player_->start();
  while (true) {
    result = tts_->Invoke(read);
    HANDLE_PARAM_ERROR(result, "reading TTS data", false);

    int size = result[MOBVOI_SDS_TTS_READ_SIZE].AsInt();
    if (-1 == size) {  // End of TTS audio
      break;
    }

    audio_player_->write(buf, size, true);
  }

  audio_player_->stop();

  // Stop TTS
  Parameter stop(MOBVOI_SDS_STOP);
  tts_->Invoke(stop);

  // Wait until TTS playback completes
  // WaitTtsPlayComplete();

  return true;
}

std::string SdsDemo::GetTtsText() {
  if (error_code_ != MOBVOI_SDS_SUCCESS) {
    return Resource::GetErrorDesc(error_code_);
  } else if (StopOnFinalTranscript()) {
    return final_trans_;
  } else {
    return ParseTtsText(result_);
  }
}

std::string SdsDemo::ParseTtsText(const std::string& result) {
  picojson::value json;
  std::string err = picojson::parse(json, result);
  if (!err.empty() || !json.is<picojson::object>()) {
    return Resource::ErrorParsingMsg();
  }

  // If found "languageOutput|tts", return it
  auto& tts = json.get("languageOutput").get("tts");
  if (!tts.is_null() && !tts.to_str().empty()) {
    return tts.to_str();
  }

  // If "status" is "empty", return no speech
  auto& status = json.get("status");
  if (!status.is_null() && status.to_str() == "empty") {
    return Resource::NoSpeech();
  }

  // If "domain" is "other", return no support
  auto& domain = json.get("domain");
  if (!domain.is_null() && domain.to_str() == "other") {
    return Resource::NoSupport();
  }

  // If "query" is set, return it
  auto& query = json.get("query");
  if (!query.is_null() && !query.to_str().empty()) {
    return query.to_str();
  }

  // Otherwise, no support
  return Resource::NoSupport();
}

void SdsDemo::ClearResult() {
  final_trans_      = "";
  result_           = "";
  error_code_       = 0;
  hotword_detected_ = false;
  stopped_          = false;
}

void SdsDemo::ShowUsage(const std::string& exe) {
  std::cerr << "Usage:\n"
               "\n"
               "    " << exe << " <base dir> <type> [<language>] [solo]\n"
               "\n"
               "Where <type>:\n"
               "\n"
               "    - offline_asr\n"
               "    - offline_recognizer\n"
               "    - online_onebox\n"
               "    - mixed\n"
               "    - offline_contact\n"
               "    - mixed_contact_only\n"
               "\n"
               "And <language>:\n"
               "\n"
               "    - zh_cn (default)\n"
               "    - zh_hk\n"
               "    - en_us\n"
               "\n"
               "Example(s):\n"
               "\n"
               "    " << exe << " ../.. offline_asr zh_cn\n"
               "    " << exe << " ../.. offline_asr zh_hk\n"
               "    " << exe << " ../.. offline_asr en_us\n"
               "    " << exe << " ../.. offline_asr zh_cn solo\n"
               "    " << exe << " ../.. online_onebox zh_cn\n"
               "    " << exe << " ../.. mixed\n";
}

void SdsDemo::ShowPrompt() {
  std::string hotword;
  std::cout << "\n";
  if ("zh_cn" == lang_) {
    std::cout << "<<< 请说 [" << Resource::GetHotword() << "] 进行唤醒\n";
    std::cout << "<<< 唤醒后说 [" << Resource::GetExitWord()
              << "] 来退出程序\n";
  } else if ("zh_hk" == lang_) {
    std::cout << "<<< 请说 [" << Resource::GetHotword() << "] 进行唤醒\n";
    std::cout << "<<< 唤醒后说 [" << Resource::GetExitWord()
              << "] 来退出程序\n";
  } else if ("en_us" == lang_) {
    std::cout << "<<< Please speak [" << Resource::GetHotword()
              << "] to wake up\n";
    std::cout << "<<< Then speak [" << Resource::GetExitWord() << "] to exit\n";
  }
}

bool SdsDemo::FeedSpeech(const Buf& buf) {
  Parameter params(MOBVOI_SDS_FEED_SPEECH);
  if (solo_) {
    Buf soloBuf(buf.GetAddr(), buf.GetSize() / kOutNum);
    params[MOBVOI_SDS_AUDIO_BUF] = soloBuf;  
  } else {
    params[MOBVOI_SDS_AUDIO_BUF] = buf;    
  }

  Parameter result;
  result = hotword_->Invoke(params);
  HANDLE_PARAM_ERROR(result, "feeding speech for hotword detection", false);

  if (speech_target_ == kToAsr && asr_ != nullptr) {
    Buf asrBuf(buf.GetAddr() + doa_index_ * buf.GetSize() / kOutNum,
        buf.GetSize() / kOutNum);
    params[MOBVOI_SDS_AUDIO_BUF] = asrBuf;
    result = asr_->Invoke(params);
    HANDLE_PARAM_ERROR(result, "feeding speech for ASR", false);
  }

  return true;
}

void SdsDemo::SetFinalTrans(const std::string& final_trans) {
  final_trans_ = final_trans;
}

void SdsDemo::SetResult(const std::string &result) {
  result_ = result;
}

void SdsDemo::SetErrorCode(int ec) {
  error_code_ = ec;
}

void SdsDemo::SetHotwordDetectedFlag() {
  pthread_mutex_lock(&mutex_);
  hotword_detected_ = true;
  pthread_cond_signal(&cond_);
  pthread_mutex_unlock(&mutex_);
}

#if kOutNum == 12
static int BFs[] = {0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330};
#elif kOutNum == 6
static int BFs[] = {30, 90, 150, 210, 270, 330};
#elif kOutNum == 8
static int BFs[] = {0, 45, 90, 135, 180, 225, 270, 315};
#endif

void SdsDemo::SelectOneBF(DblVec frames) {
  // int sum = 0;
  // int c = 0;
  // for (int i = 0; i < kOutNum; i++) {
  //   if (index & 1) {
  //     sum += i;
  //     c++;
  //   }

  //   index = index>>1;
  // }

  // if (c) {
  //   hotword_index_ = sum / c;
  // }

  doa_index_ = dsp_->GetHotwordAngle(frames);

  // int angle = dsp_->GetHotwordAngle(frame);

  // int half = (BFs[1] - BFs[0]) / 2;
  // if ((angle >= (360 - half) && angle <= 360) || (angle >= 0 && angle < half)){
  //   doa_index_ = 0;
  // } else {
  //   for (int i = 1; i < sizeof(BFs) / sizeof(BFs[0]); i++) {
  //     if (angle >= (BFs[i] - half) && angle < (BFs[i] + half)) {
  //       doa_index_ = i;
  //       break;
  //     }
  //   }
  // }

  std::cout << "SelectOneBF: doa " << doa_index_ << std::endl;
            // << " hotword " << hotword_index_ << std::endl;
}

void SdsDemo::SetStoppedFlag() {
  pthread_mutex_lock(&mutex_);
  stopped_ = true;
  pthread_cond_signal(&cond_);
  pthread_mutex_unlock(&mutex_);
}

void SdsDemo::SetExitFlag() {
  pthread_mutex_lock(&mutex_);
  exit_app_ = true;
  pthread_mutex_unlock(&mutex_);
}

bool SdsDemo::StopOnFinalTranscript() {
  return MOBVOI_SDS_OFFLINE_ASR     == asr_type_ ||
         MOBVOI_SDS_OFFLINE_CONTACT == asr_type_;
}

}  // namespace
}  // namespace sds
}  // namespace mobvoi

int main(int argc, char* argv[]) {
  mobvoi::sds::SdsDemo demo;
  if (demo.Run(argc, argv)) {
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
