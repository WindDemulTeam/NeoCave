#include "sh3_onchip.h"

#include <cstring>
#include <iostream>

#include "sh3.h"

namespace sh3 {
void Cpu::RecomputeInterrupt() {
  uint32_t cnt = 0;
  uint32_t vpend = interrupt_pending;
  uint32_t vmask = interrupt_mask;
  interrupt_pending = 0;
  interrupt_mask = 0;

  for (uint32_t i = 0; i < 16; i++) {
    for (int32_t j = std::size(interrupt_source_list) - 1; j >= 0; j--) {
      auto interrupts = &interrupt_source_list[j];
      auto& get_level = interrupts->get_level;
      if ((this->*get_level)() == i) {
        auto& get_intevt = interrupts->get_intevt;
        interrupt_env_id[cnt] = (this->*get_intevt)();
        interrupt2_env_id[cnt] = interrupts->int_evn_code;
        interrupt_request[cnt] = interrupts->interrupt_request;
        bool p = (interrupt_bit[j] & vpend) != 0;
        bool m = (interrupt_bit[j] & vmask) != 0;
        interrupt_bit[j] = 1 << cnt;
        if (p) interrupt_pending |= interrupt_bit[j];
        if (m) interrupt_mask |= interrupt_bit[j];
        cnt++;
      }
    }
    interrupt_level_bit[i] = (1 << cnt) - 1;
  }
  RecomputeImask();
}

void Cpu::SetInterruptPending(uint32_t intr) {
  interrupt_pending |= interrupt_bit[intr];
  auto interrupt = (interrupt_pending & interrupt_mask) > interrupt_imask;
  if (interrupt) {
    Insert(irq);
  }
}

void Cpu::ResetInterruptPending(uint32_t intr) {
  interrupt_pending &= ~interrupt_bit[intr];
  auto interrupt = (interrupt_pending & interrupt_mask) > interrupt_imask;
  if (interrupt) {
    Insert(irq);
  }
}
void Cpu::SetInterruptMask(uint32_t intr) {
  interrupt_mask |= interrupt_bit[intr];
  auto interrupt = (interrupt_pending & interrupt_mask) > interrupt_imask;
  if (interrupt) {
    Insert(irq);
  }
}
void Cpu::ResetInterruptMask(uint32_t intr) {
  interrupt_mask &= ~interrupt_bit[intr];
  auto interrupt = (interrupt_pending & interrupt_mask) > interrupt_imask;
  if (interrupt) {
    Insert(irq);
  }
}

void Cpu::RecomputeImask() {
  interrupt_imask =
      state.sr.bl ? 0xFFFFFFFF : interrupt_level_bit[state.sr.imask];

  auto interrupt = (interrupt_pending & interrupt_mask) > interrupt_imask;
  if (interrupt) {
    Insert(irq);
  }
}

void Cpu::TestInterrupt() {
  auto pend = interrupt_pending;
  auto mask = interrupt_mask;

  if ((pend & mask) > interrupt_imask) {
    for (int i = 31; i >= 0; i--) {
      if (pend & (1 << i)) {
        Interrupt(i);
        break;
      }
    }
  }
}

void Cpu::Interrupt(uint32_t intevt) {
  state.spc = state.pc;
  state.ssr = state.sr.all;

  INTEVT = interrupt_env_id[intevt];
  INTEVT2 = interrupt2_env_id[intevt];

  (this->*interrupt_request[intevt])();

  if (!state.sr.rb) {
    SwapBank();
  }

  state.sr.md = 1;
  state.sr.rb = 1;
  state.sr.bl = 1;
  state.pc = state.vbr + 0x600;

  RecomputeImask();
}

void Cpu::Tmu0(counters::Counter* counter) {
  counter->SetCount(TCOR_0 + 1);
  TCR_0 |= 1 << 8;
  SetInterruptPending(kTmu0Tuni0);
}

void Cpu::Tmu1(counters::Counter* counter) {
  counter->SetCount(TCOR_1 + 1);
  TCR_1 |= 1 << 8;
  SetInterruptPending(kTmu1Tuni1);
}

void Cpu::Tmu2(counters::Counter* counter) {
  counter->SetCount(TCOR_2 + 1);
  TCR_2 |= 1 << 8;
  SetInterruptPending(kTmu2Tuni2);
}

void Cpu::Dma0() {
  uint32_t size = DMATCR_0;

  if ((CHCR_0 & 0x0000c000) == 0x00004000) {
    DAR_0 += size;
  }

  if ((CHCR_0 & 0x00003000) == 0x00001000) {
    SAR_0 += size;
  }

  DMATCR_0 = 0;
  CHCR_0 &= ~0x01;
  CHCR_0 |= 0x02;
}

uint8_t Cpu::OnchipRead8(uint32_t addr) {
  switch (addr) {
    case kPadr:
    case kPbdr:
    case kPcdr:
    case kPddr:
    case kPedr:
    case kPfdr:
    case kPgdr:
    case kPhdr:
    case kPjdr:
    case kPkdr:
    case kPldr:
      if (io_read[(addr - kPadr) >> 1]) {
        return io_read[(addr - kPadr) >> 1]();
      }
      return 0xff;

    default:
      return OnchipRef8(addr);
  }
}

uint16_t Cpu::OnchipRead16(uint32_t addr) {
  switch (addr) {
    default:
      return OnchipRef16(addr);
  }
}

uint32_t Cpu::OnchipRead32(uint32_t addr) {
  switch (addr) {
    case kTcnt_0:
      return ReadCounter(tmu0);
    case kTcnt_1:
      return ReadCounter(tmu1);
    case kTcnt_2:
      return ReadCounter(tmu2);
    default:
      return OnchipRef32(addr);
  }
}

void Cpu::OnchipWrite8(uint32_t addr, uint8_t value) {
  uint8_t tchanged;

  switch (addr) {
    case kIrr0:
      IRR0 = value;
      if ((value & 0x01) == 0) ResetInterruptPending(kIrl0);
      if ((value & 0x02) == 0) ResetInterruptPending(kIrl1);
      if ((value & 0x04) == 0) ResetInterruptPending(kIrl2);
      break;

    case kTstr:
      tchanged = TSTR ^ value;
      TSTR = value;
      if (tchanged & 1) {
        if (value & 1) {
          tmu0->SetCount(ReadCounter(tmu0));
          tmu0->SetRate(tmu_divider[TCR_0 & 7] * pclk_rate);
          Insert(tmu0);
        } else {
          uint32_t count = ReadCounter(tmu0);
          Remove(tmu0);
          tmu0->SetCount(count);
        }
      }
      if (tchanged & 2) {
        if (value & 2) {
          tmu1->SetCount(ReadCounter(tmu1));
          tmu1->SetRate(tmu_divider[TCR_1 & 7] * pclk_rate);
          Insert(tmu1);
        } else {
          uint32_t count = ReadCounter(tmu1);
          Remove(tmu1);
          tmu1->SetCount(count);
        }
      }
      if (tchanged & 4) {
        if (value & 4) {
          tmu2->SetCount(ReadCounter(tmu2));
          tmu2->SetRate(tmu_divider[TCR_2 & 7] * pclk_rate);
          Insert(tmu1);
        } else {
          uint32_t count = ReadCounter(tmu2);
          Remove(tmu2);
          tmu2->SetCount(count);
        }
      }
      break;

    default:
      OnchipRef8(addr) = value;
      break;
  }
}
void Cpu::OnchipWrite16(uint32_t addr, uint16_t value) {
  uint16_t oldval;

  switch (addr) {
    case kIpra:
      IPRA = value;
      RecomputeInterrupt();
      break;
    case kIprb:
      IPRB = value;
      RecomputeInterrupt();
      break;
    case kIprc:
      IPRC = value;
      RecomputeInterrupt();
      break;
    case kIprd:
      IPRD = value;
      RecomputeInterrupt();
      break;
    case kIpre:
      IPRE = value;
      RecomputeInterrupt();
      break;

    case kTcr_0:
      oldval = TCR_0;
      TCR_0 = value;
      if (value & 0x100) {
        TCR_0 = value ^ (oldval & 0x100) ^ 0x100;
      } else {
        ResetInterruptPending(kTmu0Tuni0);
      }
      if (value & 0x20) {
        SetInterruptMask(kTmu0Tuni0);
      } else {
        ResetInterruptMask(kTmu0Tuni0);
      }
      tmu0->SetRate(tmu_divider[value & 7] * pclk_rate);
      break;

    case kTcr_1:
      oldval = TCR_1;
      TCR_1 = value;
      if (value & 0x100) {
        TCR_1 = value ^ (oldval & 0x100) ^ 0x100;
      } else {
        ResetInterruptPending(kTmu1Tuni1);
      }
      if (value & 0x20) {
        SetInterruptMask(kTmu1Tuni1);
      } else {
        ResetInterruptMask(kTmu1Tuni1);
      }
      tmu1->SetRate(tmu_divider[value & 7] * pclk_rate);
      break;

    case kTcr_2:
      oldval = TCR_2;
      TCR_2 = value;
      if (value & 0x100) {
        TCR_2 = value ^ (oldval & 0x100) ^ 0x100;
      } else {
        ResetInterruptPending(kTmu2Tuni2);
      }
      if (value & 0x20) {
        SetInterruptMask(kTmu2Tuni2);
      } else {
        ResetInterruptMask(kTmu2Tuni2);
      }
      tmu2->SetRate(tmu_divider[value & 7] * pclk_rate);
      break;

    default:
      OnchipRef16(addr) = value;
      break;
  }
}

void Cpu::OnchipWrite32(uint32_t addr, uint32_t value) {
  switch (addr) {
    case kTcnt_0:
      tmu0->SetCount(value + 1);
      break;
    case kTcnt_1:
      tmu1->SetCount(value + 1);
      break;
    case kTcnt_2:
      tmu2->SetCount(value + 1);
      break;

    case kChcr_0:
      CHCR_0 = value;
      if ((DMAOR & 1) && (CHCR_0 & 1)) {
        uint32_t dst = DAR_0;
        uint32_t src = SAR_0;
        uint32_t size = DMATCR_0;

        for (uint32_t i = 0; i < size; i++) {
          uint8_t byte = Read8(src);
          Write8(dst, byte);
          if ((CHCR_0 & 0x0000c000) == 0x00004000) dst++;
          if ((CHCR_0 & 0x00003000) == 0x00001000) src++;
        }

        dma0->SetCount(size * 15 * 2);
        Insert(dma0);
      }
      break;

    default:
      OnchipRef32(addr) = value;
      break;
  }
}
}  // namespace sh3