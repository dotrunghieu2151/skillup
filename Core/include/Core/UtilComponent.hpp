#pragma once

#include <algorithm>
#include <string>

#include <imgui.h>

namespace Core {

namespace UtilComponents {

void ProgressBarSlider(const std::string& id, float& progress,
                       ImVec2 size = ImVec2(0.0f, 0.0f)) {
  ImGuiIO& io = ImGui::GetIO();
  ImGui::BeginGroup();
  ImVec2 progressBarStart{ImGui::GetCursorScreenPos()};
  ImVec4 progressBarFillColor{0.90f, 0.70f, 0.00f, 1.00f};
  if (progress <= 0.3f) {
    progressBarFillColor = ImVec4{1.0f, 0.0f, 0.2f, 1.0f};
  } else if (progress == 1.0f) {
    progressBarFillColor = ImVec4{0.4f, 0.7f, 0.0f, 0.5f};
  }
  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, progressBarFillColor);
  ImGui::ProgressBar(progress, size);
  ImGui::PopStyleColor();
  ImVec2 progressBarSize{ImGui::GetItemRectSize()};
  ImGui::SetCursorScreenPos(progressBarStart);
  ImGui::SetItemAllowOverlap();
  ImGui::InvisibleButton(
      ("###progressBarInteractiveArea" + id).c_str(), progressBarSize,
      ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
  if (ImGui::IsItemActive()) {
    float xAxisDifference{io.MousePos.x - progressBarStart.x};
    progress = {std::clamp(xAxisDifference / progressBarSize.x, 0.0f, 1.0f)};
  }
  ImGui::EndGroup();
}
} // namespace UtilComponents
} // namespace Core