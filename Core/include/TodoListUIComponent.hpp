#pragma once

#include <imgui.h>

#include "EventEmitter.hpp"
#include "UIComponent.hpp"

namespace Core {
class TodoListUIComponent : public UIComponent {
public:
  struct EventTodo : EventSystem::Event<TodoListUIComponent> {
    bool hi;
  };

  inline EventSystem::EventEmitter<EventTodo>& OnInitEvent() {
    return m_OnInitEvent;
  }

  TodoListUIComponent(bool& visible) : m_Visible{visible} {}

  void Render() override {
    if (!ImGui::Begin("Example: Auto-resizing window", &m_Visible,
                      ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::End();
      return;
    }
    OnInitEvent().Trigger(EventTodo{*this, true});

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

private:
  EventSystem::EventEmitter<EventTodo> m_OnInitEvent{};
  bool& m_Visible;
  bool m_IsLoading{false};
};
} // namespace Core