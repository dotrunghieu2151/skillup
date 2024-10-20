#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

#include <Vendors/imguiPlot/imgui_plot.hpp>
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_stdlib.h>

#include "AudioController.hpp"
#include "Core/Event.hpp"
#include "Core/EventEmitter.hpp"
#include "Core/UIComponent.hpp"
#include "Core/UtilComponent.hpp"

namespace Transcribe {
class TranscribeUIComponent : public Core::UIStatefulComponent {
public:
  struct RecordEvent : Core::EventSystem::Event<TranscribeUIComponent> {
    int inputDeviceID;
  };

  struct StopRecordEvent : Core::EventSystem::Event<TranscribeUIComponent> {};

  struct PauseRecordEvent : Core::EventSystem::Event<TranscribeUIComponent> {};

  struct PlaybackEvent : Core::EventSystem::Event<TranscribeUIComponent> {
    int outputDeviceID;
  };

  inline Core::EventSystem::EventEmitter<RecordEvent>& OnRecordEvent() {
    return m_OnRecordEvent;
  }

  inline Core::EventSystem::EventEmitter<PlaybackEvent>& OnPlaybackEvent() {
    return m_OnPlaybackEvent;
  }

  inline Core::EventSystem::EventEmitter<StopRecordEvent>& OnStopRecordEvent() {
    return m_OnStopRecordEvent;
  }

  inline Core::EventSystem::EventEmitter<PauseRecordEvent>&
  OnPauseRecordEvent() {
    return m_OnPauseRecordEvent;
  }

  TranscribeUIComponent(
      bool& visible, float*& samples, int& sampleSize,
      const std::vector<AudioController::DeviceInfo>& inputDevices,
      const std::vector<AudioController::DeviceInfo>& outputDevices,
      bool& isRecording, bool& isPlayingBack)
      : m_Visible{visible}, m_Samples{samples}, m_SampleSize{sampleSize},
        m_IsRecording{isRecording}, m_IsPlayingBack{isPlayingBack},
        m_InputDevices{inputDevices}, m_OutputDevices{outputDevices} {}

  void Render() override {
    if (!ImGui::Begin("Transcribe", &m_Visible,
                      ImGuiWindowFlags_AlwaysAutoResize |
                          ImGuiWindowFlags_MenuBar)) {
      ImGui::End();
      return;
    }

    // list audio device
    if (ImGui::BeginListBox("Input devices")) {
      for (const AudioController::DeviceInfo& device : m_InputDevices) {
        if (ImGui::Selectable(device.name.c_str(),
                              selectedInput == device.deviceID)) {
          selectedInput = device.deviceID;
        }
      }
      ImGui::EndListBox();
    }

    if (ImGui::BeginListBox("Output devices")) {
      for (const AudioController::DeviceInfo& device : m_OutputDevices) {
        if (ImGui::Selectable(device.name.c_str(),
                              selectedOutput == device.deviceID)) {
          selectedOutput = device.deviceID;
        }
      }
      ImGui::EndListBox();
    }

    if (m_IsRecording || m_IsPlayingBack) {
      if (ImGui::Button("Pause")) {
        m_OnPauseRecordEvent.Trigger(PauseRecordEvent{*this});
      }
      if (ImGui::Button("Stop")) {
        m_OnStopRecordEvent.Trigger(StopRecordEvent{*this});
      }
    } else {
      if (selectedInput > -1) {
        if (ImGui::Button("Record")) {
          m_OnRecordEvent.Trigger(RecordEvent{*this, selectedInput});
        }
      }

      if (selectedOutput > -1 && m_SampleSize) {
        if (ImGui::Button("Playback")) {
          m_OnPlaybackEvent.Trigger(PlaybackEvent{*this, selectedOutput});
        }
      }
    }

    if (m_SampleSize) {
      ImGui::PlotConfig conf;
      conf.values.ys = m_Samples;
      conf.values.count = m_SampleSize;
      conf.tooltip.show = true;
      conf.tooltip.format = "x=%.2f, y=%.2f";
      conf.scale.min = -0.1;
      conf.scale.max = 0.1;
      conf.grid_x.show = true;
      conf.grid_y.show = true;
      conf.frame_size = ImVec2(400, 400);
      conf.line_thickness = 2.f;

      ImGui::Plot("plotAudio", conf);
    }

    ImGui::End();
  }

private:
  Core::EventSystem::EventEmitter<RecordEvent> m_OnRecordEvent{};
  Core::EventSystem::EventEmitter<PlaybackEvent> m_OnPlaybackEvent{};
  Core::EventSystem::EventEmitter<StopRecordEvent> m_OnStopRecordEvent{};
  Core::EventSystem::EventEmitter<PauseRecordEvent> m_OnPauseRecordEvent{};
  float*& m_Samples;
  int& m_SampleSize;
  int selectedInput{-1};
  int selectedOutput{-1};
  bool& m_Visible;
  bool& m_IsRecording;
  bool& m_IsPlayingBack;
  const std::vector<AudioController::DeviceInfo>& m_InputDevices;
  const std::vector<AudioController::DeviceInfo>& m_OutputDevices;
};
} // namespace Transcribe