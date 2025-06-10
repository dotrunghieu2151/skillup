#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <soxr.h>
#include <whisper.h>

#include "AudioController.hpp"
#include "Core/Job/Job.hpp"
#include "Core/Stream/Stream.hpp"

namespace Transcribe {

struct TranscriptionResult {
  std::string text;
  std::string language;
  float confidence;
  int64_t start_time_ms;
  int64_t end_time_ms;
  bool is_translation;
};

class WhisperTranscriber {
public:
  struct Config {
    std::string model_path = "models/ggml-base.en.bin";
    std::string language = "auto"; // "auto" for auto-detection
    bool translate_to_english = false;
    int n_threads = std::min(4, (int32_t)std::thread::hardware_concurrency());
    bool use_gpu = true;
    float vad_threshold = 0.6f;
    int segment_length_ms = 5000; // 5 seconds
    int overlap_ms = 500;         // 500ms overlap
    int input_rate = 48000;
  };

  using TranscriptionCallback = std::function<void(const TranscriptionResult&)>;

public:
  WhisperTranscriber(const Config& config = Config{});
  ~WhisperTranscriber();

  // Non-copyable
  WhisperTranscriber(const WhisperTranscriber&) = delete;
  WhisperTranscriber& operator=(const WhisperTranscriber&) = delete;

  bool Initialize();
  void Shutdown();

  // Real-time transcription
  bool StartRealTimeTranscription(TranscriptionCallback callback);
  void StopRealTimeTranscription();
  bool IsTranscribing() const { return m_IsTranscribing.load(); }

  void ProcessAudioBuffer(const float* samples, int sample_count,
                          int sample_rate);

  // Batch transcription
  TranscriptionResult TranscribeFile(const std::string& audio_file_path);
  // Get configuration
  const Config& GetConfig() const { return m_Config; }

  // Configuration
  void SetLanguage(const std::string& language) {
    m_Config.language = language;
  }
  void SetTranslateToEnglish(bool translate) {
    m_Config.translate_to_english = translate;
  }
  void SetVADThreshold(float threshold) { m_Config.vad_threshold = threshold; }

  // Model info
  bool IsModelLoaded() const { return m_Context != nullptr; }
  std::string GetModelInfo() const;
  std::vector<std::string> GetSupportedLanguages() const;

  // High-quality resampling using SoX Resampler
  std::vector<float> ResampleAudioSoXR(const float* samples, int sample_count,
                                       int input_rate, int output_rate,
                                       int channels);

private:
  struct AudioSegment {
    std::vector<float> samples;
    int sample_rate;
    int64_t timestamp_ms;
  };

  Config m_Config;
  whisper_context* m_Context;
  whisper_full_params m_Params;

  std::atomic<bool> m_IsTranscribing{false};

  TranscriptionCallback m_Callback;
  std::jthread audioProcessingThread;
  std::jthread transcriptionThread;

  // Audio buffer management
  std::vector<float> m_AudioDataBufferOld;
  std::vector<whisper_token> m_PromptTokens;
  Core::Stream<float> m_AudioStream;

  // Internal methods
  bool LoadModel();
  void SetupParams();
  void TranscriptionLoop(std::stop_token s);
  TranscriptionResult ProcessSegment(const AudioSegment& segment);

  bool IsValidAudioSegment(const std::vector<float>& samples);

  // VAD (Voice Activity Detection) helpers
  bool HasSpeech(const float* samples, int sample_count);
  float CalculateRMS(const float* samples, int sample_count);
  void ConvertStereoToMono(const float* samples, int sample_count,
                           std::vector<float>& output);
};

} // namespace Transcribe