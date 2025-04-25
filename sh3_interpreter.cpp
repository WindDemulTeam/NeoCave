
#include "sh3_interpreter.h"

#include <cfenv>
#include <cmath>
#include <iostream>

#include "sh3.h"

namespace sh3 {
Interpreter::Interpreter(Cpu* c) : cpu(c), delay_slot_(false), branch(false) {
  for (size_t i = 0; i < std::size(opTable); i++) {
    opTable[i] = &Interpreter::Unknown;
  }

  for (size_t i = 0; i < std::size(opTemplate); i++) {
    auto& op = opTemplate[i];
    for (size_t j = 0; j < std::size(opTable); j++) {
      if ((j & op.mask) == op.opcode) {
        opTable[j] = op.op;
      }
    }
  }
}

void Interpreter::Init() {}

void Interpreter::Run(int32_t& icount) {
  while (true) {
    branch = false;
    cpu->state.npc = cpu->state.pc + 2;
    icount -= Step(cpu->state.pc);
    cpu->state.pc = cpu->state.npc;
    if (branch && icount <= 0) {
      break;
    }
  }
}

uint32_t Interpreter::Step(uint32_t pc) {
  uint32_t code = cpu->Read16(pc);
  return (this->*opTable[code])(code);
}

void Interpreter::IsPrivilege() {}
void Interpreter::IsSlotIllegal() {}

uint32_t Interpreter::Unknown(uint32_t code) {
  std::cout << "Unknown opcode = " << std::hex << cpu->state.pc
            << ", pc = " << cpu->state.pc << std::endl;

  return 1;
}

uint32_t Interpreter::Add(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] += cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Addc(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t tmp0 = cpu->state.r[n];
  uint32_t tmp1 = cpu->state.r[n] + cpu->state.r[m];
  cpu->state.r[n] = tmp1 + cpu->state.sr.t;
  cpu->state.sr.t = tmp0 > tmp1;
  if (tmp1 > cpu->state.r[n]) cpu->state.sr.t = 1;

  return 1;
}

uint32_t Interpreter::Addi(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] += (uint32_t)(int32_t)(int8_t)i;

  return 1;
}

uint32_t Interpreter::Addv(uint32_t code) {
  uint32_t ans;
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t dest = (cpu->state.r[n] >> 31) & 1;
  uint32_t src = (cpu->state.r[m] >> 31) & 1;

  src += dest;
  cpu->state.r[n] += cpu->state.r[m];

  ans = (cpu->state.r[n] >> 31) & 1;
  ans += dest;

  if ((src == 0) || (src == 2))
    cpu->state.sr.t = (ans == 1);
  else
    cpu->state.sr.t = 0;

  return 1;
}

uint32_t Interpreter::Sub(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] -= cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Subc(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t tmp0 = cpu->state.r[n];
  uint32_t tmp1 = cpu->state.r[n] - cpu->state.r[m];
  cpu->state.r[n] = tmp1 - cpu->state.sr.t;
  cpu->state.sr.t = (tmp0 < tmp1);
  if (tmp1 < cpu->state.r[n]) cpu->state.sr.t = 1;

  return 1;
}

uint32_t Interpreter::Subv(uint32_t code) {
  uint32_t ans;
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t dest = (cpu->state.r[n] >> 31) & 1;
  uint32_t src = (cpu->state.r[m] >> 31) & 1;

  src += dest;
  cpu->state.r[n] -= cpu->state.r[m];

  ans = (cpu->state.r[n] >> 31) & 1;
  ans += dest;

  if (src == 1)
    cpu->state.sr.t = (ans == 1);
  else
    cpu->state.sr.t = 0;

  return 1;
}

uint32_t Interpreter::And(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] &= cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Andi(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  cpu->state.r[0] &= i;

  return 1;
}

uint32_t Interpreter::Andm(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  uint32_t uAddress = cpu->state.gbr + cpu->state.r[0];
  uint8_t temp = cpu->Read8(uAddress);
  cpu->Write8(uAddress, temp & i);

  return 5;
}

uint32_t Interpreter::Or(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] |= cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Ori(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  cpu->state.r[0] |= i;

  return 1;
}

uint32_t Interpreter::Orm(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  uint32_t uAddress = cpu->state.gbr + cpu->state.r[0];
  uint8_t temp = cpu->Read8(uAddress);
  cpu->Write8(uAddress, temp | i);

  return 5;
}

uint32_t Interpreter::Xor(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] ^= cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Xori(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  cpu->state.r[0] ^= i;

  return 1;
}

uint32_t Interpreter::Xorm(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  uint32_t uAddress = cpu->state.gbr + cpu->state.r[0];
  uint8_t temp = cpu->Read8(uAddress);
  cpu->Write8(uAddress, temp ^ i);

  return 5;
}

uint32_t Interpreter::Tst(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = ((cpu->state.r[n] & cpu->state.r[m]) == 0);

  return 1;
}

uint32_t Interpreter::Tsti(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  cpu->state.sr.t = ((cpu->state.r[0] & i) == 0);

  return 1;
}

uint32_t Interpreter::Tstm(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  uint32_t uAddress = cpu->state.gbr + cpu->state.r[0];
  uint8_t temp = cpu->Read8(uAddress);
  cpu->state.sr.t = ((temp & i) == 0);

  return 5;
}

uint32_t Interpreter::Tas(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.r[n];
  uint8_t temp = cpu->Read8(uAddress);
  cpu->Write8(uAddress, temp | 0x80);
  cpu->state.sr.t = (temp == 0);

  return 1;
}

uint32_t Interpreter::Neg(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = 0 - cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Negc(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t tmp = 0 - cpu->state.r[m];
  cpu->state.r[n] = tmp - cpu->state.sr.t;
  cpu->state.sr.t = 0 < tmp;
  if (tmp < cpu->state.r[n]) cpu->state.sr.t = 1;

  return 1;
}

uint32_t Interpreter::Not(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = ~cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Nop(uint32_t code) { return 1; }

uint32_t Interpreter::Rotcl(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t t = (cpu->state.r[n] >> 31) & 1;

  cpu->state.r[n] = ((cpu->state.r[n] & 0x7fffffff) << 1) |
                    ((cpu->state.sr.t & 0x00000001) << 0);

  cpu->state.sr.t = t;

  return 1;
}

uint32_t Interpreter::Rotl(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = (cpu->state.r[n] >> 31) & 1;

  cpu->state.r[n] = ((cpu->state.r[n] & 0x7fffffff) << 1) |
                    ((cpu->state.r[n] & 0x80000000) >> 31);

  return 1;
}

uint32_t Interpreter::Rotcr(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t t = (cpu->state.r[n] >> 0) & 1;

  cpu->state.r[n] = ((cpu->state.sr.t & 0x00000001) << 31) |
                    ((cpu->state.r[n] & 0xfffffffe) >> 1);

  cpu->state.sr.t = t;

  return 1;
}

uint32_t Interpreter::Rotr(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = (cpu->state.r[n] >> 0) & 1;

  cpu->state.r[n] = ((cpu->state.r[n] & 0x00000001) << 31) |
                    ((cpu->state.r[n] & 0xfffffffe) >> 1);

  return 1;
}

uint32_t Interpreter::Dt(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = (--cpu->state.r[n] == 0);

  return 1;
}

uint32_t Interpreter::Extsb(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = (uint32_t)(int32_t)(int8_t)cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Extsw(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = (uint32_t)(int32_t)(int16_t)cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Extub(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = (uint32_t)(uint8_t)cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Extuw(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = (uint32_t)(uint16_t)cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Bra(uint32_t code) {
  IsSlotIllegal();

  uint32_t d = ((int32_t)(code << 20) >> 20);

  cpu->state.npc = cpu->state.pc + (d << 1) + 4;

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;
  branch = true;

  return 2 + cycle;
}

uint32_t Interpreter::Bsr(uint32_t code) {
  IsSlotIllegal();

  uint32_t d = ((int32_t)(code << 20) >> 20);

  cpu->state.pr = cpu->state.pc + 4;
  cpu->state.npc = cpu->state.pc + (d << 1) + 4;

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;

  branch = true;

  return 2 + cycle;
}

uint32_t Interpreter::Braf(uint32_t code) {
  IsSlotIllegal();

  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.npc = cpu->state.pc + cpu->state.r[n] + 4;

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;

  branch = true;

  return 2 + cycle;
}

uint32_t Interpreter::Bsrf(uint32_t code) {
  IsSlotIllegal();

  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.pr = cpu->state.pc + 4;
  cpu->state.npc = cpu->state.pc + cpu->state.r[n] + 4;
  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;
  branch = true;

  return 2 + cycle;
}

uint32_t Interpreter::Jmp(uint32_t code) {
  IsSlotIllegal();

  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.npc = cpu->state.r[n];

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;

  branch = true;

  return 2 + cycle;
}

uint32_t Interpreter::Jsr(uint32_t code) {
  IsSlotIllegal();

  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.pr = cpu->state.pc + 4;
  cpu->state.npc = cpu->state.r[n];

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;

  branch = true;

  return 2 + cycle;
}

uint32_t Interpreter::Bf(uint32_t code) {
  IsSlotIllegal();

  if (cpu->state.sr.t == 0) {
    uint32_t d = ((code >> 0) & 0xff);
    cpu->state.npc = cpu->state.pc + (((int32_t)(int8_t)d) << 1) + 4;
  } else {
    cpu->state.npc = cpu->state.pc + 2;
  }

  branch = true;

  return 2;
}

uint32_t Interpreter::Bfs(uint32_t code) {
  IsSlotIllegal();

  if (cpu->state.sr.t == 0) {
    uint32_t d = ((code >> 0) & 0xff);
    cpu->state.npc = cpu->state.pc + (((int32_t)(int8_t)d) << 1) + 4;
  } else {
    cpu->state.npc = cpu->state.pc + 4;
  }

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;

  branch = true;

  return 2 + cycle;
}

uint32_t Interpreter::Bt(uint32_t code) {
  IsSlotIllegal();

  if (cpu->state.sr.t != 0) {
    uint32_t d = ((code >> 0) & 0xff);
    cpu->state.npc = cpu->state.pc + (((int32_t)(int8_t)d) << 1) + 4;
  } else {
    cpu->state.npc = cpu->state.pc + 2;
  }

  branch = true;

  return 2;
}

uint32_t Interpreter::Bts(uint32_t code) {
  IsSlotIllegal();

  if (cpu->state.sr.t != 0) {
    uint32_t d = ((code >> 0) & 0xff);
    cpu->state.npc = cpu->state.pc + (((int32_t)(int8_t)d) << 1) + 4;
  } else {
    cpu->state.npc = cpu->state.pc + 4;
  }

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;

  branch = true;

  return 2 + cycle;
}

uint32_t Interpreter::Rts(uint32_t code) {
  IsSlotIllegal();

  cpu->state.npc = cpu->state.pr;

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;

  branch = true;

  return 1 + cycle;
}

uint32_t Interpreter::Rte(uint32_t code) {
  IsSlotIllegal();
  IsPrivilege();

  uint32_t ssr = cpu->state.ssr;
  uint32_t rb = cpu->state.sr.rb;

  cpu->state.sr.all = (cpu->state.sr.all & 0x40000000) | (ssr & 0x300083f3);

  if (cpu->state.sr.rb != rb) {
    cpu->SwapBank();
  }

  cpu->state.npc = cpu->state.spc;

  delay_slot_ = true;
  uint32_t cycle = Step(cpu->state.pc + 2);
  delay_slot_ = false;
  cpu->state.sr.md = (ssr & 0x40000000) != 0;

  cpu->RecomputeImask();

  branch = true;

  return 1 + cycle;
}

uint32_t Interpreter::Trapa(uint32_t code) {
  IsSlotIllegal();

  uint32_t i = ((code >> 0) & 0xff);

  cpu->OnchipRef32(kTra) = i << 2;
  cpu->OnchipRef32(kExpevt) = 0x0160;

  if (!cpu->state.sr.rb) {
    cpu->SwapBank();
  }

  cpu->state.ssr = cpu->state.sr.all;
  cpu->state.sr.md = 1;
  cpu->state.sr.rb = 1;
  cpu->state.sr.bl = 1;
  cpu->state.spc = cpu->state.pc + 2;
  cpu->state.npc = cpu->state.vbr + 0x0100;
  cpu->RecomputeImask();

  branch = true;
  return 1;
}

uint32_t Interpreter::Cmpim(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);

  cpu->state.sr.t = (cpu->state.r[0] == (uint32_t)(int32_t)(int8_t)i);

  return 1;
}

uint32_t Interpreter::Cmpeq(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = (cpu->state.r[n] == cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Cmpge(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = ((int32_t)cpu->state.r[n] >= (int32_t)cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Cmpgt(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = ((int32_t)cpu->state.r[n] > (int32_t)cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Cmphs(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = ((uint32_t)cpu->state.r[n] >= (uint32_t)cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Cmphi(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = ((uint32_t)cpu->state.r[n] > (uint32_t)cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Cmppz(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = ((int32_t)cpu->state.r[n] >= 0);

  return 1;
}

uint32_t Interpreter::Cmppl(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = ((int32_t)cpu->state.r[n] > 0);

  return 1;
}

uint32_t Interpreter::Cmpstr(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t tmp = cpu->state.r[n] ^ cpu->state.r[m];
  uint32_t HH = (tmp >> 24) & 0x000000FF;
  uint32_t HL = (tmp >> 16) & 0x000000FF;
  uint32_t LH = (tmp >> 8) & 0x000000FF;
  uint32_t LL = (tmp >> 0) & 0x000000FF;

  cpu->state.sr.t = ((HH && HL && LH && LL) == 0);

  return 1;
}

uint32_t Interpreter::Div0s(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.q = (cpu->state.r[n] >> 31) & 1;
  cpu->state.sr.m = (cpu->state.r[m] >> 31) & 1;
  cpu->state.sr.t = cpu->state.sr.q ^ cpu->state.sr.m;

  return 1;
}

uint32_t Interpreter::Div0u(uint32_t code) {
  cpu->state.sr.q = cpu->state.sr.m = cpu->state.sr.t = 0;

  return 1;
}

uint32_t Interpreter::Div1(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t op2 = cpu->state.r[n];
  uint32_t carry = op2 >> 31;
  op2 = (op2 << 1) | cpu->state.sr.t;
  if (cpu->state.sr.q == cpu->state.sr.m) {
    if (cpu->state.r[m] > op2) carry ^= 1;
    op2 -= cpu->state.r[m];
  } else {
    uint32_t top2 = op2;
    op2 += cpu->state.r[m];
    if (op2 < top2) carry ^= 1;
  }
  cpu->state.sr.q = cpu->state.sr.m ^ carry;
  cpu->state.sr.t = ~carry;
  cpu->state.r[n] = op2;

  return 1;
}

uint32_t Interpreter::Dmuls(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  int64_t mult =
      (int64_t)(int32_t)cpu->state.r[n] * (int64_t)(int32_t)cpu->state.r[m];

  cpu->state.macl = (uint32_t)((mult >> 0) & 0xffffffff);
  cpu->state.mach = (uint32_t)((mult >> 32) & 0xffffffff);

  return 5;
}

uint32_t Interpreter::Dmulu(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint64_t mult =
      (uint64_t)(uint32_t)cpu->state.r[n] * (uint64_t)(uint32_t)cpu->state.r[m];

  cpu->state.macl = (uint32_t)((mult >> 0) & 0xffffffff);
  cpu->state.mach = (uint32_t)((mult >> 32) & 0xffffffff);

  return 5;
}

uint32_t Interpreter::Macl(uint32_t code) { return 3; }

uint32_t Interpreter::Macw(uint32_t code) { return 3; }

uint32_t Interpreter::Mova(uint32_t code) {
  IsSlotIllegal();

  uint32_t d = ((code >> 0) & 0xff);

  cpu->state.r[0] = ((cpu->state.pc & 0xfffffffc) + (d << 2) + 4);

  return 1;
}

uint32_t Interpreter::Mov(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = cpu->state.r[m];

  return 1;
}

uint32_t Interpreter::Movi(uint32_t code) {
  uint32_t i = ((code >> 0) & 0xff);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = (uint32_t)(int32_t)(int8_t)i;

  return 1;
}

uint32_t Interpreter::Movli(uint32_t code) {
  IsSlotIllegal();

  uint32_t d = ((code >> 0) & 0xff);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = (cpu->state.pc & 0xfffffffc) + (d << 2) + 4;
  uint32_t temp = cpu->Read32(uAddress);
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movwi(uint32_t code) {
  IsSlotIllegal();

  uint32_t d = ((code >> 0) & 0xff);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.pc + (d << 1) + 4;
  int32_t temp = static_cast<int16_t>(cpu->Read16(uAddress));
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movll(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t temp = cpu->Read32(cpu->state.r[m]);
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movwl(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  int32_t temp = static_cast<int16_t>(cpu->Read16(cpu->state.r[m]));
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movbl(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  int32_t temp = static_cast<int8_t>(cpu->Read8(cpu->state.r[m]));
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movll0(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.r[m] + cpu->state.r[0];
  uint32_t temp = cpu->Read32(uAddress);
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movwl0(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.r[m] + cpu->state.r[0];
  int32_t temp = static_cast<int16_t>(cpu->Read16(uAddress));
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movbl0(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.r[m] + cpu->state.r[0];
  int32_t temp = static_cast<int8_t>(cpu->Read8(uAddress));
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movll4(uint32_t code) {
  uint32_t d = ((code >> 0) & 0x0f);
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.r[m] + (d << 2);
  uint32_t temp = cpu->Read32(uAddress);
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movwl4(uint32_t code) {
  uint32_t d = ((code >> 0) & 0x0f);
  uint32_t m = ((code >> 4) & 0x0f);

  uint32_t uAddress = cpu->state.r[m] + (d << 1);
  int32_t temp = static_cast<int16_t>(cpu->Read16(uAddress));
  cpu->state.r[0] = temp;

  return 1;
}

uint32_t Interpreter::Movbl4(uint32_t code) {
  uint32_t d = ((code >> 0) & 0x0f);
  uint32_t m = ((code >> 4) & 0x0f);

  uint32_t uAddress = cpu->state.r[m] + (d << 0);
  int32_t temp = static_cast<int8_t>(cpu->Read8(uAddress));
  cpu->state.r[0] = temp;

  return 1;
}

uint32_t Interpreter::Movllg(uint32_t code) {
  uint32_t d = ((code >> 0) & 0xff);

  uint32_t uAddress = cpu->state.gbr + (d << 2);
  uint32_t temp = cpu->Read32(uAddress);
  cpu->state.r[0] = temp;

  return 1;
}

uint32_t Interpreter::Movwlg(uint32_t code) {
  uint32_t d = ((code >> 0) & 0xff);

  uint32_t uAddress = cpu->state.gbr + (d << 1);
  int32_t temp = static_cast<int16_t>(cpu->Read16(uAddress));
  cpu->state.r[0] = temp;

  return 1;
}

uint32_t Interpreter::Movblg(uint32_t code) {
  uint32_t d = ((code >> 0) & 0xff);

  uint32_t uAddress = cpu->state.gbr + (d << 0);
  int32_t temp = static_cast<int8_t>(cpu->Read8(uAddress));
  cpu->state.r[0] = temp;

  return 1;
}

uint32_t Interpreter::Movlp(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.r[m];
  uint32_t temp = cpu->Read32(uAddress);
  if (n != m) cpu->state.r[m] += 4;
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movwp(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.r[m];
  int32_t temp = static_cast<int16_t>(cpu->Read16(uAddress));
  if (n != m) cpu->state.r[m] += 2;
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movbp(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  uint32_t uAddress = cpu->state.r[m];
  int32_t temp = static_cast<int8_t>(cpu->Read8(uAddress));
  if (n != m) cpu->state.r[m] += 1;
  cpu->state.r[n] = temp;

  return 1;
}

uint32_t Interpreter::Movls(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write32(cpu->state.r[n], cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Movws(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write16(cpu->state.r[n], cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Movbs(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write8(cpu->state.r[n], cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Movls0(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write32(cpu->state.r[n] + cpu->state.r[0], cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Movws0(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write16(cpu->state.r[n] + cpu->state.r[0], cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Movbs0(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write8(cpu->state.r[n] + cpu->state.r[0], cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Movls4(uint32_t code) {
  uint32_t d = ((code >> 0) & 0x0f);
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write32(cpu->state.r[n] + (d << 2), cpu->state.r[m]);

  return 1;
}

uint32_t Interpreter::Movws4(uint32_t code) {
  uint32_t d = ((code >> 0) & 0x0f);
  uint32_t n = ((code >> 4) & 0x0f);

  cpu->Write16(cpu->state.r[n] + (d << 1), cpu->state.r[0]);

  return 1;
}

uint32_t Interpreter::Movbs4(uint32_t code) {
  uint32_t d = ((code >> 0) & 0x0f);
  uint32_t n = ((code >> 4) & 0x0f);

  cpu->Write8(cpu->state.r[n] + (d << 0), cpu->state.r[0]);

  return 1;
}

uint32_t Interpreter::Movlsg(uint32_t code) {
  uint32_t d = ((code >> 0) & 0xff);

  cpu->Write32(cpu->state.gbr + (d << 2), cpu->state.r[0]);

  return 1;
}

uint32_t Interpreter::Movwsg(uint32_t code) {
  uint32_t d = ((code >> 0) & 0xff);

  cpu->Write16(cpu->state.gbr + (d << 1), cpu->state.r[0]);

  return 1;
}

uint32_t Interpreter::Movbsg(uint32_t code) {
  uint32_t d = ((code >> 0) & 0xff);

  cpu->Write8(cpu->state.gbr + (d << 0), cpu->state.r[0]);

  return 1;
}

uint32_t Interpreter::Movlm(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write32(cpu->state.r[n] - 4, cpu->state.r[m]);
  cpu->state.r[n] -= 4;

  return 1;
}

uint32_t Interpreter::Movwm(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write16(cpu->state.r[n] - 2, cpu->state.r[m]);
  cpu->state.r[n] -= 2;

  return 1;
}

uint32_t Interpreter::Movbm(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write8(cpu->state.r[n] - 1, cpu->state.r[m]);
  cpu->state.r[n] -= 1;

  return 1;
}

uint32_t Interpreter::Movcal(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->Write32(cpu->state.r[n], cpu->state.r[0]);

  return 1;
}

uint32_t Interpreter::Mull(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.macl = cpu->state.r[n] * cpu->state.r[m];

  return 5;
}

uint32_t Interpreter::Mulsu(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.macl =
      (uint32_t)(uint16_t)cpu->state.r[n] * (uint32_t)(uint16_t)cpu->state.r[m];

  return 5;
}

uint32_t Interpreter::Mulsw(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.macl =
      (int32_t)(int16_t)cpu->state.r[n] * (int32_t)(int16_t)cpu->state.r[m];

  return 5;
}

uint32_t Interpreter::Ldtlb(uint32_t code) {
  cpu->LdTlb();
  return 1;
}

uint32_t Interpreter::Ocbi(uint32_t code) { return 1; }

uint32_t Interpreter::Ocbp(uint32_t code) { return 1; }

uint32_t Interpreter::Ocbwb(uint32_t code) { return 1; }

uint32_t Interpreter::Pref(uint32_t code) { return 1; }

uint32_t Interpreter::Shad(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  if ((cpu->state.r[m] & 0x80000000) == 0) {
    cpu->state.r[n] <<= (cpu->state.r[m] & 0x1f);
  } else if ((cpu->state.r[m] & 0x1f) == 0) {
    cpu->state.r[n] = (int32_t)cpu->state.r[n] >> 31;
  } else {
    cpu->state.r[n] =
        (int32_t)cpu->state.r[n] >> ((~cpu->state.r[m] & 0x1f) + 1);
  }

  return 1;
}

uint32_t Interpreter::Shld(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  if ((cpu->state.r[m] & 0x80000000) == 0) {
    cpu->state.r[n] <<= (cpu->state.r[m] & 0x1f);
  } else if ((cpu->state.r[m] & 0x1f) == 0) {
    cpu->state.r[n] = 0;
  } else {
    cpu->state.r[n] >>= ((~cpu->state.r[m] & 0x1f) + 1);
  }

  return 1;
}

uint32_t Interpreter::Shll(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = (cpu->state.r[n] >> 31) & 1;
  cpu->state.r[n] <<= 1;

  return 1;
}

uint32_t Interpreter::Shal(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = (cpu->state.r[n] >> 31) & 1;
  cpu->state.r[n] <<= 1;

  return 1;
}

uint32_t Interpreter::Shll16(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] <<= 16;

  return 1;
}

uint32_t Interpreter::Shll8(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] <<= 8;

  return 1;
}

uint32_t Interpreter::Shll2(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] <<= 2;

  return 1;
}

uint32_t Interpreter::Shlr16(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] >>= 16;

  return 1;
}

uint32_t Interpreter::Shlr8(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] >>= 8;

  return 1;
}

uint32_t Interpreter::Shlr2(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] >>= 2;

  return 1;
}

uint32_t Interpreter::Shlr(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = (cpu->state.r[n] >> 0) & 1;
  cpu->state.r[n] >>= 1;

  return 1;
}

uint32_t Interpreter::Shar(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.sr.t = cpu->state.r[n] & 1;
  cpu->state.r[n] = (int32_t)cpu->state.r[n] >> 1;

  return 1;
}

uint32_t Interpreter::Sleep(uint32_t code) {
  IsSlotIllegal();

  branch = true;

  return 1;
}

uint32_t Interpreter::Clrmac(uint32_t code) {
  cpu->state.macl = cpu->state.mach = 0;
  return 1;
}

uint32_t Interpreter::Clrs(uint32_t code) {
  cpu->state.sr.s = 0;
  return 1;
}

uint32_t Interpreter::Clrt(uint32_t code) {
  cpu->state.sr.t = 0;
  return 1;
}

uint32_t Interpreter::Sets(uint32_t code) {
  cpu->state.sr.s = 1;
  return 1;
}

uint32_t Interpreter::Sett(uint32_t code) {
  cpu->state.sr.t = 1;
  return 1;
}

uint32_t Interpreter::Movt(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = cpu->state.sr.t;

  return 1;
}

uint32_t Interpreter::Ldc(uint32_t m) { return cpu->state.r[m]; }

uint32_t Interpreter::Ldcgbr(uint32_t code) {
  uint32_t m = ((code >> 8) & 0x0f);
  cpu->state.gbr = Ldc(m);

  return 2;
}

uint32_t Interpreter::Ldspr(uint32_t code) {
  uint32_t m = ((code >> 8) & 0x0f);
  cpu->state.pr = Ldc(m);

  return 2;
}

uint32_t Interpreter::Ldcspc(uint32_t code) {
  IsPrivilege();

  uint32_t m = ((code >> 8) & 0x0f);
  cpu->state.spc = Ldc(m);

  return 2;
}

uint32_t Interpreter::Ldcsr(uint32_t code) {
  IsPrivilege();
  IsSlotIllegal();

  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t rb = cpu->state.sr.rb;
  cpu->state.sr.all = Ldc(m) & 0x700083f3;

  if (cpu->state.sr.rb != rb) {
    cpu->SwapBank();
  }
  cpu->RecomputeImask();

  return 4;
}

uint32_t Interpreter::Ldcssr(uint32_t code) {
  IsPrivilege();

  uint32_t m = ((code >> 8) & 0x0f);
  cpu->state.ssr = Ldc(m);

  return 2;
}

uint32_t Interpreter::Ldcvbr(uint32_t code) {
  IsPrivilege();

  uint32_t m = ((code >> 8) & 0x0f);
  cpu->state.vbr = Ldc(m);

  return 2;
}

uint32_t Interpreter::Ldsmach(uint32_t code) {
  uint32_t m = ((code >> 8) & 0x0f);
  cpu->state.mach = Ldc(m);

  return 1;
}

uint32_t Interpreter::Ldsmacl(uint32_t code) {
  uint32_t m = ((code >> 8) & 0x0f);
  cpu->state.macl = Ldc(m);

  return 2;
}

uint32_t Interpreter::Ldcrbank(uint32_t code) {
  IsPrivilege();

  uint32_t b = ((code >> 4) & 0x07) + 16;
  uint32_t m = ((code >> 8) & 0x0f);
  cpu->state.r[b] = Ldc(m);

  return 2;
}

uint32_t Interpreter::Ldcm(uint32_t m) {
  uint32_t uAddress = cpu->state.r[m];
  uint32_t temp = cpu->Read32(uAddress);
  return temp;
}

uint32_t Interpreter::Ldcmgbr(uint32_t code) {
  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);
  cpu->state.r[m] += 4;
  cpu->state.gbr = temp;

  return 2;
}

uint32_t Interpreter::Ldsmpr(uint32_t code) {
  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);
  cpu->state.r[m] += 4;
  cpu->state.pr = temp;

  return 2;
}

uint32_t Interpreter::Ldcmspc(uint32_t code) {
  IsPrivilege();

  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);
  cpu->state.r[m] += 4;
  cpu->state.spc = temp;

  return 2;
}

uint32_t Interpreter::Ldcmsr(uint32_t code) {
  IsPrivilege();
  IsSlotIllegal();

  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);

  cpu->state.r[m] += 4;

  uint32_t rb = cpu->state.sr.rb;
  cpu->state.sr.all = temp & 0x700083f3;

  if (cpu->state.sr.rb != rb) {
    cpu->SwapBank();
  }
  cpu->RecomputeImask();

  return 2;
}

uint32_t Interpreter::Ldcmssr(uint32_t code) {
  IsPrivilege();

  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);
  cpu->state.r[m] += 4;
  cpu->state.ssr = temp;

  return 2;
}

uint32_t Interpreter::Ldcmvbr(uint32_t code) {
  IsPrivilege();

  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);
  cpu->state.r[m] += 4;
  cpu->state.vbr = temp;

  return 2;
}

uint32_t Interpreter::Ldsmmach(uint32_t code) {
  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);
  cpu->state.r[m] += 4;
  cpu->state.mach = temp;

  return 2;
}

uint32_t Interpreter::Ldsmmacl(uint32_t code) {
  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);
  cpu->state.r[m] += 4;
  cpu->state.macl = temp;

  return 2;
}

uint32_t Interpreter::Ldcmrbank(uint32_t code) {
  IsPrivilege();

  uint32_t b = ((code >> 4) & 0x07) + 16;
  uint32_t m = ((code >> 8) & 0x0f);

  uint32_t temp = Ldcm(m);
  cpu->state.r[m] += 4;
  cpu->state.r[b] = temp;

  return 2;
}

void Interpreter::Stc(uint32_t n, uint32_t val) { cpu->state.r[n] = val; }

uint32_t Interpreter::Stcgbr(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.gbr);

  return 2;
}

uint32_t Interpreter::Stspr(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.pr);

  return 2;
}

uint32_t Interpreter::Stcspc(uint32_t code) {
  IsPrivilege();

  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.spc);

  return 2;
}

uint32_t Interpreter::Stcsr(uint32_t code) {
  IsPrivilege();

  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.sr.all);

  return 2;
}

uint32_t Interpreter::Stcssr(uint32_t code) {
  IsPrivilege();

  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.ssr);

  return 2;
}

uint32_t Interpreter::Stcvbr(uint32_t code) {
  IsPrivilege();

  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.vbr);

  return 2;
}

uint32_t Interpreter::Stcrbank(uint32_t code) {
  IsPrivilege();

  uint32_t b = ((code >> 4) & 0x07) + 16;
  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.r[b]);

  return 2;
}

uint32_t Interpreter::Stsmach(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.mach);

  return 2;
}

uint32_t Interpreter::Stsmacl(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);
  Stc(n, cpu->state.macl);

  return 2;
}

void Interpreter::Stcm(uint32_t n, uint32_t val) {
  uint32_t uAddress = cpu->state.r[n] - 4;
  cpu->Write32(uAddress, val);
  cpu->state.r[n] -= 4;
}

uint32_t Interpreter::Stcmgbr(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.gbr);

  return 2;
}

uint32_t Interpreter::Stsmpr(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.pr);

  return 2;
}

uint32_t Interpreter::Stcmspc(uint32_t code) {
  IsPrivilege();

  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.spc);

  return 2;
}

uint32_t Interpreter::Stcmsr(uint32_t code) {
  IsPrivilege();

  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.sr.all);

  return 2;
}

uint32_t Interpreter::Stcmssr(uint32_t code) {
  IsPrivilege();

  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.ssr);

  return 2;
}

uint32_t Interpreter::Stcmvbr(uint32_t code) {
  IsPrivilege();

  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.vbr);

  return 2;
}

uint32_t Interpreter::Stcmrbank(uint32_t code) {
  IsPrivilege();

  uint32_t b = ((code >> 4) & 0x07) + 16;
  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.r[b]);

  return 2;
}

uint32_t Interpreter::Stsmmach(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.mach);

  return 2;
}

uint32_t Interpreter::Stsmmacl(uint32_t code) {
  uint32_t n = ((code >> 8) & 0x0f);
  Stcm(n, cpu->state.macl);

  return 2;
}

uint32_t Interpreter::Xtrct(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = (cpu->state.r[n] >> 16) | (cpu->state.r[m] << 16);

  return 1;
}

uint32_t Interpreter::Swapb(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = ((cpu->state.r[m] & 0xffff0000) << 0) |
                    ((cpu->state.r[m] & 0x000000ff) << 8) |
                    ((cpu->state.r[m] & 0x0000ff00) >> 8);

  return 1;
}

uint32_t Interpreter::Swapw(uint32_t code) {
  uint32_t m = ((code >> 4) & 0x0f);
  uint32_t n = ((code >> 8) & 0x0f);

  cpu->state.r[n] = (cpu->state.r[m] >> 16) | (cpu->state.r[m] << 16);

  return 1;
}
}  // namespace sh3