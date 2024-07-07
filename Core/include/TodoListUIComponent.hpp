#pragma once

#include <imgui.h>

#include "IEventEmitter.hpp"
#include "UIComponent.hpp"

namespace Core {
class TodoListUIComponent : public UIComponent, IEventEmitter {
public:
  struct EventTodo : IEventEmitter::EventArgs {
    bool hi;
  };

  TodoListUIComponent(bool& visible) : m_Visible{visible} {}

  void Render() override {
    if (!ImGui::Begin("Example: Auto-resizing window", &m_Visible,
                      ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::End();
      return;
    }

    static int lines = 10;
    ImGui::TextUnformatted(
        "Window will resize every-frame to the size of its content.\n"
        "Note that you probably don't want to query the window size to\n"
        "output your content because that would create a feedback loop.");
    ImGui::SliderInt("Number of lines", &lines, 1, 20);
    for (int i = 0; i < lines; i++)
      ImGui::Text("%*sThis is line %d", i * 4, "",
                  i); // Pad with space to extend size horizontally
    ImGui::End();
  }

  void On(const char*, std::function<void(const EventTodo&)>) override {

  };

  void Off(const char*, std::function<void(const EventTodo&)>) override {

  };

  void Trigger(const char*, const EventTodo&) override {

  };

private:
  bool& m_Visible;
  bool m_IsLoading{false};
};
} // namespace Core