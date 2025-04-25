
#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "memory.h"

namespace sh3 {
const uint32_t kLookupShift = 10;
const uint32_t kLookupSize = 0x1000000000 >> kLookupShift;

using ReadHandler8 = std::function<uint8_t(uint32_t)>;
using ReadHandler16 = std::function<uint16_t(uint32_t)>;
using ReadHandler32 = std::function<uint32_t(uint32_t)>;
using WriteHandler8 = std::function<void(uint32_t, uint8_t)>;
using WriteHandler16 = std::function<void(uint32_t, uint16_t)>;
using WriteHandler32 = std::function<void(uint32_t, uint32_t)>;

struct MemHandler {
  ReadHandler8 read8;
  ReadHandler16 read16;
  ReadHandler32 read32;
  WriteHandler8 write8;
  WriteHandler16 write16;
  WriteHandler32 write32;
};

struct Map {
  uint32_t addr;
  uint32_t size;
  MemHandler mem_handler;

  Map(uint32_t a, uint32_t s, MemHandler& h)
      : addr(a), size(s), mem_handler(h) {}
};

enum MemoryRegionType : uint8_t { kCached = 0x40, kMmu = 0x80 };
enum class MemoryAccessType : uint32_t { kRead, kWrite };

struct Itlb {
  static const size_t kEntries = 4;

  uint32_t vpn;
  uint32_t ppn;
  uint32_t asid;
  uint32_t mask;
  uint32_t flags;
};

struct Utlb {
  static const size_t kEntries = 64;

  uint32_t vpn;
  uint32_t mask;
  uint32_t asid;
  uint32_t ppn;
  uint32_t flags;
  uint32_t pcmcia;
};

class Mmu {
 public:
  Mmu();

  void Init(std::vector<Map>& map);

  void LdTlb();

  uint8_t Read8(uint32_t adr);
  uint16_t Read16(uint32_t adr);
  uint32_t Read32(uint32_t adr);

  void Write8(uint32_t adr, uint8_t v);
  void Write16(uint32_t adr, uint16_t v);
  void Write32(uint32_t adr, uint32_t v);

 private:
  std::vector<MemHandler> memHandlers;
  uint8_t mem_regions_priv[kLookupSize];
  uint8_t mem_regions_user[kLookupSize];
  uint8_t* mem_regions;

  Itlb itlb[Itlb::kEntries];
  Utlb utlb[Utlb::kEntries];

  void SetPrivMemoryRegion(uint32_t region, uint32_t addr, uint32_t size);

  template <MemoryAccessType type, typename T>
  void MemAccess(uint32_t addr, T& value);
};
}  // namespace sh3