#include <memory>

#include <functional>
#include <iostream>
#include <memory>

#include "AppLayers/MainMenuLayer.hpp"
#include "AppLayers/TaskLayer.hpp"
#include "Core/Application.hpp"

int main(void) {
  Core::ApplicationSpecification spec{};
  Core::Application* app = new Core::Application{spec};

  app->PushLayer<MainMenuLayer>();
  app->PushLayer<TaskLayer>();

  app->Run();
  delete app;
}
