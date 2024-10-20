#pragma once

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <portaudio.h>

#include "Entities/PlaybackStream.hpp"
#include "Entities/RecordStream.hpp"

namespace Transcribe {
class AudioController {

  // private nested classes
private:
  template <typename T> struct StreamConfig {
    StreamConfig() = delete;
    StreamConfig(int inputID, int channels, int frames, int samples)
        : inputDeviceID{inputID}, numOfChannels{channels},
          framesPerBuffer{frames}, sampleRate{samples} {}

    int inputDeviceID;
    int numOfChannels;
    int framesPerBuffer;
    int sampleRate;
  };

  template <> struct StreamConfig<float> {
    using Sample = float;

    StreamConfig() = delete;
    StreamConfig(int inputID, int channels, int frames, int samples)
        : inputDeviceID{inputID}, numOfChannels{channels},
          framesPerBuffer{frames}, sampleRate{samples} {}

    int inputDeviceID;
    int numOfChannels;
    int framesPerBuffer;
    int sampleRate;

    const PaSampleFormat format{paFloat32};
    const Sample sampleSilence{0.0f};
    const std::string printFormat{"%.8f"};
  };

  template <> struct StreamConfig<short> {
    using Sample = short;

    StreamConfig() = delete;
    StreamConfig(int inputID, int channels, int frames, int samples)
        : inputDeviceID{inputID}, numOfChannels{channels},
          framesPerBuffer{frames}, sampleRate{samples} {}

    int inputDeviceID;
    int numOfChannels;
    int framesPerBuffer;
    int sampleRate;

    const PaSampleFormat format{paInt16};
    const Sample sampleSilence{0};
    const std::string printFormat{"%d"};
  };

  template <> struct StreamConfig<unsigned char> {
    using Sample = unsigned char;

    StreamConfig() = delete;
    StreamConfig(int inputID, int channels, int frames, int samples)
        : inputDeviceID{inputID}, numOfChannels{channels},
          framesPerBuffer{frames}, sampleRate{samples} {}

    int inputDeviceID;
    int numOfChannels;
    int framesPerBuffer;
    int sampleRate;

    const PaSampleFormat format{paUInt8};
    const Sample sampleSilence{128};
    const std::string printFormat{"%d"};
  };

  // public nested classes
public:
  using StreamSampleFloat32Config = StreamConfig<float>;

  using StreamSampleInt16Config = StreamConfig<short>;

  using StreamSampleInt8Config = StreamConfig<unsigned char>;

  struct DeviceInfo {
    std::string name;
    int deviceID;
    double latency;
  };
  struct Config {
    int sampleRate{48000};
    int framesPerBuffer{1024};
    int numOfChannels{2};
  };

  // constructors
  AudioController(Config config) : m_Config{config} { InitPortAudio(); }

  ~AudioController() { Pa_Terminate(); }

  // public functions
  int InitPortAudio() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
      printf("ERROR: Pa_Initialize failed %d\n", err);
      return 0;
    }
  }

  std::vector<DeviceInfo> GetInputDevices() const {
    std::vector<DeviceInfo> v{};

    int numDevices = Pa_GetDeviceCount();

    if (numDevices < 0) {
      printf("ERROR: Pa_CountDevices returned 0x%x\n", numDevices);
      return v;
    }

    const PaDeviceInfo* deviceInfo;

    for (int i = 0; i < numDevices; ++i) {
      deviceInfo = Pa_GetDeviceInfo(i);
      bool isWasapi = Pa_GetHostApiInfo(deviceInfo->hostApi)->type == paWASAPI;
      if (isWasapi && deviceInfo->maxInputChannels == m_Config.numOfChannels) {
        v.push_back({deviceInfo->name, i, deviceInfo->defaultLowInputLatency});
      }
    }

    int defaultInputDeviceIndex = Pa_GetDefaultInputDevice();
    std::sort(
        v.begin(), v.end(),
        [defaultInputDeviceIndex](const DeviceInfo& a, const DeviceInfo& b) {
          return a.latency < b.latency;
        });

    return v;
  }

  std::vector<DeviceInfo> GetOutputDevices() const {
    std::vector<DeviceInfo> v{};

    int numDevices = Pa_GetDeviceCount();

    if (numDevices < 0) {
      printf("ERROR: Pa_CountDevices returned 0x%x\n", numDevices);
      return v;
    }

    const PaDeviceInfo* deviceInfo;

    for (int i = 0; i < numDevices; ++i) {
      deviceInfo = Pa_GetDeviceInfo(i);
      bool isWasapi = Pa_GetHostApiInfo(deviceInfo->hostApi)->type == paWASAPI;
      if (isWasapi && deviceInfo->maxOutputChannels == m_Config.numOfChannels) {
        v.push_back({deviceInfo->name, i, deviceInfo->defaultLowInputLatency});
      }
    }

    int defaultOutputDeviceIndex = Pa_GetDefaultOutputDevice();
    std::sort(
        v.begin(), v.end(),
        [defaultOutputDeviceIndex](const DeviceInfo& a, const DeviceInfo& b) {
          return a.latency < b.latency;
        });

    return v;
  }

  template <typename StreamConfigT>
  std::unique_ptr<typename RecordStream<StreamConfigT>>
  Record(int inputDeviceID) {
    using Stream = RecordStream<StreamConfigT>;

    Stream* stream{new Stream{
        m_Config.numOfChannels * m_Config.framesPerBuffer * 4,
        StreamConfigT{inputDeviceID, m_Config.numOfChannels,
                      m_Config.framesPerBuffer, m_Config.sampleRate}}};
    stream->Start();

    return std::unique_ptr<Stream>(stream);
  }

  template <typename StreamConfigT>
  std::unique_ptr<typename PlaybackStream<StreamConfigT>>
  Playback(int outputDeviceID,
           std::vector<typename StreamConfigT::Sample>& data) {
    using Stream = PlaybackStream<StreamConfigT>;

    Stream* stream{new Stream{
        data, StreamConfigT{outputDeviceID, m_Config.numOfChannels,
                            m_Config.framesPerBuffer, m_Config.sampleRate}}};
    stream->Start();

    return std::unique_ptr<Stream>(stream);
  }

  // members
private:
  Config m_Config{};
};
} // namespace Transcribe