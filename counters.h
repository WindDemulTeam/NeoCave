#pragma once

#include <cstdint>
#include <functional>

namespace counters {
class Counters;

class Counter {
  friend Counters;

 public:
  using Callback = std::function<void(Counter *counter)>;
  enum Mode : uint32_t { kEnable = 1, kOneShot = 2, kNone = 0 };

  Counter(uint32_t m, uint32_t c, uint32_t r, Callback handler)
      : mode(m),
        count(c),
        rate(r),
        e_cycle(0),
        s_cycle(0),
        callback(handler),
        next(nullptr),
        prev(nullptr) {}

  void SetCount(uint32_t c) { count = c; }
  void SetRate(uint32_t r) { rate = r; }

 private:
  uint32_t mode;
  uint32_t count;
  uint32_t rate;
  uint64_t e_cycle;
  uint64_t s_cycle;
  Callback callback;
  Counter *next;
  Counter *prev;
};

class Counters {
 public:
  int32_t icount;

  void Insert(Counter *counter);
  void Remove(Counter *counter);
  void TestCounters();
  void GetNextCounter();
  uint32_t ReadCounter(Counter *counter);

  Counters() : icount(0), cycle(0), s_cycle(0), e_cycle(0), head(nullptr) {}

 private:
  uint64_t cycle;
  uint64_t s_cycle;
  uint64_t e_cycle;

  Counter *head;
};
}  // namespace counters