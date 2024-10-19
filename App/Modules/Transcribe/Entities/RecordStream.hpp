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

template <typename StreamConfigT> class RecordStream {
public:
  struct alignas(std::hardware_destructive_interference_size) Buffer {
    StreamConfigT::Sample* buffer;
    int maxSize;
    int size{0};
  };

  enum State { Recording, Paused, Stopped };

  RecordStream() = delete;
  RecordStream(int bufferSize, StreamConfigT config)
      : m_Config{config},
        writeBuffer{new StreamConfigT::Sample[bufferSize], bufferSize, 0},
        middleBuffer{new StreamConfigT::Sample[bufferSize], bufferSize, 0},
        readBuffer{new StreamConfigT::Sample[bufferSize], bufferSize, 0} {}

  // non-copyable
  RecordStream(const RecordStream&) = delete;
  RecordStream& operator=(const RecordStream&) = delete;

  ~RecordStream() {
    if (!IsStopped()) {
      Stop();
      Pa_CloseStream(m_paStream);
    }

    delete[] writeBuffer.buffer;
    writeBuffer.buffer = nullptr;
    writeBuffer.size = 0;

    delete[] middleBuffer.buffer;
    middleBuffer.buffer = nullptr;
    middleBuffer.size = 0;

    delete[] readBuffer.buffer;
    readBuffer.buffer = nullptr;
    readBuffer.size = 0;
  }

  bool IsRecording() const { return Pa_IsStreamActive(m_paStream) == 1; }

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

  const Buffer& Read() {
    bool isDirtyCurrent = isDirty.load(std::memory_order_acquire);
    // no new data, exit
    if (!isDirtyCurrent) {
      readBuffer.size = 0;
      return readBuffer;
    }

    std::swap(readBuffer.buffer, middleBuffer.buffer);
    readBuffer.size = middleBuffer.size;
    middleBuffer.size = 0;
    isDirty.exchange(false, std::memory_order_release);

    return readBuffer;
  }

  void Write(const typename StreamConfigT::Sample* data, int size) {
    // check for overflow
    if (writeBuffer.maxSize == writeBuffer.size) {
      // data loss occurs if write is faster than read
      return;
    }

    int remainingSpaceInBuffer = writeBuffer.maxSize - writeBuffer.size;
    int copySize =
        size > remainingSpaceInBuffer ? remainingSpaceInBuffer : size;
    typename StreamConfigT::Sample* start{writeBuffer.buffer +
                                          writeBuffer.size};

    std::memcpy(start, data, copySize * sizeof(StreamConfigT::Sample));
    writeBuffer.size += copySize;

    CommitWrite();
  }

  void Write(StreamConfigT::Sample data, int size) {
    // check for overflow
    if (writeBuffer.maxSize == writeBuffer.size) {
      // data loss occurs if write is faster than read
      return;
    }

    int remainingSpaceInBuffer = writeBuffer.maxSize - writeBuffer.size;
    int copySize =
        size > remainingSpaceInBuffer ? remainingSpaceInBuffer : size;

    for (int i{0}; i < copySize; ++i) {
      writeBuffer.buffer[writeBuffer.size + i] = data;
    }

    writeBuffer.size += copySize;

    CommitWrite();
  }

private:
  struct StateObj {
    State state;
    unsigned int version{0};
  };

private:
  alignas(std::hardware_destructive_interference_size) StreamConfigT m_Config;
  alignas(std::hardware_destructive_interference_size) PaStream* m_paStream{
      nullptr};
  alignas(std::hardware_destructive_interference_size)
      std::atomic<StateObj> m_State;

  alignas(std::hardware_destructive_interference_size) std::atomic_bool isDirty{
      false};

  Buffer writeBuffer;
  Buffer middleBuffer;
  Buffer readBuffer;

  static int RecordCallback(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags, void* userData) {
    RecordStream* stream = (RecordStream*)userData;
    if (stream->IsPaused()) {
      return paContinue;
    } else if (stream->IsStopped()) {
      return paComplete;
    }

    const typename StreamConfigT::Sample* input =
        (const typename StreamConfigT::Sample*)inputBuffer;
    if (input == NULL) {
      stream->Write(stream->m_Config.sampleSilence,
                    (int)framesPerBuffer * stream->m_Config.numOfChannels);
    } else {
      stream->Write(input,
                    (int)framesPerBuffer * stream->m_Config.numOfChannels);
    }

    return paContinue;
  };

  void Init() {
    PaError err = paNoError;
    PaStreamParameters inputParameters;
    inputParameters.device = m_Config.inputDeviceID;
    inputParameters.channelCount = m_Config.numOfChannels;
    inputParameters.sampleFormat = m_Config.format;
    inputParameters.suggestedLatency =
        Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&m_paStream, &inputParameters,
                        NULL, /* &outputParameters, */
                        m_Config.sampleRate, m_Config.framesPerBuffer,
                        paClipOff, /* we won't output out of range samples so
                                      don't bother clipping them */
                        RecordCallback, this);
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

  void CommitWrite() {
    // lock-free/ wait-free
    // try to acquire atomic and swap, if not just continue
    bool isDirtyCurrent = isDirty.load(std::memory_order_acquire);
    if (!isDirtyCurrent) {
      std::swap(writeBuffer.buffer, middleBuffer.buffer);
      middleBuffer.size = writeBuffer.size;
      writeBuffer.size = 0;
      isDirty.exchange(true, std::memory_order_release);
    }
  }
};
} // namespace Transcribe