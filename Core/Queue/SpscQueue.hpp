#pragma once

#include <atomic>
#include <new>
#include <thread>

namespace Core {

template <typename Element, std::size_t Size> class SpscQueue {
public:
  static constexpr size_t Capacity{Size + 1};
  SpscQueue()
      : _tail(0), _head(0),
        _array{static_cast<Element*>(::operator new[](
            sizeof(Element) * Capacity,
            std::align_val_t(std::hardware_destructive_interference_size)))} {}

  // not thread-safe make sure destructor is called when no thread is working on
  // it
  virtual ~SpscQueue() {
    auto currentHead = _head.load(std::memory_order_relaxed);
    auto currentTail = _tail.load(std::memory_order_relaxed);
    while (currentHead != currentTail) {
      _array[currentHead].~Element();
      currentHead = Increment(currentHead);
    }
    ::operator delete[](
        _array, std::align_val_t(std::hardware_destructive_interference_size));
  };

  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;

  bool Push(const Element& item) {
    const auto currentTail = _tail.load(std::memory_order_relaxed);
    const auto nextTail = Increment(currentTail);
    if (nextTail != _head.load(std::memory_order_acquire)) {
      _array[currentTail] = item;
      _tail.store(nextTail, std::memory_order_release);
      return true;
    }

    return false;
  }

  bool Push(Element&& item) {
    const auto currentTail = _tail.load(std::memory_order_relaxed);
    const auto nextTail = Increment(currentTail);
    if (nextTail != _head.load(std::memory_order_acquire)) {
      _array[currentTail] = std::move(item);
      _tail.store(nextTail, std::memory_order_release);
      return true;
    }

    return false;
  }
  bool Pop(Element& item) {
    const auto currentHead = _head.load(std::memory_order_relaxed);
    if (currentHead == _tail.load(std::memory_order_acquire)) {
      return false; // empty queue
    }

    item = std::move(_array[currentHead]);
    _head.store(Increment(currentHead), std::memory_order_release);
    return true;
  }

  inline size_t GetSize() const { return Size; }

private:
  size_t Increment(size_t idx) const { return (idx + 1) % Capacity; }

  Element* _array;
  alignas(
      std::hardware_destructive_interference_size) std::atomic<size_t> _tail;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<size_t> _head; // head(output) index
};
} // namespace Core