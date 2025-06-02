#pragma once

#include <cstring>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "AudioController.hpp"
#include "AudioFileWriter.hpp"
#include "Components/TranscribeUIComponent.hpp"
#include "Core/Application.hpp"
#include "WhisperTranscriber.hpp"

namespace Transcribe {
class TranscribeLayer : public Core::ApplicationLayer {
private:
  using StreamConfig = AudioController::StreamSampleFloat32Config;
  using RecordStreamT = RecordStream<StreamConfig>;
  using PlaybackStreamT = PlaybackStream<StreamConfig>;
  using BufferT = RecordStreamT::Buffer;

  bool isRecording{false};
  bool isPlayingBack{false};
  bool open{false};
  int recordAudioDataSize{};
  StreamConfig::Sample* recordAudioDataPtr{nullptr};
  AudioController controller;
  StreamConfig::Sample maxFrequency{};
  std::vector<StreamConfig::Sample> recordAudioData{};
  std::vector<AudioController::DeviceInfo> inputDevices{};
  std::vector<AudioController::DeviceInfo> outputDevices{};
  std::unique_ptr<RecordStreamT> recordStream{nullptr};
  std::unique_ptr<PlaybackStreamT> playbackStream{nullptr};

  // Whisper transcription
  std::unique_ptr<WhisperTranscriber> whisperTranscriber{nullptr};
  std::unique_ptr<WhisperTranscriber> translationTranscriber{nullptr};
  std::vector<TranscriptionResult> transcriptionResults{};
  std::vector<TranscriptionResult> translationResults{};
  bool isTranscribing{false};
  bool autoStartTranscription{true};
  bool recordingWithTranscription{
      true}; // Track if current recording includes transcription
  std::string currentJapaneseTranscription{};
  std::string currentEnglishTranslation{};

  // Audio context for better transcription
  std::vector<float> audioContext{};
  std::vector<float> translationAudioBuffer{};
  static constexpr size_t MAX_CONTEXT_SAMPLES =
      48000 * 10; // 10 seconds at 48kHz

public:
  std::shared_ptr<TranscribeUIComponent> transcribeUIComponent;

  TranscribeLayer()
      : controller{AudioController::Config{}},
        inputDevices{controller.GetInputDevices()},
        outputDevices{controller.GetOutputDevices()},
        whisperTranscriber{CreateWhisperTranscriber()},
        translationTranscriber{CreateTranslationTranscriber()},
        transcribeUIComponent{std::make_shared<TranscribeUIComponent>(
            open, recordAudioDataPtr, recordAudioDataSize, inputDevices,
            outputDevices, isRecording, isPlayingBack,
            currentJapaneseTranscription, currentEnglishTranslation,
            isTranscribing, autoStartTranscription)} {}

  void OnAttach() override {
    transcribeUIComponent->OnRecordEvent() +=
        [this](const TranscribeUIComponent::RecordEvent& event) {
          ClearRecording();
          recordStream = controller.Record<StreamConfig>(event.inputDeviceID);
          recordingWithTranscription = event.enableTranscription;
        };
    transcribeUIComponent->OnPlaybackEvent() +=
        [this](const TranscribeUIComponent::PlaybackEvent& event) {
          playbackStream = controller.Playback<StreamConfig>(
              event.outputDeviceID, recordAudioData);
        };
    transcribeUIComponent->OnPauseRecordEvent() +=
        [this](const TranscribeUIComponent::PauseRecordEvent& event) {
          if (recordStream) {
            recordStream->Pause();
          }

          if (playbackStream) {
            playbackStream->Pause();
          }
        };
    transcribeUIComponent->OnStopRecordEvent() +=
        [this](const TranscribeUIComponent::StopRecordEvent& event) {
          if (recordStream) {
            recordStream->Stop();
          }

          if (playbackStream) {
            playbackStream->Stop();
          }
        };
    transcribeUIComponent->OnSaveAudioEvent() +=
        [this](const TranscribeUIComponent::SaveAudioEvent& event) {
          SaveAudioToFile(event.filename);
        };
  }

  void OnUpdate(float deltaTime) override {
    if (recordStream) {
      if (recordStream->IsRecording()) {
        isRecording = true;
        const BufferT& recordBuffer = recordStream->Read();
        if (recordBuffer.size) {
          // Store audio data for playback
          recordAudioData.reserve(recordAudioData.capacity() +
                                  recordBuffer.size);
          int start = recordAudioData.size();
          for (int i{}; i < recordBuffer.size; ++i) {
            recordAudioData.push_back(recordBuffer.buffer[i]);
          }
          recordAudioDataPtr = &recordAudioData[start];
          recordAudioDataSize = static_cast<int>(recordBuffer.size);

          // Only process for transcription if recording with transcription
          // enabled
          if (recordingWithTranscription) {
            // Add to audio context for transcription
            AddToAudioContext(recordBuffer.buffer, recordBuffer.size);

            // Auto-start transcription if enabled and not already running
            if (autoStartTranscription && !isTranscribing) {
              StartTranscription();
            }

            // Process audio for real-time transcription
            ProcessAudioForTranscription(recordBuffer.buffer,
                                         static_cast<int>(recordBuffer.size));
          }
        }
      } else if (recordStream->IsStopped()) {
        recordStream = nullptr;
        if (isTranscribing) {
          StopTranscription();
        }
      }
    } else {
      isRecording = false;
    }

    if (playbackStream) {
      if (playbackStream->IsPlaying()) {
        isPlayingBack = true;
      } else if (playbackStream->IsStopped()) {
        isPlayingBack = false;
      }
    }
  }

  void OnUIRender() override {
    if (open) {
      transcribeUIComponent->Render();
    }
  }

  void Toggle() { open = !open; }

private:
  std::unique_ptr<WhisperTranscriber> CreateWhisperTranscriber() {
    WhisperTranscriber::Config config;
    config.model_path =
        "models/ggml-large-v3-turbo-q8_0.bin"; // Large turbo model for better
                                               // Japanese
    config.language = "ja";                    // Japanese input
    config.translate_to_english = false;       // We want original Japanese
    config.use_gpu = true;
    config.n_threads = 4;
    config.vad_threshold = 0.5f; // Lower threshold for Japanese speech patterns
    config.segment_length_ms = 4000; // Slightly longer for Japanese processing
    config.overlap_ms = 750;         // More overlap for better Japanese context

    return std::make_unique<WhisperTranscriber>(config);
  }

  std::unique_ptr<WhisperTranscriber> CreateTranslationTranscriber() {
    WhisperTranscriber::Config config;
    config.model_path =
        "models/ggml-large-v3-turbo-q8_0.bin"; // Large turbo model for better
                                               // Japanese
    config.language = "ja";                    // Japanese input
    config.translate_to_english = true;        // Translate Japanese to English
    config.use_gpu = true;
    config.n_threads = 2;        // Use fewer threads for translation
    config.vad_threshold = 0.5f; // Lower threshold for Japanese
    config.segment_length_ms =
        4000;                // Longer segments for better Japanese processing
    config.overlap_ms = 750; // More overlap for Japanese context

    return std::make_unique<WhisperTranscriber>(config);
  }

  void AddToAudioContext(const float* samples, int sampleCount) {
    // Add new samples to context buffer
    for (int i = 0; i < sampleCount; ++i) {
      audioContext.push_back(samples[i]);
    }

    // Keep only the last MAX_CONTEXT_SAMPLES
    if (audioContext.size() > MAX_CONTEXT_SAMPLES) {
      audioContext.erase(audioContext.begin(),
                         audioContext.begin() +
                             (audioContext.size() - MAX_CONTEXT_SAMPLES));
    }
  }

public:
  // Whisper transcription methods
  bool StartTranscription() {
    if (!whisperTranscriber->Initialize()) {
      return false;
    }

    if (!translationTranscriber->Initialize()) {
      return false;
    }

    // Start main transcription
    bool transcriptionStarted = whisperTranscriber->StartRealTimeTranscription(
        [this](const TranscriptionResult& result) {
          transcriptionResults.push_back(result);

          // Update current Japanese transcription
          if (!result.text.empty()) {
            currentJapaneseTranscription = result.text;

            // Process same audio for translation
            if (!translationAudioBuffer.empty()) {
              ProcessAudioForTranslation(translationAudioBuffer.data(),
                                         translationAudioBuffer.size());
            }
          }

          // Keep only last 50 results to prevent memory growth
          if (transcriptionResults.size() > 50) {
            transcriptionResults.erase(transcriptionResults.begin());
          }
        });

    // Start translation transcription
    bool translationStarted =
        translationTranscriber->StartRealTimeTranscription(
            [this](const TranscriptionResult& result) {
              translationResults.push_back(result);

              // Update current English translation
              if (!result.text.empty()) {
                currentEnglishTranslation = result.text;
              }

              // Keep only last 50 results to prevent memory growth
              if (translationResults.size() > 50) {
                translationResults.erase(translationResults.begin());
              }
            });

    isTranscribing = transcriptionStarted && translationStarted;
    return isTranscribing;
  }

  void StopTranscription() {
    if (whisperTranscriber) {
      whisperTranscriber->StopRealTimeTranscription();
    }
    if (translationTranscriber) {
      translationTranscriber->StopRealTimeTranscription();
    }
    isTranscribing = false;
  }

  void ProcessAudioForTranscription(const float* samples, int sampleCount) {
    if (isTranscribing && whisperTranscriber) {
      whisperTranscriber->ProcessAudioBuffer(samples, sampleCount, 48000);

      // Store audio for translation processing
      translationAudioBuffer.assign(samples, samples + sampleCount);
    }
  }

  void ProcessAudioForTranslation(const float* samples, int sampleCount) {
    if (isTranscribing && translationTranscriber) {
      translationTranscriber->ProcessAudioBuffer(samples, sampleCount, 48000);
    }
  }

  void ToggleAutoTranscription() {
    autoStartTranscription = !autoStartTranscription;
    if (!autoStartTranscription && isTranscribing) {
      StopTranscription();
    }
  }

  const std::vector<TranscriptionResult>& GetTranscriptionResults() const {
    return transcriptionResults;
  }

  // Audio file saving functionality
  bool SaveAudioToFile(const std::string& filename) {
    if (recordAudioData.empty()) {
      // No recorded audio to save
      return false;
    }

    std::string saveFilename = filename;
    if (saveFilename.empty()) {
      // Generate timestamped filename with mode prefix
      std::string prefix = recordingWithTranscription ? "transcribed_recording"
                                                      : "audio_recording";
      saveFilename = AudioFileWriter::GenerateTimestampedFilename(prefix);
    }

    // Create recordings directory if it doesn't exist
    std::string recordingsDir = "recordings/";
    std::filesystem::create_directories(recordingsDir);
    std::string fullPath = recordingsDir + saveFilename;

    // Save as 16-bit PCM WAV for better compatibility
    bool success =
        AudioFileWriter::WriteWAV(fullPath, recordAudioData, 48000, 2);

    if (success) {
      // Log success with recording mode info
      printf("Audio saved to: %s\n", fullPath.c_str());
      printf("Duration: %.2f seconds\n",
             (float)recordAudioData.size() / 48000.0f / 2.0f);
      printf("Recording mode: %s\n",
             recordingWithTranscription ? "With Transcription" : "Audio Only");
    } else {
      printf("Failed to save audio to: %s\n", fullPath.c_str());
    }

    return success;
  }

  // Get recording info for UI display
  bool HasRecordedAudio() const { return !recordAudioData.empty(); }

  float GetRecordingDuration() const {
    if (recordAudioData.empty())
      return 0.0f;
    return (float)recordAudioData.size() / 48000.0f / 2.0f; // 48kHz stereo
  }

  size_t GetRecordedSampleCount() const { return recordAudioData.size(); }

  bool IsRecordingWithTranscription() const {
    return recordingWithTranscription;
  }

  // Clear recording data (useful for starting fresh)
  void ClearRecording() {
    recordAudioData.clear();
    audioContext.clear();
    translationAudioBuffer.clear();
    currentJapaneseTranscription.clear();
    currentEnglishTranslation.clear();
    transcriptionResults.clear();
    translationResults.clear();
  }
};
} // namespace Transcribe