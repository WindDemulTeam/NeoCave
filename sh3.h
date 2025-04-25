#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include "sh3_mmu.h"
#include "sh3_onchip.h"

namespace sh3 {
class Interpreter;
class Jit;

struct State {
  uint32_t r[16 + 8];

  uint32_t ssr;
  uint32_t gbr;
  uint32_t macl;
  uint32_t mach;
  uint32_t pr;
  uint32_t spc;
  uint32_t vbr;

  union {
    struct {
      uint32_t t : 1;
      uint32_t s : 1;
      uint32_t : 2;
      uint32_t imask : 4;
      uint32_t q : 1;
      uint32_t m : 1;
      uint32_t : 5;
      uint32_t fd : 1;
      uint32_t : 12;
      uint32_t bl : 1;
      uint32_t rb : 1;
      uint32_t md : 1;
      uint32_t : 1;
    };
    uint32_t all;
  } sr;

  uint32_t pc;
  uint32_t npc;
};

class Cpu : public Mmu, public counters::Counters {
  friend Interpreter;
  friend Jit;

 public:
  const static uint32_t kHz = 51200000;

  Cpu();
  ~Cpu();

  void Init(std::vector<Map> &map);
  void Reset(bool soft = false);
  void Run();

  void SetClockRate(uint32_t clk) { pclk_rate = clk; }

  void SetIoRead(int port, std::function<uint8_t()> io_r) {
    io_read[port] = io_r;
  }

  uint8_t &OnchipRef8(uint32_t addr) {
    return addr > 0xff000000 ? (uint8_t &)regs2[addr & (regs2.size() - 1)]
                             : (uint8_t &)regs1[addr & (regs1.size() - 1)];
  }
  uint16_t &OnchipRef16(uint32_t addr) {
    return addr > 0xff000000 ? (uint16_t &)regs2[addr & (regs2.size() - 1)]
                             : (uint16_t &)regs1[addr & (regs1.size() - 1)];
  }
  uint32_t &OnchipRef32(uint32_t addr) {
    return addr > 0xff000000 ? (uint32_t &)regs2[addr & (regs2.size() - 1)]
                             : (uint32_t &)regs1[addr & (regs1.size() - 1)];
  }

  void SetInterruptPending(uint32_t intr);
  void ResetInterruptPending(uint32_t intr);

 private:
  alignas(64) State state;

  std::array<uint8_t, 0x10000> regs1;
  std::array<uint8_t, 0x10000> regs2;

  uint8_t OnchipRead8(uint32_t addr);
  uint16_t OnchipRead16(uint32_t addr);
  uint32_t OnchipRead32(uint32_t addr);

  void OnchipWrite8(uint32_t addr, uint8_t value);
  void OnchipWrite16(uint32_t addr, uint16_t value);
  void OnchipWrite32(uint32_t addr, uint32_t value);

  const uint32_t tmu_divider[8] = {4, 16, 64, 256, 1024, 1024, 1024, 1024};
  uint32_t pclk_rate;

  uint32_t interrupt_imask;
  uint32_t interrupt_pending;
  uint32_t interrupt_mask;
  uint32_t interrupt_bit[32];
  uint32_t interrupt_level_bit[17];
  uint32_t interrupt_env_id[32];
  uint32_t interrupt2_env_id[32];
  void (Cpu::*interrupt_request[32])();

  std::function<uint8_t()> io_read[11];

  counters::Counter *tmu0;
  counters::Counter *tmu1;
  counters::Counter *tmu2;
  counters::Counter *dma0;
  counters::Counter *irq;

  void Tmu0(counters::Counter *counter);
  void Tmu1(counters::Counter *counter);
  void Tmu2(counters::Counter *counter);

  void Dma0();

  template <int irl>
  int GetIrlPriority() {
    return 15 - irl;
  }

  template <int idx>
  int GetPriorityA() {
    return OnchipRef16(kIpra) >> (4 * idx) & 0xf;
  }

  template <int idx>
  int GetPriorityB() {
    return OnchipRef16(kIprb) >> (4 * idx) & 0xF;
  }

  template <int idx>
  int GetPriorityC() {
    return OnchipRef16(kIprc) >> (4 * idx) & 0xF;
  }

  template <int idx>
  int GetPriorityD() {
    return OnchipRef16(kIprd) >> (4 * idx) & 0xF;
  }

  template <int idx>
  int GetPriorityE() {
    return OnchipRef16(kIpre) >> (4 * idx) & 0xF;
  }

  int GetPriorityNmi() { return 16; }

  template <int idx>
  int GetIntevtA() {
    return idx;
  }

  template <int idx>
  int GetIntevtB() {
    return idx;
  }

  template <int idx>
  int GetIntevtC() {
    return ((16 - GetPriorityC<idx>()) * 0x20) + 0x200;
  }

  template <int idx>
  int GetIntevtD() {
    return ((16 - GetPriorityD<idx>()) * 0x20) + 0x200;
  }

  template <int idx>
  int GetIntevtE() {
    return ((16 - GetPriorityE<idx>()) * 0x20) + 0x200;
  }

  void SetRequestDummy() {}

  template <int idx>
  void SetRequest0() {
    OnchipRef8(kIrr0) |= (1 << idx);
  }

  template <int idx>
  void SetRequest1() {
    OnchipRef8(kIrr1) |= (1 << idx);
  }

  template <int idx>
  void SetRequest2() {
    OnchipRef8(kIrr2) |= (1 << idx);
  }

  struct InterruptSourceList {
    int (Cpu::*get_level)();
    int (Cpu::*get_intevt)();
    uint32_t int_evn_code;
    void (Cpu::*interrupt_request)();
  };

  constexpr static InterruptSourceList interrupt_source_list[7] = {
      {&Cpu::GetPriorityC<0>, &Cpu::GetIntevtC<0>, 0x600, &Cpu::SetRequest0<0>},
      {&Cpu::GetPriorityC<1>, &Cpu::GetIntevtC<1>, 0x620, &Cpu::SetRequest0<1>},
      {&Cpu::GetPriorityC<2>, &Cpu::GetIntevtC<2>, 0x640, &Cpu::SetRequest0<2>},

      {&Cpu::GetPriorityA<3>, &Cpu::GetIntevtA<0x400>, 0x400,
       &Cpu::SetRequestDummy},

      {&Cpu::GetPriorityA<2>, &Cpu::GetIntevtA<0x420>, 0x420,
       &Cpu::SetRequestDummy},

      {&Cpu::GetPriorityA<1>, &Cpu::GetIntevtA<0x440>, 0x440,
       &Cpu::SetRequestDummy},

      {&Cpu::GetPriorityA<1>, &Cpu::GetIntevtA<0x460>, 0x460,
       &Cpu::SetRequestDummy},

  };

  void SetInterruptMask(uint32_t intr);
  void ResetInterruptMask(uint32_t intr);
  void RecomputeInterrupt();
  void RecomputeImask();
  void TestInterrupt();
  void Interrupt(uint32_t intevt);

  void SwapBank();

  Interpreter *interpreter;

  uint32_t ReadIcAddr(uint32_t addr) { return 0; }
  void WriteIcAddr(uint32_t addr, uint32_t v) {}

  uint32_t ReadIcData(uint32_t addr) { return 0; }
  void WriteIcData(uint32_t addr, uint32_t v) {}

  uint32_t ReadOcAddr(uint32_t addr) { return 0; }
  void WriteOcAddr(uint32_t addr, uint32_t v) {}

  uint32_t ReadOcData(uint32_t addr) { return 0; }
  void WriteOcData(uint32_t addr, uint32_t v) {}
};

}  // namespace sh3