#pragma once

#include <cstdint>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

namespace Transcribe {

class AudioFileWriter {
public:
  struct WAVHeader {
    // RIFF header
    char riff_id[4] = {'R', 'I', 'F', 'F'};
    uint32_t riff_size;
    char riff_type[4] = {'W', 'A', 'V', 'E'};

    // Format chunk
    char fmt_id[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t format_tag = 3; // IEEE float
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t bytes_per_second;
    uint16_t block_align;
    uint16_t bits_per_sample = 32;

    // Data chunk
    char data_id[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size;
  };

  static bool WriteWAV(const std::string& filename,
                       const std::vector<float>& samples, int sample_rate,
                       int channels = 2) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
      return false;
    }

    WAVHeader header;
    header.channels = static_cast<uint16_t>(channels);
    header.sample_rate = static_cast<uint32_t>(sample_rate);
    header.bytes_per_second =
        header.sample_rate * header.channels * (header.bits_per_sample / 8);
    header.block_align = header.channels * (header.bits_per_sample / 8);
    header.data_size = static_cast<uint32_t>(samples.size() * sizeof(float));
    header.riff_size = 36 + header.data_size;

    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write audio data
    file.write(reinterpret_cast<const char*>(samples.data()), header.data_size);

    return file.good();
  }

  static bool WriteWAV16(const std::string& filename,
                         const std::vector<float>& samples, int sample_rate,
                         int channels = 2) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
      return false;
    }

    WAVHeader header;
    header.format_tag = 1; // PCM
    header.bits_per_sample = 16;
    header.channels = static_cast<uint16_t>(channels);
    header.sample_rate = static_cast<uint32_t>(sample_rate);
    header.bytes_per_second =
        header.sample_rate * header.channels * (header.bits_per_sample / 8);
    header.block_align = header.channels * (header.bits_per_sample / 8);
    header.data_size = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    header.riff_size = 36 + header.data_size;

    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Convert float samples to 16-bit PCM and write
    for (float sample : samples) {
      int16_t pcm_sample = static_cast<int16_t>(sample * 32767.0f);
      file.write(reinterpret_cast<const char*>(&pcm_sample),
                 sizeof(pcm_sample));
    }

    return file.good();
  }

  static std::string
  GenerateTimestampedFilename(const std::string& prefix = "recording") {
    auto now = std::time(nullptr);
    auto* tm = std::localtime(&now);

    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm);

    return prefix + "_" + timestamp + ".wav";
  }
};

} // namespace Transcribe