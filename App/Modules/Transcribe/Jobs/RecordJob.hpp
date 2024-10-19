#pragma once

#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <portaudio.h>

#include "Core/Job/Job.hpp"
#include "Core/Serialization/FileStream.hpp"

namespace Transcribe {
template <typename StreamConfigT> class RecordJob : public Core::Job {
public:
  class RecordStream;

private:
  int m_InputDeviceID;
  int m_NumOfChannels;
  int m_SampleRate;
  int m_FramesPerBuffer = -1;
  StreamConfigT m_StreamConfig;
  RecordStream* m_Output;

public:
  RecordJob(int inputDeviceID, int numOfChannels, int sampleRate,
            int framesPerBuffer, const StreamConfigT& config,
            RecordStream* recordStreamPtr)
      : m_InputDeviceID{inputDeviceID}, m_NumOfChannels{numOfChannels},
        m_SampleRate{sampleRate}, m_FramesPerBuffer{framesPerBuffer},
        m_StreamConfig{config}, m_Output{recordStreamPtr} {}

  void Execute() override {
    m_Output->UpdateState(RecordStream::State::Recording);

    Record();

    m_Output->UpdateState(RecordStream::State::Stopped);
  }

private:
  static int RecordCallback(const void* inputBuffer, void* outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags, void* userData) {
    RecordJob* job = (RecordJob*)userData;
    if (job->m_Output->IsPaused()) {
      return paContinue;
    } else if (job->m_Output->IsStopping() || job->m_Output->IsStopped()) {
      return paComplete;
    }

    const typename StreamConfigT::Sample* input =
        (const typename StreamConfigT::Sample*)inputBuffer;
    if (input == NULL) {
      job->m_Output->Write(job->m_StreamConfig.sampleSilence,
                           (int)framesPerBuffer * job->m_NumOfChannels);
    } else {
      job->m_Output->Write(input, (int)framesPerBuffer * job->m_NumOfChannels);
    }

    return paContinue;
  };

  void Record() {
    PaError err = paNoError;
    PaStreamParameters inputParameters;
    PaStream* stream;

    inputParameters.device = m_InputDeviceID;
    inputParameters.channelCount = m_NumOfChannels;
    inputParameters.sampleFormat = m_StreamConfig.format;
    inputParameters.suggestedLatency =
        Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    /* Record audio. -------------------------------------------- */
    err =
        Pa_OpenStream(&stream, &inputParameters, NULL, /* &outputParameters, */
                      m_SampleRate, m_FramesPerBuffer,
                      paClipOff, /* we won't output out of range samples so
                                    don't bother clipping them */
                      RecordCallback, this);
    if (err != paNoError) {
      return;
    }

    err = Pa_StartStream(stream);

    if (err != paNoError) {
      return;
    }

    while (((err = Pa_IsStreamActive(stream)) == 1) &&
           !m_Output->IsStopping()) {
      Pa_Sleep(1000);
    }

    if (err < 0) {
      return;
    }

    if (m_Output->IsStopping()) {
      Pa_AbortStream(stream);
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
      return;
    }
  }

public:
  class RecordStream {
    friend class RecordJob;

  public:
    struct alignas(std::hardware_destructive_interference_size) Buffer {
      StreamConfigT::Sample* buffer;
      int maxSize;
      int size{0};
    };

    enum State { Recording, Paused, Stopping, Stopped };

    bool IsRecording() const {
      return m_State.load(std::memory_order_relaxed).state == State::Recording;
    }

    bool IsPaused() const {
      return m_State.load(std::memory_order_relaxed).state == State::Paused;
    }

    bool IsStopped() const {
      return m_State.load(std::memory_order_relaxed).state == State::Stopped;
    }

    bool IsStopping() const {
      return m_State.load(std::memory_order_relaxed).state == State::Stopping;
    }

    void Pause() { UpdateState(State::Paused); }
    void Stop() { UpdateState(State::Stopping); }

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

    RecordStream() = delete;
    RecordStream(int bufferSize)
        : writeBuffer{new StreamConfigT::Sample[bufferSize], bufferSize, 0},
          middleBuffer{new StreamConfigT::Sample[bufferSize], bufferSize, 0},
          readBuffer{new StreamConfigT::Sample[bufferSize], bufferSize, 0} {}

    // non-copyable
    RecordStream(const RecordStream&) = delete;
    RecordStream& operator=(const RecordStream&) = delete;

    ~RecordStream() {
      if (!IsStopped()) {
        UpdateState(State::Stopping);

        while (!IsStopped()) {
          printf("Waiting for audio thread to stop stream...");
        }
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

  private:
    struct StateObj {
      State state;
      unsigned int version{0};
    };

  private:
    alignas(std::hardware_destructive_interference_size)
        std::atomic<StateObj> m_State;

    alignas(std::hardware_destructive_interference_size)
        std::atomic_bool isDirty{false};

    Buffer writeBuffer;
    Buffer middleBuffer;
    Buffer readBuffer;

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
};
} // namespace Transcribe
