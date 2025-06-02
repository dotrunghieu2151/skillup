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
    bool enableTranscription;
  };

  struct StopRecordEvent : Core::EventSystem::Event<TranscribeUIComponent> {};

  struct PauseRecordEvent : Core::EventSystem::Event<TranscribeUIComponent> {};

  struct PlaybackEvent : Core::EventSystem::Event<TranscribeUIComponent> {
    int outputDeviceID;
  };

  struct SaveAudioEvent : Core::EventSystem::Event<TranscribeUIComponent> {
    std::string filename;
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

  inline Core::EventSystem::EventEmitter<SaveAudioEvent>& OnSaveAudioEvent() {
    return m_OnSaveAudioEvent;
  }

  TranscribeUIComponent(
      bool& visible, float*& samples, int& sampleSize,
      const std::vector<AudioController::DeviceInfo>& inputDevices,
      const std::vector<AudioController::DeviceInfo>& outputDevices,
      bool& isRecording, bool& isPlayingBack,
      std::string& currentJapaneseTranscription,
      std::string& currentEnglishTranslation, bool& isTranscribing,
      bool& autoStartTranscription)
      : m_Visible{visible}, m_Samples{samples}, m_SampleSize{sampleSize},
        m_IsRecording{isRecording}, m_IsPlayingBack{isPlayingBack},
        m_InputDevices{inputDevices}, m_OutputDevices{outputDevices},
        m_CurrentTranscription{currentJapaneseTranscription},
        m_CurrentTranslation{currentEnglishTranslation},
        m_IsTranscribing{isTranscribing},
        m_AutoStartTranscription{autoStartTranscription} {}

  void Render() override {
    if (!ImGui::Begin("Real-time Transcription & Translation", &m_Visible,
                      ImGuiWindowFlags_AlwaysAutoResize |
                          ImGuiWindowFlags_MenuBar)) {
      ImGui::End();
      return;
    }

    // Recording Mode Settings Section
    if (ImGui::CollapsingHeader("Recording Mode",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::RadioButton("Record with Transcription", &m_RecordingMode, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Record Audio Only", &m_RecordingMode, 1);

      if (m_RecordingMode == 0) {
        ImGui::Indent();
        ImGui::Checkbox("Auto-start transcription with recording",
                        &m_AutoStartTranscription);
        ImGui::Unindent();
      }

      // Status display
      if (m_IsRecording) {
        if (m_RecordingMode == 0 && m_IsTranscribing) {
          ImGui::Text("Status: Recording & Transcribing");
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● ●");
        } else if (m_RecordingMode == 0) {
          ImGui::Text("Status: Recording (Transcription stopped)");
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "● ○");
        } else {
          ImGui::Text("Status: Recording Audio Only");
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "●");
        }
      } else {
        ImGui::Text("Status: Stopped");
      }
    }

    // Audio Devices Section
    if (ImGui::CollapsingHeader("Audio Devices")) {
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
    }

    // Recording Controls
    if (ImGui::CollapsingHeader("Recording Controls",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      if (m_IsRecording || m_IsPlayingBack) {
        if (ImGui::Button("Pause")) {
          m_OnPauseRecordEvent.Trigger(PauseRecordEvent{*this});
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
          m_OnStopRecordEvent.Trigger(StopRecordEvent{*this});
        }
      } else {
        if (selectedInput > -1) {
          // Different button text based on recording mode
          if (m_RecordingMode == 0) {
            if (ImGui::Button("🎤 Start Recording & Transcription")) {
              m_OnRecordEvent.Trigger(RecordEvent{*this, selectedInput, true});
            }
          } else {
            if (ImGui::Button("🎤 Start Audio Recording")) {
              m_OnRecordEvent.Trigger(RecordEvent{*this, selectedInput, false});
            }
          }
        } else {
          ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                             "Please select an input device");
        }

        if (selectedOutput > -1 && m_SampleSize) {
          ImGui::SameLine();
          if (ImGui::Button("🔊 Playback")) {
            m_OnPlaybackEvent.Trigger(PlaybackEvent{*this, selectedOutput});
          }
        }
      }

      // Save Audio Controls - only show when there's recorded audio
      if (m_SampleSize > 0 && !m_IsRecording) {
        ImGui::Separator();
        ImGui::Text("Save Recording:");

        // Filename input
        static char filename[256] = "recording";
        ImGui::InputText("Filename", filename, sizeof(filename));

        ImGui::SameLine();
        if (ImGui::Button("💾 Save Audio File")) {
          std::string fullFilename = std::string(filename) + ".wav";
          m_OnSaveAudioEvent.Trigger(SaveAudioEvent{*this, fullFilename});
        }

        ImGui::SameLine();
        if (ImGui::Button("📅 Auto-name & Save")) {
          // Use timestamped filename
          m_OnSaveAudioEvent.Trigger(SaveAudioEvent{*this, ""});
        }

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                           "Recorded: %.1f seconds (%d samples)",
                           (float)m_SampleSize / 48000.0f / 2.0f, m_SampleSize);
      }
    }

    // Real-time Transcription Results - only show when transcription mode is
    // enabled
    if (m_RecordingMode == 0 &&
        ImGui::CollapsingHeader("Live Transcription",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Japanese (Original):");
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.2f, 1.0f));
      ImGui::InputTextMultiline(
          "##transcription", const_cast<char*>(m_CurrentTranscription.c_str()),
          m_CurrentTranscription.length() + 1, ImVec2(-1, 80),
          ImGuiInputTextFlags_ReadOnly);
      ImGui::PopStyleColor();

      ImGui::Spacing();
      ImGui::Text("English (Translation):");
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
      ImGui::InputTextMultiline("##translation",
                                const_cast<char*>(m_CurrentTranslation.c_str()),
                                m_CurrentTranslation.length() + 1,
                                ImVec2(-1, 80), ImGuiInputTextFlags_ReadOnly);
      ImGui::PopStyleColor();
    } else if (m_RecordingMode == 1) {
      // Audio-only mode info
      if (ImGui::CollapsingHeader("Audio Recording Info",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Recording Mode: Audio Only");
        ImGui::Text("Transcription: Disabled");
        if (m_SampleSize > 0) {
          ImGui::Text("Current Recording: %.1f seconds",
                      (float)m_SampleSize / 48000.0f / 2.0f);
        }
      }
    }

    // Audio Visualization
    if (m_SampleSize && ImGui::CollapsingHeader("Audio Waveform")) {
      ImGui::PlotConfig conf;
      conf.values.ys = m_Samples;
      conf.values.count = m_SampleSize;
      conf.tooltip.show = true;
      conf.tooltip.format = "x=%.2f, y=%.2f";
      conf.scale.min = -0.1f;
      conf.scale.max = 0.1f;
      conf.grid_x.show = true;
      conf.grid_y.show = true;
      conf.frame_size = ImVec2(400, 200);
      conf.line_thickness = 1.5f;

      ImGui::Plot("Audio Waveform", conf);
    }

    ImGui::End();
  }

  int GetRecordingMode() const { return m_RecordingMode; }
  void SetRecordingMode(int mode) { m_RecordingMode = mode; }

private:
  Core::EventSystem::EventEmitter<RecordEvent> m_OnRecordEvent{};
  Core::EventSystem::EventEmitter<PlaybackEvent> m_OnPlaybackEvent{};
  Core::EventSystem::EventEmitter<StopRecordEvent> m_OnStopRecordEvent{};
  Core::EventSystem::EventEmitter<PauseRecordEvent> m_OnPauseRecordEvent{};
  Core::EventSystem::EventEmitter<SaveAudioEvent> m_OnSaveAudioEvent{};
  float*& m_Samples;
  int& m_SampleSize;
  int selectedInput{-1};
  int selectedOutput{-1};
  int m_RecordingMode{0}; // 0 = transcription, 1 = audio only
  bool& m_Visible;
  bool& m_IsRecording;
  bool& m_IsPlayingBack;
  const std::vector<AudioController::DeviceInfo>& m_InputDevices;
  const std::vector<AudioController::DeviceInfo>& m_OutputDevices;

  // Transcription references
  std::string& m_CurrentTranscription;
  std::string& m_CurrentTranslation;
  bool& m_IsTranscribing;
  bool& m_AutoStartTranscription;
};
} // namespace Transcribe