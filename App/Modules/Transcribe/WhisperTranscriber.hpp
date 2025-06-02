#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <whisper.h>

#include "AudioController.hpp"
#include "Core/Job/Job.hpp"

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
    int n_threads = 4;
    bool use_gpu = true;
    float vad_threshold = 0.6f;
    int segment_length_ms = 5000; // 5 seconds
    int overlap_ms = 500;         // 500ms overlap
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

  // Process audio buffer (for integration with existing audio system)
  void ProcessAudioBuffer(const float* samples, int sample_count,
                          int sample_rate);

  // Batch transcription
  TranscriptionResult TranscribeFile(const std::string& audio_file_path);
  TranscriptionResult TranscribeBuffer(const float* samples, int sample_count,
                                       int sample_rate);

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
  std::atomic<bool> m_ShouldStop{false};

  TranscriptionCallback m_Callback;
  std::thread m_ProcessingThread;

  // Audio buffer management
  std::vector<float> m_AudioBuffer;
  std::mutex m_BufferMutex;
  std::atomic<bool> m_HasNewAudio{false};

  // Internal methods
  bool LoadModel();
  void SetupParams();
  void ProcessingLoop();
  TranscriptionResult ProcessSegment(const AudioSegment& segment);
  std::vector<float> ResampleAudio(const float* samples, int sample_count,
                                   int input_rate, int output_rate);
  bool IsValidAudioSegment(const std::vector<float>& samples);

  // VAD (Voice Activity Detection) helpers
  bool HasSpeech(const float* samples, int sample_count);
  float CalculateRMS(const float* samples, int sample_count);
};

// Job for async transcription
class TranscriptionJob : public Core::Job {
public:
  TranscriptionJob(WhisperTranscriber& transcriber,
                   const std::vector<float>& audio_data, int sample_rate,
                   WhisperTranscriber::TranscriptionCallback callback);

  void Execute() override;

private:
  WhisperTranscriber& m_Transcriber;
  std::vector<float> m_AudioData;
  int m_SampleRate;
  WhisperTranscriber::TranscriptionCallback m_Callback;
};

} // namespace Transcribe