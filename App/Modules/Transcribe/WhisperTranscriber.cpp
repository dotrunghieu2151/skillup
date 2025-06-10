#include "WhisperTranscriber.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>

namespace Transcribe {

WhisperTranscriber::WhisperTranscriber(const Config& config)
    : m_Config(config), m_Context(nullptr),
      m_AudioStream(
          WHISPER_SAMPLE_RATE * config.segment_length_ms / 1000,
          std::min(WHISPER_SAMPLE_RATE * (config.segment_length_ms / 1000) * 3,
                   30 * WHISPER_SAMPLE_RATE),
          "AudioStream") {}

WhisperTranscriber::~WhisperTranscriber() { Shutdown(); }

bool WhisperTranscriber::Initialize() {
  if (m_Context != nullptr) {
    return true; // Already initialized
  }

  if (!LoadModel()) {
    std::cerr << "Failed to load Whisper model from: " << m_Config.model_path
              << std::endl;
    return false;
  }

  SetupParams();

  std::cout << "Whisper transcriber initialized successfully" << std::endl;
  std::cout << "Model: " << GetModelInfo() << std::endl;

  return true;
}

void WhisperTranscriber::Shutdown() {
  StopRealTimeTranscription();

  if (m_Context) {
    whisper_free(m_Context);
    m_Context = nullptr;
  }
}

bool WhisperTranscriber::LoadModel() {
  // Check if model file exists
  std::ifstream file(m_Config.model_path);
  if (!file.good()) {
    std::cerr << "Model file not found: " << m_Config.model_path << std::endl;
    return false;
  }

  // Initialize whisper context
  struct whisper_context_params cparams = whisper_context_default_params();
  cparams.use_gpu = m_Config.use_gpu;

  m_Context =
      whisper_init_from_file_with_params(m_Config.model_path.c_str(), cparams);

  return m_Context != nullptr;
}

void WhisperTranscriber::SetupParams() {
  m_Params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

  // Basic parameters
  m_Params.n_threads = m_Config.n_threads;
  m_Params.translate = m_Config.translate_to_english;
  m_Params.print_progress = false;
  m_Params.print_timestamps = true;
  m_Params.print_realtime = true;
  m_Params.print_special = false;
  m_Params.no_timestamps = false;

  // Language setting
  if (m_Config.language != "auto") {
    m_Params.language = m_Config.language.c_str();
  }

  // Audio processing
  m_Params.audio_ctx = 0; // Use full audio context

  // VAD settings
  m_Params.no_speech_thold = m_Config.vad_threshold;
  m_Params.no_context = true;
  m_Params.single_segment = true;
}

bool WhisperTranscriber::StartRealTimeTranscription(
    TranscriptionCallback callback) {
  if (m_IsTranscribing.load()) {
    return false; // Already transcribing
  }

  if (!IsModelLoaded()) {
    if (!Initialize()) {
      return false;
    }
  }

  m_Callback = callback;
  m_IsTranscribing.store(true);

  transcriptionThread =
      std::jthread([this](std::stop_token s) { TranscriptionLoop(s); });

  return true;
}

void WhisperTranscriber::StopRealTimeTranscription() {
  if (!m_IsTranscribing.load()) {
    return;
  }

  m_IsTranscribing.store(false);

  transcriptionThread.request_stop();

  m_AudioStream.Flush();
}

void WhisperTranscriber::ProcessAudioBuffer(const float* samples,
                                            int sample_count, int sample_rate) {
  if (sample_rate != WHISPER_SAMPLE_RATE) {
    std::vector<float> resampled_samples = ResampleAudioSoXR(
        samples, sample_count, sample_rate, WHISPER_SAMPLE_RATE, 2);
    std::vector<float> mono_samples;
    ConvertStereoToMono(resampled_samples.data(), resampled_samples.size(),
                        mono_samples);
    m_AudioStream.Write(mono_samples.data(), mono_samples.size());
  } else {
    m_AudioStream.Write(samples, sample_count);
  }
}

void WhisperTranscriber::TranscriptionLoop(std::stop_token s) {
  const int segment_samples =
      (m_Config.segment_length_ms * WHISPER_SAMPLE_RATE * 1) / 1000;
  const int overlap_samples =
      (m_Config.overlap_ms * WHISPER_SAMPLE_RATE * 1) / 1000;

  auto last_process_time = std::chrono::steady_clock::now();
  m_AudioDataBufferOld.reserve(overlap_samples);

  while (!s.stop_requested()) {
    const Core::Stream<float>::Buffer& buffer = m_AudioStream.Read();
    if (!buffer.size) {
      std::this_thread::yield();
      continue;
    }

    std::vector<float> segment_data(segment_samples);
    segment_data.assign(buffer.buffer, buffer.buffer + buffer.size);

    if (!segment_data.empty()) {
      auto now = std::chrono::steady_clock::now();
      auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now.time_since_epoch())
                              .count();

      AudioSegment segment;
      segment.samples = std::move(segment_data);
      segment.sample_rate = WHISPER_SAMPLE_RATE;
      segment.timestamp_ms = timestamp_ms;

      auto result = ProcessSegment(segment);

      if (!result.text.empty() && m_Callback) {
        m_Callback(result);
      }
    }

    std::this_thread::yield();
  }
}

TranscriptionResult
WhisperTranscriber::ProcessSegment(const AudioSegment& segment) {
  TranscriptionResult result;
  result.start_time_ms = segment.timestamp_ms;
  result.end_time_ms = segment.timestamp_ms + m_Config.segment_length_ms;
  result.is_translation = m_Config.translate_to_english;

  if (!IsModelLoaded() || segment.samples.empty()) {
    return result;
  }

  if (!m_Params.no_context) {
    m_Params.prompt_tokens = m_PromptTokens.data();
    m_Params.prompt_n_tokens = m_PromptTokens.size();
  }
  // Run whisper inference
  int ret = whisper_full(m_Context, m_Params, segment.samples.data(),
                         static_cast<int>(segment.samples.size()));

  if (ret != 0) {
    std::cerr << "Whisper inference failed with code: " << ret << std::endl;
    return result;
  }

  // Extract results
  const int n_segments = whisper_full_n_segments(m_Context);

  std::string full_text;
  float total_confidence = 0.0f;
  int valid_segments = 0;

  for (int i = 0; i < n_segments; ++i) {
    const char* text = whisper_full_get_segment_text(m_Context, i);
    if (text && strlen(text) > 0) {
      if (!full_text.empty()) {
        full_text += " ";
      }
      full_text += text;

      // Get confidence (probability)
      float segment_confidence = 0.0f;
      const int n_tokens = whisper_full_n_tokens(m_Context, i);
      if (!m_Params.no_context) {
        m_PromptTokens.clear();
      }
      for (int j = 0; j < n_tokens; ++j) {
        segment_confidence += whisper_full_get_token_p(m_Context, i, j);
        if (!m_Params.no_context) {
          m_PromptTokens.push_back(whisper_full_get_token_id(m_Context, i, j));
        }
      }
      if (n_tokens > 0) {
        segment_confidence /= n_tokens;
      }

      total_confidence += segment_confidence;
      valid_segments++;
    }
  }

  result.text = full_text;
  result.confidence =
      valid_segments > 0 ? total_confidence / valid_segments : 0.0f;

  // Get detected language
  const int lang_id = whisper_full_lang_id(m_Context);
  result.language = whisper_lang_str(lang_id);

  return result;
}

TranscriptionResult
WhisperTranscriber::TranscribeFile(const std::string& audio_file_path) {
  TranscriptionResult result;

  if (!IsModelLoaded()) {
    if (!Initialize()) {
      return result;
    }
  }

  // Load audio file (this is a simplified version - you might want to use a
  // proper audio library)
  std::vector<float> audio_data;
  // TODO: Implement audio file loading (WAV, MP3, etc.)
  // For now, return empty result

  return result;
}

std::vector<float> WhisperTranscriber::ResampleAudioSoXR(const float* samples,
                                                         int sample_count,
                                                         int input_rate,
                                                         int output_rate,
                                                         int channels) {
  if (input_rate == output_rate) {
    return std::vector<float>(samples, samples + sample_count);
  }

  // Calculate output sample count for interleaved stereo
  double ratio = static_cast<double>(output_rate) / input_rate;
  // size_t output_count = static_cast<size_t>(sample_count * ratio + 0.5);
  size_t output_count = sample_count * ratio + 0.5;
  std::vector<float> output(output_count);

  // Create a one-shot resampler for this specific operation
  soxr_error_t error = nullptr;

  // Configure I/O specification (float32 input and output)
  soxr_io_spec_t io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);

  // Configure quality (SOXR_HQ for high quality)
  soxr_quality_spec_t quality_spec = soxr_quality_spec(SOXR_HQ, 0);

  // Create resampler for 2-channel interleaved stereo
  soxr_t resampler = soxr_create(input_rate, output_rate, channels, &error,
                                 &io_spec, &quality_spec, nullptr);

  if (error || !resampler) {
    std::cerr << "SoX Resampler creation failed: "
              << (error ? soxr_strerror(error) : "Unknown error") << std::endl;

    return {};
  }

  // Perform the resampling in one shot
  size_t input_used = 0;
  size_t output_generated = 0;

  size_t ilen = sample_count / channels;
  size_t olen = output_count / channels;

  error = soxr_process(resampler, samples, ilen, &input_used, output.data(),
                       olen, &output_generated);

  if (error) {
    std::cerr << "SoX Resampler processing failed: " << soxr_strerror(error)
              << std::endl;
    // Return original data as fallback (shouldn't happen with proper input)
    return {};
  }

  // Step 2: Flush remaining buffered output

  size_t flushed = 0;
  error = soxr_process(resampler, NULL, 0, NULL, // no more input
                       output.data() + output_generated * 2,
                       olen - output_generated,
                       &flushed // flush output
  );

  if (error) {
    fprintf(stderr, "soxr_process error (flush): %s\n", error);
  }

  // printf("Flushed frames = %zu\n", flushed);

  // soxr_delete(resampler);

  // std::cout << "SoX Resampler: " << sample_count << " samples @ " <<
  // input_rate
  //           << "Hz -> " << output_generated << " samples @ " << output_rate
  //           << "Hz (used " << input_used << " input samples)" << std::endl;

  return output;
}

bool WhisperTranscriber::IsValidAudioSegment(
    const std::vector<float>& samples) {
  if (samples.empty()) {
    return false;
  }

  // Check if there's actual speech using simple VAD
  return HasSpeech(samples.data(), static_cast<int>(samples.size()));
}

bool WhisperTranscriber::HasSpeech(const float* samples, int sample_count) {
  float rms = CalculateRMS(samples, sample_count);
  return rms > m_Config.vad_threshold * 0.01f; // Simple threshold-based VAD
}

float WhisperTranscriber::CalculateRMS(const float* samples, int sample_count) {
  if (sample_count == 0)
    return 0.0f;

  double sum = 0.0;
  for (int i = 0; i < sample_count; ++i) {
    sum += samples[i] * samples[i];
  }

  return static_cast<float>(std::sqrt(sum / sample_count));
}

void WhisperTranscriber::ConvertStereoToMono(const float* samples,
                                             int sample_count,
                                             std::vector<float>& output) {
  output.reserve(sample_count / 2);
  for (int i = 0; i < sample_count; i += 2) {
    output.push_back((samples[i] + samples[i + 1]) * 0.5f);
  }
}

std::string WhisperTranscriber::GetModelInfo() const {
  if (!IsModelLoaded()) {
    return "No model loaded";
  }

  // Get model information
  return m_Config.model_path; // You can expand this with actual model info
}

std::vector<std::string> WhisperTranscriber::GetSupportedLanguages() const {
  std::vector<std::string> languages;

  // Get all supported languages from whisper
  for (int i = 0; i < whisper_lang_max_id(); ++i) {
    const char* lang = whisper_lang_str(i);
    if (lang) {
      languages.emplace_back(lang);
    }
  }

  return languages;
}
} // namespace Transcribe