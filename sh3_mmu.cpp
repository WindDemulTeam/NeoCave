
#include "sh3_mmu.h"

#include <cstring>
#include <iostream>

#include "sh3.h"

namespace sh3 {
Mmu::Mmu() {
  std::memset(itlb, 0, sizeof(itlb));
  std::memset(utlb, 0, sizeof(utlb));
}

void Mmu::Init(std::vector<Map>& map) {
  std::memset(mem_regions_priv, 0xffffffff, sizeof(mem_regions_priv));
  std::memset(mem_regions_user, 0xffffffff, sizeof(mem_regions_user));

  for (auto m : map) {
    SetPrivMemoryRegion(static_cast<uint32_t>(memHandlers.size()), m.addr,
                        m.size);
    memHandlers.push_back(m.mem_handler);
  }
  mem_regions = mem_regions_priv;
}

void Mmu::SetPrivMemoryRegion(uint32_t region, uint32_t addr, uint32_t size) {
  if (addr < 0x10000000) {
    for (uint32_t mirror = 0; mirror < 0xE0000000; mirror += 0x20000000) {
      uint32_t start = addr + mirror;
      uint32_t end = start + size;

      for (uint32_t i = start; i < end; i += (1 << kLookupShift)) {
        if (i < 0x80000000) {
          mem_regions_priv[i >> kLookupShift] =
              region | MemoryRegionType::kCached | MemoryRegionType::kMmu;
        } else if (i < 0xA0000000) {
          mem_regions_priv[i >> kLookupShift] =
              region | MemoryRegionType::kCached;
        } else if (i < 0xC0000000) {
          mem_regions_priv[i >> kLookupShift] = region;
        } else if (i < 0xE0000000) {
          mem_regions_priv[i >> kLookupShift] =
              region | MemoryRegionType::kCached | MemoryRegionType::kMmu;
        }
      }
    }
  } else {
    uint64_t start = addr;
    uint64_t end = start + size;
    for (uint64_t i = start; i < end; i += (1 << kLookupShift)) {
      mem_regions_priv[i >> kLookupShift] = region;
    }
  }
}

template <MemoryAccessType type, typename T>
void Mmu::MemAccess(uint32_t addr, T& value) {
  uint8_t region = mem_regions[addr >> kLookupShift];

  if (region != 0xff) {
    auto& memHandler = memHandlers[region & 0x3f];
    if constexpr (type == MemoryAccessType::kRead) {
      if constexpr (sizeof(T) == 1) {
        if (memHandler.read8) {
          value = memHandler.read8(addr);
          return;
        }
      } else if constexpr (sizeof(T) == 2) {
        if (memHandler.read16) {
          value = memHandler.read16(addr);
          return;
        }
      } else if constexpr (sizeof(T) == 4) {
        if (memHandler.read32) {
          value = memHandler.read32(addr);
          return;
        }
      }
    } else {
      if constexpr (sizeof(T) == 1) {
        if (memHandler.write8) {
          memHandler.write8(addr, value);
          return;
        }
      } else if constexpr (sizeof(T) == 2) {
        if (memHandler.write16) {
          memHandler.write16(addr, value);
          return;
        }
      } else if constexpr (sizeof(T) == 4) {
        if (memHandler.write32) {
          memHandler.write32(addr, value);
          return;
        }
      }
    }
  }
  if constexpr (type == MemoryAccessType::kRead) {
    std::cout << "sh4 unimplemented read" << std::dec << sizeof(T) * 8
              << " : adr = " << std::hex << addr << std::endl;
    value = -1;
  } else {
    std::cout << "sh4 unimplemented write" << std::dec << sizeof(T) * 8
              << " : adr = " << std::hex << addr << ", val = " << value
              << std::endl;
  }
}

uint8_t Mmu::Read8(uint32_t addr) {
  uint8_t value;
  MemAccess<MemoryAccessType::kRead, uint8_t>(addr, value);
  return value;
}

uint16_t Mmu::Read16(uint32_t addr) {
  uint16_t value;
  MemAccess<MemoryAccessType::kRead, uint16_t>(addr, value);
  return value;
}

uint32_t Mmu::Read32(uint32_t addr) {
  uint32_t value;
  MemAccess<MemoryAccessType::kRead, uint32_t>(addr, value);
  return value;
}

void Mmu::Write8(uint32_t addr, uint8_t value) {
  MemAccess<MemoryAccessType::kWrite, uint8_t>(addr, value);
}

void Mmu::Write16(uint32_t addr, uint16_t value) {
  MemAccess<MemoryAccessType::kWrite, uint16_t>(addr, value);
}

void Mmu::Write32(uint32_t addr, uint32_t value) {
  MemAccess<MemoryAccessType::kWrite, uint32_t>(addr, value);
}

void Mmu::LdTlb() {}
}  // namespace sh3