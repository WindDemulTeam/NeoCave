
#include "cave.h"

#include <thread>

Cave3rd::Cave3rd() : gpu_(ram_, cpu_) {
  ram_.fill(0);
  bios_.fill(0);
  games_list_.Init();
  emu_thread_ = new std::thread(&Cave3rd::EmuThread, this);
}

Cave3rd::~Cave3rd() {
  Stop();
  emu_thread_->join();
  delete emu_thread_;
}

void Cave3rd::Start() {
  msg_mutex_.lock();
  message_ = kMsgStart;
  msg_mutex_.unlock();
}

void Cave3rd::Stop() {
  msg_mutex_.lock();
  message_ = kMsgStop;
  msg_mutex_.unlock();
}

void Cave3rd::EmuThread() {
  bool stop = false;

  while (true) {
    msg_mutex_.lock();
    auto msg = message_;
    message_ = kMsgNone;
    msg_mutex_.unlock();

    switch (msg) {
      case kMsgStart:
        Init();
        running_ = true;
        break;
      case kMsgStop:
        Close();
        running_ = false;
        stop = true;
        break;
    }

    if (stop) {
      break;
    }

    if (running_) {
      Execute();
    }
  }
}

void Cave3rd::Init() {
  std::vector<sh3::Map> map;
  sh3::MemHandler mem_handler;

  // BIOS

  mem_handler.read8 = [this](uint32_t addr) -> uint8_t {
    return BiosRead<uint8_t>(addr);
  };
  mem_handler.read16 = [this](uint32_t addr) -> uint16_t {
    return BiosRead<uint16_t>(addr);
  };
  mem_handler.read32 = [this](uint32_t addr) -> uint32_t {
    return BiosRead<uint32_t>(addr);
  };
  mem_handler.write8 = nullptr;
  mem_handler.write16 = nullptr;
  mem_handler.write32 = nullptr;
  map.push_back(sh3::Map(kBiosBase, kBiosSize, mem_handler));

  // RAM

  mem_handler.read8 = [this](uint32_t addr) -> uint8_t {
    return RamRead<uint8_t>(addr);
  };
  mem_handler.read16 = [this](uint32_t addr) -> uint16_t {
    return RamRead<uint16_t>(addr);
  };
  mem_handler.read32 = [this](uint32_t addr) -> uint32_t {
    return RamRead<uint32_t>(addr);
  };
  mem_handler.write8 = [this](uint32_t addr, uint8_t value) -> void {
    RamWrite<uint8_t>(addr, value);
  };
  mem_handler.write16 = [this](uint32_t addr, uint16_t value) -> void {
    RamWrite<uint16_t>(addr, value);
  };
  mem_handler.write32 = [this](uint32_t addr, uint32_t value) -> void {
    RamWrite<uint32_t>(addr, value);
  };
  map.push_back(sh3::Map(kRamBase, kRamSize, mem_handler));

  // NAND

  mem_handler.read8 = [this](uint32_t addr) -> uint8_t { return nand_.Read(); };
  mem_handler.write8 = [this](uint32_t addr, uint8_t value) -> void {
    nand_.Write(value);
  };

  mem_handler.read16 = nullptr;
  mem_handler.read32 = nullptr;
  mem_handler.write16 = nullptr;
  mem_handler.write32 = nullptr;
  map.push_back(sh3::Map(0xB0000000, 0x00010000, mem_handler));

  // SERIAL EEPROM/RTC

  mem_handler.read8 = [this](uint32_t addr) -> uint8_t {
    return rtc9701_.Read8(addr);
  };
  mem_handler.write8 = [this](uint32_t addr, uint8_t value) -> void {
    rtc9701_.Write8(addr, value);
  };

  mem_handler.read16 = nullptr;
  mem_handler.read32 = nullptr;
  mem_handler.write16 = nullptr;
  mem_handler.write32 = nullptr;
  map.push_back(sh3::Map(0xB0C00000, 0x00010000, mem_handler));

  // GPU

  mem_handler.read8 = [this](uint32_t addr) -> uint8_t {
    return gpu_.Read8(addr);
  };
  mem_handler.read32 = [this](uint32_t addr) -> uint32_t {
    return gpu_.Read32(addr);
  };
  mem_handler.write8 = [this](uint32_t addr, uint8_t value) -> void {
    gpu_.Write8(addr, value);
  };
  mem_handler.write32 = [this](uint32_t addr, uint32_t value) -> void {
    gpu_.Write32(addr, value);
  };

  mem_handler.read16 = nullptr;
  mem_handler.write16 = nullptr;
  map.push_back(sh3::Map(0xB8000000, 0x00010000, mem_handler));

  // spu

  mem_handler.write8 = [this](uint32_t addr, uint8_t value) -> void {
    spu_.Write(addr, value);
  };

  mem_handler.read8 = nullptr;
  mem_handler.read16 = nullptr;
  mem_handler.read32 = nullptr;
  mem_handler.write16 = nullptr;
  mem_handler.write32 = nullptr;
  map.push_back(sh3::Map(0xB0400000, 0x00010000, mem_handler));

  cpu_.Init(map);
  cpu_.SetClockRate(4);
  input_data_ = 0xffffffff;
  cpu_.SetIoRead(2, [this]() -> uint8_t {
    return static_cast<uint8_t>(input_data_ >> 0);
  });
  cpu_.SetIoRead(3, [this]() -> uint8_t {
    return static_cast<uint8_t>(input_data_ >> 8);
  });
  cpu_.SetIoRead(10, [this]() -> uint8_t {
    return static_cast<uint8_t>(input_data_ >> 16);
  });

  rtc9701_.Init();
  nand_.Init(&games_list_);
  games_list_.LoadGame(game_idx_, game_path_, true);

  for (int i = 0; i < kBiosSize; i++) {
    bios_[kBiosSize - i - 1] = games_list_.GetGameRomValue(0x08400000 + i);
  }

  for (int i = 0; i < Ymz770::kSpuSize; i++) {
    spu_.WriteRom(i, games_list_.GetGameRomValue(0x08800000 + i));
  }

  gpu_.Init([this](int32_t code) -> void {
    if (code >= 0) {
      cpu_.SetInterruptPending(code);
    } else {
      cpu_.ResetInterruptPending(-code);
    }
  });
}

void Cave3rd::Close() {}

void Cave3rd::Execute() { cpu_.Run(); }