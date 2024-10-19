#include <functional>
#include <iostream>
#include <memory>

#include "Core/Application.hpp"
#include "MainMenuLayer.hpp"
#include "Modules/TaskManagement/TaskLayer.hpp"
#include "Modules/Transcribe/TranscribeLayer.hpp"

int main(void) {
  Core::ApplicationSpecification spec{};
  Core::Application* app = new Core::Application{spec};

  app->PushLayer<MainMenuLayer>();
  app->PushLayer<TaskManagement::TaskLayer>();
  app->PushLayer<Transcribe::TranscribeLayer>();

  app->Run();
  delete app;
}
