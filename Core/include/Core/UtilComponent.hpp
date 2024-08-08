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
  ImGui::ProgressBar(progress, size);
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

void TextCentered(const std::string& text) {
  auto windowWidth = ImGui::GetWindowSize().x;
  auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

  ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
  ImGui::Text(text.c_str());
}
} // namespace UtilComponents
} // namespace Core