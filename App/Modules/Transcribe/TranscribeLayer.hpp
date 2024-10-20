#pragma once

#include <cstring>
#include <functional>
#include <imgui.h>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "AudioController.hpp"
#include "Components/TranscribeUIComponent.hpp"
#include "Core/Application.hpp"

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

public:
  std::shared_ptr<TranscribeUIComponent> transcribeUIComponent;

  TranscribeLayer()
      : controller{AudioController::Config{}},
        inputDevices{controller.GetInputDevices()},
        outputDevices{controller.GetOutputDevices()},
        transcribeUIComponent{std::make_shared<TranscribeUIComponent>(
            open, recordAudioDataPtr, recordAudioDataSize, inputDevices,
            outputDevices, isRecording, isPlayingBack)} {}

  void OnAttach() override {
    transcribeUIComponent->OnRecordEvent() +=
        [this](const TranscribeUIComponent::RecordEvent& event) {
          recordStream = controller.Record<StreamConfig>(event.inputDeviceID);
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
  }

  void OnUpdate(float deltaTime) override {
    if (recordStream) {
      if (recordStream->IsRecording()) {
        isRecording = true;
        const BufferT& recordBuffer = recordStream->Read();
        if (recordBuffer.size) {

          recordAudioData.reserve(recordAudioData.capacity() +
                                  recordBuffer.size);
          int start = recordAudioData.size();
          for (int i{}; i < recordBuffer.size; ++i) {
            recordAudioData.push_back(recordBuffer.buffer[i]);
          }
          recordAudioDataPtr = &recordAudioData[start];
          recordAudioDataSize = recordBuffer.size;
          // std::memcpy(recordAudioData.data() + recordAudioData.size(),
          //             recordBuffer.buffer,
          //             sizeof(StreamConfig::Sample) * recordBuffer.size);
        }
      } else if (recordStream->IsStopped()) {
        recordStream = nullptr;
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
};
} // namespace Transcribe