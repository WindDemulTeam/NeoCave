#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

#include "blitter.h"
#include "counters.h"
#include "nand.h"
#include "roms.h"
#include "rtc9701.h"
#include "sh3.h"
#include "ymz770.h"

enum : uint32_t {
  kBiosSize = 0x00400000,
  kRamSize = 0x01000000,
};

enum : uint32_t {
  kBiosBase = 0x00000000,
  kRamBase = 0x0c000000,
};

class Cave3rd {
 public:
  Cave3rd();
  ~Cave3rd();

  void Start();
  void Stop();
  uint16_t *GetBlitterData() { return gpu_.GetBlitterData(); }
  int16_t GetNextSample() { return spu_.GetNextSample(); }
  void SetInputState(uint32_t input) { input_data_ = input; }
  GamesList &GetGameList() { return games_list_; }
  void SetGame(int idx, std::string path) {
    game_idx_ = idx;
    game_path_ = path;
  }

 private:
  bool running_ = false;
  int game_idx_;
  std::string game_path_;

  enum ThreadMessage { kMsgNone = 0, kMsgStart, kMsgStop };

  ThreadMessage message_ = kMsgNone;

  std::thread *emu_thread_;
  std::mutex msg_mutex_;

  sh3::Cpu cpu_;
  Blitter gpu_;
  Ymz770 spu_;
  Nand nand_;
  Rtc9701 rtc9701_;
  GamesList games_list_;

  std::array<uint8_t, kBiosSize> bios_;
  std::array<uint8_t, kRamSize> ram_;
  uint32_t input_data_;

  template <typename T>
  T BiosRead(uint32_t addr) {
    addr &= bios_.size() - 1;
    return *(T *)&bios_[bios_.size() - addr - sizeof(T)];
  }

  template <typename T>
  T RamRead(uint32_t addr) {
    addr &= ram_.size() - 1;
    return *(T *)&ram_[ram_.size() - addr - sizeof(T)];
  }

  template <typename T>
  void RamWrite(uint32_t addr, T value) {
    addr &= ram_.size() - 1;
    *(T *)&ram_[ram_.size() - addr - sizeof(T)] = value;
  }

  void EmuThread();

  void Init();
  void Execute();
  void Close();
};