#include "Test.hpp"
#include <imgui.h>

namespace tests {
tests::TestMenu::TestMenu(Test*& currentTestPtr)
    : m_CurrentTest{currentTestPtr} {}

void tests::TestMenu::OnImGuiRender() {
  for (auto& test : m_Tests) {
    if (ImGui::Button(test.first.c_str())) {
      m_CurrentTest = test.second();
    }
  }
}
} // namespace tests