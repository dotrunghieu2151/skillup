#pragma once

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace Core {

template <typename Type> class Stream {
public:
  struct alignas(std::hardware_destructive_interference_size) Buffer {
    Type* buffer;
    int maxSize;
    int size{0};
  };

  enum State { Streaming, Paused, Stopped };

  Stream() = delete;
  Stream(int minBufferSize, int maxBufferSize,
         const std::string& name = "Stream")
      : writeBuffer{new Type[maxBufferSize], maxBufferSize, 0},
        middleBuffer{new Type[maxBufferSize], maxBufferSize, 0},
        readBuffer{new Type[maxBufferSize], maxBufferSize, 0},
        minBufferSize{minBufferSize}, name{name} {}

  // non-copyable
  Stream(const Stream&) = delete;
  Stream& operator=(const Stream&) = delete;

  ~Stream() {
    if (!IsStopped()) {
      Stop();
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

  bool IsStreaming() const {
    return m_State.load(std::memory_order_relaxed).state == State::Streaming;
  }

  bool IsPaused() const {
    return m_State.load(std::memory_order_relaxed).state == State::Paused;
  }

  bool IsStopped() const {
    return m_State.load(std::memory_order_relaxed).state == State::Stopped;
  }

  void Start() { UpdateState(State::Streaming); }
  void Pause() { UpdateState(State::Paused); }
  void Stop() { UpdateState(State::Stopped); }
  void Flush() {
    writeBuffer.size = 0;
    middleBuffer.size = 0;
    readBuffer.size = 0;
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

  void Write(const Type* data, int size) {
    // check for overflow
    if (writeBuffer.maxSize == writeBuffer.size) {
      // data loss occurs if write is faster than read
      std::printf("Stream %s: Write Buffer overflow\n", name.c_str());
      return;
    }

    int remainingSpaceInBuffer = writeBuffer.maxSize - writeBuffer.size;
    int copySize = std::min(size, remainingSpaceInBuffer);
    Type* start{writeBuffer.buffer + writeBuffer.size};

    std::memcpy(start, data, copySize * sizeof(Type));
    writeBuffer.size += copySize;

    CommitWrite();
  }

  void Write(Type data, int size) {
    // check for overflow
    if (writeBuffer.maxSize == writeBuffer.size) {
      // data loss occurs if write is faster than read
      std::printf("Stream %s: Write Buffer overflow\n", name.c_str());
      return;
    }

    int remainingSpaceInBuffer = writeBuffer.maxSize - writeBuffer.size;
    int copySize = std::min(size, remainingSpaceInBuffer);

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
  alignas(std::hardware_destructive_interference_size)
      std::atomic<StateObj> m_State;

  alignas(std::hardware_destructive_interference_size) std::atomic_bool isDirty{
      false};

  Buffer writeBuffer;
  Buffer middleBuffer;
  Buffer readBuffer;
  int minBufferSize;
  std::string name;

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
    if (!isDirtyCurrent && writeBuffer.size >= minBufferSize) {
      std::swap(writeBuffer.buffer, middleBuffer.buffer);
      middleBuffer.size = writeBuffer.size;
      writeBuffer.size = 0;
      isDirty.exchange(true, std::memory_order_release);
    }
  }
};
} // namespace Core