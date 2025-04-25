
#include "sh3.h"

#include <cstring>
#include <iostream>

#include "sh3_interpreter.h"

namespace sh3 {

Cpu::Cpu() : interrupt_imask(0), interrupt_pending(0), interrupt_mask(0) {
  interpreter = new Interpreter(this);

  Reset();

  tmu0 =
      new counters::Counter(counters::Counter::kNone, TCNT_0, 1,
                            std::bind(&Cpu::Tmu0, this, std::placeholders::_1));
  tmu1 =
      new counters::Counter(counters::Counter::kNone, TCNT_1, 1,
                            std::bind(&Cpu::Tmu1, this, std::placeholders::_1));
  tmu2 =
      new counters::Counter(counters::Counter::kNone, TCNT_2, 1,
                            std::bind(&Cpu::Tmu2, this, std::placeholders::_1));

  dma0 = new counters::Counter(counters::Counter::kOneShot, 0, 1,
                               std::bind(&Cpu::Dma0, this));

  irq = new counters::Counter(counters::Counter::kOneShot, 0, 0, nullptr);

  interrupt_mask = 0xFFFFFFFF;
  RecomputeInterrupt();
  interrupt_mask = 0xFFFFFFFF;
}

Cpu::~Cpu() {
  delete interpreter;
  delete tmu0;
  delete tmu1;
  delete tmu2;
  delete irq;
}

void Cpu::Init(std::vector<Map>& map) {
  MemHandler mem_handler;

  mem_handler.read8 = [this](uint32_t addr) -> uint8_t {
    return OnchipRead8(addr);
  };
  mem_handler.read16 = [this](uint32_t addr) -> uint16_t {
    return OnchipRead16(addr);
  };
  mem_handler.read32 = [this](uint32_t addr) -> uint32_t {
    return OnchipRead32(addr);
  };
  mem_handler.write8 = [this](uint32_t addr, uint8_t value) -> void {
    OnchipWrite8(addr, value);
  };
  mem_handler.write16 = [this](uint32_t addr, uint16_t value) -> void {
    OnchipWrite16(addr, value);
  };
  mem_handler.write32 = [this](uint32_t addr, uint32_t value) -> void {
    OnchipWrite32(addr, value);
  };
  map.push_back(sh3::Map(0xFF000000, 0x00FFFFFF, mem_handler));
  map.push_back(sh3::Map(0xA4000000, 0x00FFFFFF, mem_handler));

  mem_handler.read8 = nullptr;
  mem_handler.read16 = nullptr;
  mem_handler.write8 = nullptr;
  mem_handler.write16 = nullptr;
  mem_handler.read32 = [this](uint32_t addr) -> uint32_t {
    return ReadIcAddr(addr);
  };
  mem_handler.write32 = [this](uint32_t addr, uint32_t value) -> void {
    WriteIcAddr(addr, value);
  };
  map.push_back(sh3::Map(0xF0000000, 0x01000000, mem_handler));

  mem_handler.read32 = [this](uint32_t addr) -> uint32_t {
    return ReadIcData(addr);
  };
  mem_handler.write32 = [this](uint32_t addr, uint32_t value) -> void {
    WriteIcData(addr, value);
  };
  map.push_back(sh3::Map(0xF1000000, 0x01000000, mem_handler));

  mem_handler.read32 = [this](uint32_t addr) -> uint32_t {
    return ReadOcAddr(addr);
  };
  mem_handler.write32 = [this](uint32_t addr, uint32_t value) -> void {
    WriteOcAddr(addr, value);
  };
  map.push_back(sh3::Map(0xF4000000, 0x01000000, mem_handler));

  mem_handler.read32 = [this](uint32_t addr) -> uint32_t {
    return ReadOcData(addr);
  };
  mem_handler.write32 = [this](uint32_t addr, uint32_t value) -> void {
    WriteOcData(addr, value);
  };
  map.push_back(sh3::Map(0xF5000000, 0x01000000, mem_handler));
  Mmu::Init(map);

  interpreter->Init();
  Reset();
}

void Cpu::Reset(bool soft) {
  state.sr.all = 0x700000f0;
  state.pc = state.npc = 0xa0000000;

  std::memset(&regs1[0], 0, sizeof(regs1));
  std::memset(&regs2[0], 0, sizeof(regs2));

  TCOR_0 = 0xFFFFFFFF;
  TCNT_0 = 0xFFFFFFFF;
  TCOR_1 = 0xFFFFFFFF;
  TCNT_1 = 0xFFFFFFFF;
  TCOR_2 = 0xFFFFFFFF;
  TCNT_2 = 0xFFFFFFFF;
}

void Cpu::Run() {
  interpreter->Run(icount);
  TestCounters();
  TestInterrupt();
}

void Cpu::SwapBank() {
  uint32_t temp[8];

  std::memcpy(temp, &state.r[0], sizeof(uint32_t) * 8);
  std::memcpy(&state.r[0], &state.r[16], sizeof(uint32_t) * 8);
  std::memcpy(&state.r[16], temp, sizeof(uint32_t) * 8);
}

}  // namespace sh3