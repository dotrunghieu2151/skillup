#include "MarinaTranslator.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace Translate {

MarinaTranslator::MarinaTranslator(const Config& config) : m_Config(config) {}

MarinaTranslator::~MarinaTranslator() { Shutdown(); }

bool MarinaTranslator::Initialize() {
  if (IsModelLoaded()) {
    return true; // Already initialized
  }

  LoadModel();

  std::cout << "Marina translator initialized successfully" << std::endl;
  std::cout << "Model: " << m_Config.model_path << std::endl;

  return true;
}

void MarinaTranslator::Shutdown() {
  StopRealTimeTranslation();

  if (m_Translator.has_value()) {
    m_Translator.reset();
  }
}

bool MarinaTranslator::LoadModel() {
  // Check if model path exists
  if (!std::filesystem::exists(m_Config.model_path)) {
    std::cerr << "Model file not found: " << m_Config.model_path << std::endl;
    return false;
  }

  m_Translator.emplace(m_Config.model_path);

  return m_Translator.has_value();
}

bool MarinaTranslator::StartRealTimeTranslation(TranslationCallback callback) {
  if (m_IsTranslating.load()) {
    return false; // Already translating
  }

  if (!Initialize()) {
    std::cerr << "MarinaTranslator used without Initialize(). Please "
                 "Initialize() first."
              << m_Config.model_path << std::endl;
    return false;
  }

  m_Callback = callback;
  m_IsTranslating.store(true);

  translationThread =
      std::jthread([this](std::stop_token s) { TranslationLoop(s); });

  return true;
}

void MarinaTranslator::StopRealTimeTranslation() {
  if (!m_IsTranslating.load()) {
    return;
  }

  m_IsTranslating.store(false);

  translationThread.request_stop();
}

void MarinaTranslator::ProcessText(const std::string& text) {
  if (!IsModelLoaded()) {
    return;
  }

  // write to stream
  m_Stream.Write(text.c_str(), text.size());
}

void MarinaTranslator::TranslationLoop(std::stop_token s) {
  std::string result;
  std::string input;
  result.reserve(1024 * 3);
  input.reserve(1024 * 3);
  while (!s.stop_requested()) {
    const Core::Stream<char>::Buffer& buffer = m_Stream.Read();
    if (!buffer.size) {
      std::this_thread::yield();
      continue;
    }
    if (!result.empty()) {
      result.clear();
    }
    if (!input.empty()) {
      input.clear();
    }

    input.assign(buffer.buffer, buffer.buffer + buffer.size);

    result = Translate(input);

    if (!result.empty() && m_Callback) {
      m_Callback(result);
    }

    std::this_thread::yield();
  }
}

const std::string& MarinaTranslator::Translate(const char* text, int size) {
  return Translate(std::string(text, size));
}

const std::string& MarinaTranslator::Translate(const std::string& text) {
  if (!IsModelLoaded()) {
    return "";
  }
  return m_Translator.value().Generate(text);
}

std::vector<std::string> MarinaTranslator::GetSupportedLanguages() const {
  return {"ja", "en"};
}
} // namespace Translate