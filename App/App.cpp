#include <memory>

#include "Application.hpp"
#include "TodoListUIComponent.hpp"
#include <iostream>

class ExampleLayer : public Core::ApplicationLayer {
public:
  bool open{false};
  std::shared_ptr<Core::TodoListUIComponent> todoListUIComponent;
  ExampleLayer()
      : todoListUIComponent{std::make_shared<Core::TodoListUIComponent>(open)} {
  }

  void OnAttach() override {
    todoListUIComponent->OnInitEvent() +=
        [](const Core::TodoListUIComponent::EventTodo& event) {
          ImGui::ShowDemoWindow();
        };
  }

  void OnUIRender() override {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Create")) {
          open = true;
        }
        if (ImGui::MenuItem("Open", "Ctrl+O")) {
        }
        if (ImGui::MenuItem("Save", "Ctrl+S")) {
        }
        if (ImGui::MenuItem("Save as..")) {
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }
    if (open) {
      todoListUIComponent->Render();
    }
  }
};

int main(void) {
  Core::ApplicationSpecification spec{};
  Core::Application* app = new Core::Application{spec};
  std::shared_ptr<ExampleLayer> exampleLayer = std::make_shared<ExampleLayer>();
  app->PushLayer(exampleLayer);

  app->Run();
  delete app;
}
