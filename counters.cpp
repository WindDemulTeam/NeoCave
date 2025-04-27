#include "counters.h"

namespace counters {
void Counters::Insert(Counter *counter) {
  Remove(counter);
  cycle = e_cycle - icount;
  counter->mode |= Counter::kEnable;
  counter->s_cycle = cycle;
  counter->e_cycle = cycle + static_cast<uint64_t>(counter->count) *
                                 static_cast<uint64_t>(counter->rate);

  if (head == nullptr) {
    head = counter;
    counter->next = nullptr;
    counter->prev = nullptr;
  } else {
    if (head->e_cycle < counter->e_cycle) {
      Counter *current_counter = head;
      Counter *next_counter = head->next;

      while (next_counter != nullptr) {
        if (next_counter->e_cycle > counter->e_cycle) {
          break;
        }
        current_counter = next_counter;
        next_counter = next_counter->next;
      }

      if (current_counter == head) {
        counter->next = head->next;
        counter->prev = head;
        if (head->next != nullptr) head->next->prev = counter;
        head->next = counter;
      } else {
        counter->next = next_counter;
        if (next_counter != nullptr) next_counter->prev = counter;
        counter->prev = current_counter;
        current_counter->next = counter;
      }
    } else {
      counter->next = head;
      head->prev = counter;
      head = counter;
      head->prev = nullptr;
    }
  }
  GetNextCounter();
}

void Counters::Remove(Counter *counter) {
  if (counter->mode & Counter::kEnable) {
    if (counter == head) {
      head = head->next;
      if (head != nullptr) head->prev = nullptr;
    } else {
      if (counter->prev != nullptr) counter->prev->next = counter->next;
      if (counter->next != nullptr) counter->next->prev = counter->prev;
    }
    counter->mode &= ~Counter::kEnable;
    counter->next = nullptr;
    counter->prev = nullptr;
    GetNextCounter();
  }
}

void Counters::GetNextCounter() {
  if (nullptr == head) return;

  auto next_counter = head->e_cycle - cycle;
  s_cycle = cycle;
  e_cycle = cycle + next_counter;
  icount = static_cast<int32_t>(next_counter);
}

uint32_t Counters::ReadCounter(Counter *counter) {
  if (counter->mode & Counter::kEnable) {
    uint64_t cleft = counter->e_cycle - e_cycle + static_cast<uint64_t>(icount);
    return static_cast<uint32_t>(cleft / counter->rate);
  } else {
    return static_cast<uint32_t>(counter->count);
  }
}

void Counters::TestCounters() {
  cycle = e_cycle - icount;

  Counter *current_counter = head;
  while (current_counter != nullptr) {
    if (current_counter->e_cycle <= cycle) {
      auto callback = current_counter->callback;
      if (callback) {
        callback(current_counter);
      }
      Remove(current_counter);
      if ((current_counter->mode & Counter::kOneShot) == 0) {
        Insert(current_counter);
        current_counter = head;
        continue;
      }
    } else {
      break;
    }
    current_counter = current_counter->next;
  }
  GetNextCounter();
}
}  // namespace counters