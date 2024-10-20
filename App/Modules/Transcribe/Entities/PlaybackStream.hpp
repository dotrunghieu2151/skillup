#pragma once

#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <portaudio.h>

namespace Transcribe {

template <typename StreamConfigT> class PlaybackStream {
public:
  enum State { Playing, Paused, Stopped };

  PlaybackStream() = delete;
  PlaybackStream(const std::vector<typename StreamConfigT::Sample>& data,
                 StreamConfigT config)
      : audioData{data}, m_Config{config} {}

  // non-copyable
  PlaybackStream(const PlaybackStream&) = delete;
  PlaybackStream& operator=(const PlaybackStream&) = delete;

  ~PlaybackStream() {
    if (!IsStopped()) {
      Stop();
      Pa_CloseStream(m_paStream);
    }
  }

  bool IsPlaying() const { return Pa_IsStreamActive(m_paStream) == 1; }

  bool IsPaused() const {
    return m_State.load(std::memory_order_relaxed).state == State::Paused;
  }

  bool IsStopped() const {
    return m_State.load(std::memory_order_relaxed).state == State::Stopped;
  }

  void Start() {
    Init();
    PaError err = paNoError;
    err = Pa_StartStream(m_paStream);

    if (err != paNoError) {
      return;
    }
  }
  void Pause() { UpdateState(State::Paused); }
  void Stop() {
    UpdateState(State::Stopped);
    Pa_AbortStream(m_paStream);
  }

private:
  struct StateObj {
    State state;
    unsigned int version{0};
  };

public:
  alignas(std::hardware_destructive_interference_size)
      const std::vector<typename StreamConfigT::Sample>& audioData;
  alignas(
      std::hardware_destructive_interference_size) int audioDataLastReadIndex{
      0};

private:
  alignas(std::hardware_destructive_interference_size) StreamConfigT m_Config;
  alignas(std::hardware_destructive_interference_size) PaStream* m_paStream{
      nullptr};
  alignas(std::hardware_destructive_interference_size)
      std::atomic<StateObj> m_State;

  static int PlaybackCallback(const void* inputBuffer, void* outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void* userData) {
    PlaybackStream* stream = (PlaybackStream*)userData;
    if (stream->IsPaused()) {
      return paContinue;
    } else if (stream->IsStopped()) {
      return paComplete;
    }
    if (stream->audioDataLastReadIndex == stream->audioData.size() - 1) {
      return paComplete;
    }

    int remainingDataSize =
        stream->audioData.size() - (stream->audioDataLastReadIndex + 1);
    int dataToReadSize = (int)framesPerBuffer * stream->m_Config.numOfChannels;
    int readDataSize = remainingDataSize > dataToReadSize
                           ? dataToReadSize
                           : dataToReadSize - remainingDataSize;
    typename StreamConfigT::Sample* output =
        (typename StreamConfigT::Sample*)outputBuffer;

    for (int i{}; i < readDataSize; ++i) {
      *output++ = stream->audioData[stream->audioDataLastReadIndex + i];
    }
    // std::memcpy(output, stream->audioData.data(),
    //             sizeof(StreamConfigT::Sample) * readDataSize);
    stream->audioDataLastReadIndex += readDataSize;
    return paContinue;
  };

  void Init() {
    PaError err = paNoError;
    PaStreamParameters outputParameters;
    outputParameters.device = m_Config.inputDeviceID;
    outputParameters.channelCount = m_Config.numOfChannels;
    outputParameters.sampleFormat = m_Config.format;
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowInputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&m_paStream, NULL, &outputParameters,
                        m_Config.sampleRate, m_Config.framesPerBuffer,
                        paClipOff, /* we won't output out of range samples so
                                      don't bother clipping them */
                        PlaybackCallback, this);
    if (err != paNoError) {
      return;
    }
  }

  void UpdateState(State state) {
    StateObj currentState = m_State.load(std::memory_order_relaxed);
    StateObj nextState{currentState};
    do {
      nextState.state = state;
      nextState.version = currentState.version + 1;
    } while (!m_State.compare_exchange_weak(currentState, nextState,
                                            std::memory_order_release,
                                            std::memory_order_relaxed));
  }
};
} // namespace Transcribe