#include "WhisperTranscriber.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>

namespace Transcribe {

WhisperTranscriber::WhisperTranscriber(const Config& config)
    : m_Config(config), m_Context(nullptr) {}

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
  m_Params.print_realtime = false;
  m_Params.print_special = false;

  // Language setting
  if (m_Config.language != "auto") {
    m_Params.language = m_Config.language.c_str();
  }

  // Audio processing
  m_Params.audio_ctx = 0; // Use full audio context

  // VAD settings
  m_Params.no_speech_thold = m_Config.vad_threshold;

  // Suppress non-speech tokens
  m_Params.suppress_blank = true;
  m_Params.suppress_nst = true;
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
  m_ShouldStop.store(false);
  m_IsTranscribing.store(true);

  // Start processing thread
  m_ProcessingThread = std::thread(&WhisperTranscriber::ProcessingLoop, this);

  return true;
}

void WhisperTranscriber::StopRealTimeTranscription() {
  if (!m_IsTranscribing.load()) {
    return;
  }

  m_ShouldStop.store(true);
  m_IsTranscribing.store(false);

  if (m_ProcessingThread.joinable()) {
    m_ProcessingThread.join();
  }

  // Clear audio buffer
  {
    std::lock_guard<std::mutex> lock(m_BufferMutex);
    m_AudioBuffer.clear();
    m_HasNewAudio.store(false);
  }
}

void WhisperTranscriber::ProcessAudioBuffer(const float* samples,
                                            int sample_count, int sample_rate) {
  if (!m_IsTranscribing.load()) {
    return;
  }

  std::lock_guard<std::mutex> lock(m_BufferMutex);

  // Resample to 16kHz if needed (Whisper expects 16kHz)
  if (sample_rate != WHISPER_SAMPLE_RATE) {
    auto resampled =
        ResampleAudio(samples, sample_count, sample_rate, WHISPER_SAMPLE_RATE);
    m_AudioBuffer.insert(m_AudioBuffer.end(), resampled.begin(),
                         resampled.end());
  } else {
    m_AudioBuffer.insert(m_AudioBuffer.end(), samples, samples + sample_count);
  }

  m_HasNewAudio.store(true);
}

void WhisperTranscriber::ProcessingLoop() {
  const int segment_samples =
      (m_Config.segment_length_ms * WHISPER_SAMPLE_RATE) / 1000;
  const int overlap_samples =
      (m_Config.overlap_ms * WHISPER_SAMPLE_RATE) / 1000;

  auto last_process_time = std::chrono::steady_clock::now();

  while (!m_ShouldStop.load()) {
    if (!m_HasNewAudio.load()) {
      std::this_thread::yield();
      continue;
    }

    std::vector<float> segment_data;
    {
      std::lock_guard<std::mutex> lock(m_BufferMutex);

      if (m_AudioBuffer.size() >= segment_samples) {
        // Extract segment with overlap
        int extract_size = std::min(segment_samples, (int)m_AudioBuffer.size());
        segment_data.assign(m_AudioBuffer.begin(),
                            m_AudioBuffer.begin() + extract_size);

        // Remove processed samples (keeping overlap)
        int remove_count = extract_size - overlap_samples;
        if (remove_count > 0) {
          m_AudioBuffer.erase(m_AudioBuffer.begin(),
                              m_AudioBuffer.begin() + remove_count);
        }

        if (m_AudioBuffer.size() < segment_samples) {
          m_HasNewAudio.store(false);
        }
      }
    }

    if (!segment_data.empty() && IsValidAudioSegment(segment_data)) {
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
      for (int j = 0; j < n_tokens; ++j) {
        segment_confidence += whisper_full_get_token_p(m_Context, i, j);
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

  return TranscribeBuffer(audio_data.data(),
                          static_cast<int>(audio_data.size()),
                          WHISPER_SAMPLE_RATE);
}

TranscriptionResult WhisperTranscriber::TranscribeBuffer(const float* samples,
                                                         int sample_count,
                                                         int sample_rate) {
  TranscriptionResult result;

  if (!IsModelLoaded()) {
    if (!Initialize()) {
      return result;
    }
  }

  // Resample if needed using high-quality SoX Resampler
  std::vector<float> audio_data;
  if (sample_rate != WHISPER_SAMPLE_RATE) {
    audio_data = ResampleAudioSoXR(samples, sample_count, sample_rate,
                                   WHISPER_SAMPLE_RATE);
  } else {
    audio_data.assign(samples, samples + sample_count);
  }

  AudioSegment segment;
  segment.samples = std::move(audio_data);
  segment.sample_rate = WHISPER_SAMPLE_RATE;
  segment.timestamp_ms = 0;

  return ProcessSegment(segment);
}

std::vector<float> WhisperTranscriber::ResampleAudioSoXR(const float* samples,
                                                         int sample_count,
                                                         int input_rate,
                                                         int output_rate) {
  if (input_rate == output_rate) {
    return std::vector<float>(samples, samples + sample_count);
  }

  // Calculate output sample count
  double ratio = static_cast<double>(output_rate) / input_rate;
  size_t output_count = static_cast<size_t>(sample_count * ratio + 0.5);

  std::vector<float> output(output_count);

  // SoX Resampler error handling
  soxr_error_t error = nullptr;

  // Create SoX Resampler instance with high quality settings
  soxr_io_spec_t io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);

  // Use high quality configuration for polyphase downsampling
  soxr_quality_spec_t quality_spec = soxr_quality_spec(SOXR_HQ, 0);

  // Runtime spec - no specific requirements
  soxr_runtime_spec_t runtime_spec =
      soxr_runtime_spec(1); // Single-threaded for simplicity

  // Create the resampler
  soxr_t resampler =
      soxr_create(input_rate, output_rate, 2, // 2 channels (stereo)
                  &error, &io_spec, &quality_spec, &runtime_spec);

  if (error) {
    std::cerr << "SoX Resampler creation failed: " << soxr_strerror(error)
              << std::endl;
    // Fallback to simple linear interpolation
    return ResampleAudio(samples, sample_count, input_rate, output_rate);
  }

  // Perform the resampling
  size_t input_used = 0;
  size_t output_generated = 0;

  error = soxr_process(resampler, samples, sample_count, &input_used,
                       output.data(), output_count, &output_generated);

  if (error) {
    std::cerr << "SoX Resampler processing failed: " << soxr_strerror(error)
              << std::endl;
    soxr_delete(resampler);
    // Fallback to simple linear interpolation
    return ResampleAudio(samples, sample_count, input_rate, output_rate);
  }

  // Clean up
  soxr_delete(resampler);

  // Resize output vector to actual generated samples
  output.resize(output_generated);

  std::cout << "SoX Resampler: " << sample_count << " samples @ " << input_rate
            << "Hz -> " << output_generated << " samples @ " << output_rate
            << "Hz" << std::endl;

  return output;
}

std::vector<float> WhisperTranscriber::ResampleAudio(const float* samples,
                                                     int sample_count,
                                                     int input_rate,
                                                     int output_rate) {
  if (input_rate == output_rate) {
    return std::vector<float>(samples, samples + sample_count);
  }

  // Simple linear interpolation resampling (fallback method)
  double ratio = static_cast<double>(output_rate) / input_rate;
  int output_count = static_cast<int>(sample_count * ratio);

  std::vector<float> output(output_count);

  for (int i = 0; i < output_count; ++i) {
    double src_index = i / ratio;
    int src_i = static_cast<int>(src_index);
    double frac = src_index - src_i;

    if (src_i + 1 < sample_count) {
      output[i] = static_cast<float>(samples[src_i] * (1.0 - frac) +
                                     samples[src_i + 1] * frac);
    } else if (src_i < sample_count) {
      output[i] = samples[src_i];
    } else {
      output[i] = 0.0f;
    }
  }

  std::cout << "Fallback Linear Resampler: " << sample_count << " samples @ "
            << input_rate << "Hz -> " << output_count << " samples @ "
            << output_rate << "Hz" << std::endl;

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

std::string WhisperTranscriber::GetModelInfo() const {
  if (!IsModelLoaded()) {
    return "No model loaded";
  }

  // Get model information
  return "Whisper model loaded"; // You can expand this with actual model info
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

// TranscriptionJob implementation
TranscriptionJob::TranscriptionJob(
    WhisperTranscriber& transcriber, const std::vector<float>& audio_data,
    int sample_rate, WhisperTranscriber::TranscriptionCallback callback)
    : m_Transcriber(transcriber), m_AudioData(audio_data),
      m_SampleRate(sample_rate), m_Callback(callback) {}

void TranscriptionJob::Execute() {
  auto result = m_Transcriber.TranscribeBuffer(
      m_AudioData.data(), static_cast<int>(m_AudioData.size()), m_SampleRate);

  if (m_Callback && !result.text.empty()) {
    m_Callback(result);
  }
}

} // namespace Transcribe