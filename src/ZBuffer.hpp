#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <limits>


class ZBuffer {
public:
  ZBuffer(int width, int height) : width(width), height(height) {
    pBuffer = new std::atomic<float>[width * height];
  }
  ZBuffer(const ZBuffer &) = delete;
  ~ZBuffer() {
    delete[] pBuffer;
    pBuffer = nullptr;
  }
  void Clear() {
    for (int i = 0; i < width * height; ++i) {
      pBuffer[i].store(std::numeric_limits<float>::infinity(),
                       std::memory_order_relaxed);
    }
  }
  bool TestAndSet(int pos, float depth) {
    float oldDepth = pBuffer[pos].load(std::memory_order_relaxed);
    while (depth < oldDepth) {
      if (pBuffer[pos].compare_exchange_weak(oldDepth, depth,
                                             std::memory_order_relaxed)) {
        return true;
      }
    }
    return false;
  }

private:
  int width;
  int height;
  std::atomic<float> *pBuffer = nullptr;
};