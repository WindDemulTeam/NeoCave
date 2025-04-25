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
      Counter *current_�ounter = head;
      Counter *next_�ounter = head->next;

      while (next_�ounter != nullptr) {
        if (next_�ounter->e_cycle > counter->e_cycle) {
          break;
        }
        current_�ounter = next_�ounter;
        next_�ounter = next_�ounter->next;
      }

      if (current_�ounter == head) {
        counter->next = head->next;
        counter->prev = head;
        if (head->next != nullptr) head->next->prev = counter;
        head->next = counter;
      } else {
        counter->next = next_�ounter;
        if (next_�ounter != nullptr) next_�ounter->prev = counter;
        counter->prev = current_�ounter;
        current_�ounter->next = counter;
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

  auto next_�ounter = head->e_cycle - cycle;
  s_cycle = cycle;
  e_cycle = cycle + next_�ounter;
  icount = static_cast<int32_t>(next_�ounter);
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

  Counter *current_�ounter = head;
  while (current_�ounter != nullptr) {
    if (current_�ounter->e_cycle <= cycle) {
      auto callback = current_�ounter->callback;
      if (callback) {
        callback(current_�ounter);
      }
      Remove(current_�ounter);
      if ((current_�ounter->mode & Counter::kOneShot) == 0) {
        Insert(current_�ounter);
        current_�ounter = head;
        continue;
      }
    } else {
      break;
    }
    current_�ounter = current_�ounter->next;
  }
  GetNextCounter();
}
}  // namespace counters