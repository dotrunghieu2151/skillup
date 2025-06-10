#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
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
  bool isTranscribing{false};
  bool autoStartTranscription{true};
  bool recordingWithTranscription{
      true}; // Track if current recording includes transcription
  std::string currentJapaneseTranscription{};
  std::string currentEnglishTranslation{};

  // Loading popup state
  bool showLoadingPopup{false};
  std::string loadingModelPath{};
  std::thread modelLoadingThread;
  std::atomic<bool> modelLoadingComplete{false};
  std::atomic<bool> modelLoadingSuccess{false};

  static constexpr size_t MAX_CONTEXT_SAMPLES =
      48000 * 10; // 10 seconds at 48kHz

public:
  std::shared_ptr<TranscribeUIComponent> transcribeUIComponent;

  TranscribeLayer()
      : controller{AudioController::Config{}},
        inputDevices{controller.GetInputDevices()},
        outputDevices{controller.GetOutputDevices()},
        whisperTranscriber{CreateWhisperTranscriber()},
        transcribeUIComponent{std::make_shared<TranscribeUIComponent>(
            open, recordAudioDataPtr, recordAudioDataSize, inputDevices,
            outputDevices, isRecording, isPlayingBack,
            currentJapaneseTranscription, currentEnglishTranslation,
            isTranscribing, autoStartTranscription)} {}

  ~TranscribeLayer() {
    // Clean up model loading thread
    if (modelLoadingThread.joinable()) {
      modelLoadingThread.join();
    }
  }

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
    transcribeUIComponent->OnStartTranscriptionEvent() +=
        [this](const TranscribeUIComponent::StartTranscriptionEvent& event) {
          if (!isTranscribing && !showLoadingPopup) {
            StartTranscription();
          }
        };
  }

  void OnUpdate(float deltaTime) override {
    // Check if model loading is complete
    if (modelLoadingComplete.load()) {
      showLoadingPopup = false;
      isTranscribing = modelLoadingSuccess.load();
      modelLoadingComplete.store(false); // Reset for next time

      if (modelLoadingThread.joinable()) {
        modelLoadingThread.join();
      }
    }

    if (recordStream) {
      if (recordStream->IsRecording()) {
        isRecording = true;
        const BufferT& recordBuffer = recordStream->Read();
        if (recordBuffer.size) {

          // Only process for transcription if recording with transcription
          // enabled
          int start{};
          if (recordingWithTranscription) {

            // Auto-start transcription if enabled and not already running
            if (autoStartTranscription && !isTranscribing &&
                !showLoadingPopup) {
              StartTranscription();
            }
            start = recordAudioData.size();
            std::vector<float> resampled_samples =
                whisperTranscriber->ResampleAudioSoXR(
                    recordBuffer.buffer, recordBuffer.size, 48000, 16000, 2);
            recordAudioData.insert(recordAudioData.end(),
                                   resampled_samples.begin(),
                                   resampled_samples.end());
            // Process audio for real-time transcription

            ProcessAudioForTranscription(recordBuffer.buffer, recordBuffer.size,
                                         48000);
            recordAudioDataPtr = &recordAudioData[start];
            recordAudioDataSize = static_cast<int>(std::min(
                recordAudioData.size(), (size_t)resampled_samples.size()));
          } else {
            // Store audio data for playback
            start = recordAudioData.size();
            recordAudioData.insert(recordAudioData.end(), recordBuffer.buffer,
                                   recordBuffer.buffer + recordBuffer.size);
            recordAudioDataPtr = &recordAudioData[start];
            recordAudioDataSize = static_cast<int>(recordBuffer.size);
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
    } else {
      if (recordStream) {
        recordStream->Stop();
      }

      if (playbackStream) {
        playbackStream->Stop();
      }
    }

    // Render loading popup
    if (showLoadingPopup) {
      // Center the popup
      ImGuiViewport* viewport = ImGui::GetMainViewport();
      ImVec2 center = viewport->GetCenter();
      ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
      ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Always);

      if (ImGui::BeginPopupModal("Loading Whisper Model", nullptr,
                                 ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoCollapse)) {

        // Loading animation - spinning dots
        static float loadingTime = 0.0f;
        loadingTime += ImGui::GetIO().DeltaTime;

        ImGui::Text("Loading model...");
        ImGui::Text("Model: %s", loadingModelPath.c_str());
        ImGui::Separator();

        // Simple loading animation
        const char* loadingChars = "|/-\\";
        char loadingChar = loadingChars[(int)(loadingTime * 4) % 4];
        ImGui::Text("Please wait %c", loadingChar);

        // Progress bar (indeterminate)
        float progress = fmod(loadingTime * 0.5f, 1.0f);
        ImGui::ProgressBar(progress, ImVec2(-1, 0), "");

        ImGui::EndPopup();
      } else {
        // Open the popup if it's not already open
        ImGui::OpenPopup("Loading Whisper Model");
      }
    }
  }

  void Toggle() { open = !open; }

private:
  std::unique_ptr<WhisperTranscriber> CreateWhisperTranscriber() {
    WhisperTranscriber::Config config;
    config.model_path =
        "Assets/LLM/WhisperModels/ggml-large-v3-turbo-q8_0.bin"; // Large
                                                                 // turbo
                                                                 // model for
                                                                 // better
                                                                 // Japanese
    config.language = "ja";              // Japanese input
    config.translate_to_english = false; // We want original Japanese
    config.use_gpu = true;
    config.n_threads = 4;
    config.vad_threshold = 0.6f; // Lower threshold for Japanese speech patterns
    config.segment_length_ms = 10000; // Slightly longer for Japanese processing
    config.overlap_ms = 2000; // More overlap for better Japanese context
    return std::make_unique<WhisperTranscriber>(config);
  }

public:
  // Whisper transcription methods
  bool StartTranscription() {
    // Show loading popup with model path
    loadingModelPath =
        whisperTranscriber
            ? "Assets/LLM/WhisperModels/ggml-large-v3-turbo-q8_0.bin"
            : "Unknown model";
    showLoadingPopup = true;
    modelLoadingComplete.store(false);
    modelLoadingSuccess.store(false);

    // Start model loading in background thread
    if (modelLoadingThread.joinable()) {
      modelLoadingThread.join(); // Wait for any previous loading to complete
    }

    modelLoadingThread = std::thread([this]() {
      bool success = false;
      if (whisperTranscriber) {
        success = whisperTranscriber->Initialize();

        if (success) {
          // Start real-time transcription
          success = whisperTranscriber->StartRealTimeTranscription(
              [this](const TranscriptionResult& result) {
                // Update current English translation
                if (!result.text.empty()) {
                  if (whisperTranscriber->GetConfig().translate_to_english) {
                    currentEnglishTranslation += "\n" + result.text;
                  } else {
                    currentJapaneseTranscription += "\n" + result.text;
                  }
                }
              });
        }
      }

      modelLoadingSuccess.store(success);
      modelLoadingComplete.store(true);
    });

    return true; // Return immediately, actual result will be checked when
                 // loading completes
  }

  void StopTranscription() {
    if (whisperTranscriber) {
      whisperTranscriber->StopRealTimeTranscription();
    }
    isTranscribing = false;
  }

  void ProcessAudioForTranscription(const float* samples, int sampleCount,
                                    int sampleRate) {
    if (isTranscribing && whisperTranscriber) {
      whisperTranscriber->ProcessAudioBuffer(samples, sampleCount, sampleRate);
    }
  }

  void ToggleAutoTranscription() {
    autoStartTranscription = !autoStartTranscription;
    if (!autoStartTranscription && isTranscribing) {
      StopTranscription();
    }
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
        AudioFileWriter::WriteWAV(fullPath, recordAudioData, 16000, 2);

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
    currentEnglishTranslation.clear();
  }
};
} // namespace Transcribe