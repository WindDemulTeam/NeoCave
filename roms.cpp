
#include "roms.h"

uint32_t GetRomHash(uint32_t crc) { return crc; }
uint32_t GetRomHash(uint32_t crc, const char *sha1) { return crc; }

#include "romsets.cxx"

#define DRIVER(NAME) &game_##NAME,

static const GameEntry *games[] = {
#include "cave3rddriv.cxx"
    nullptr};

const char *GamesList::GetGameField(uint32_t idx, EGameField field) const {
  if (idx >= count_) return nullptr;

  GameEntry **entry = (GameEntry **)head_;
  switch (field) {
    case eGameFieldName:
      return entry[idx]->name;
    case eGameFieldFullName:
      return entry[idx]->full_name;
    case eGameFieldCompany:
      return entry[idx]->company;
    case eGameFieldYear:
      return entry[idx]->year;
  }
  return NULL;
}

uint32_t GamesList::IndexOfName(const char *name) const {
  GameEntry **entry = (GameEntry **)head_;
  for (uint32_t i = 0; i < count_; i++) {
    if (std::strcmp(entry[i]->name, name) == 0) {
      return i;
    }
  }
  return (uint32_t)-1;
}

void GamesList::Init() {
  head_ = games;
  GameEntry **entry = (GameEntry **)head_;
  uint32_t i = 0;
  count_ = 0;
  while (entry[i++] != nullptr) {
    count_++;
  }
}

bool GamesList::Load(RomEntry *rom_entry, std::filesystem::path path,
                     bool open) {
  bool error = false;
  std::filesystem::path sub_path;
  while (!error && rom_entry->type != RomEntryEnd) {
    switch (rom_entry->type) {
      case RomEntryLoad:
      case RomEntryLoadXwordSwap:
        sub_path = path / rom_entry->name;
        if (std::filesystem::exists(sub_path)) {
          if (open) {
            rom_entry->file = new std::fstream(
                sub_path, std::ios_base::in | std::ios_base::binary);
            if (!rom_entry->file->is_open()) {
              error = true;
            }
          }
        } else {
          error = true;
        }
        break;
    }
    rom_entry++;
  }

  if (error) {
    return false;
  }

  return true;
}

bool GamesList::LoadGame(uint32_t idx, std::string path, bool open) {
  RomEntry *rom_entry;

  FreeGameRoms();

  if ((game = GetInfo(idx)) == nullptr) return false;

  // RTC9701_Init(eeprom_cave);
  // LoadRTC9701File(game->name);

  // rotate = game->rotate & 3;

  rom_entry = game->roms;
  if (!Load(rom_entry, path, open)) {
    return false;
  }

  if (open) {
    for (int i = 0; i < 0x21000; i++) {
      nand_buffer_[i] = GetGameRomValue(0x21000 + i);
    }

    // LoadNANDFile(game->name);
  }
  return true;
}

void GamesList::FreeGameRoms() {
  RomEntry *rom_entry;

  if (game == nullptr) return;

  rom_entry = game->roms;
  game = nullptr;

  while (rom_entry->type != RomEntryEnd) {
    switch (rom_entry->type) {
      case RomEntryLoad:
      case RomEntryLoadXwordSwap:
        if (rom_entry->file && rom_entry->file->is_open()) {
          rom_entry->file->close();
        }
        break;
    }
    rom_entry++;
  }
}

uint8_t GamesList::GetByte(std::fstream *file, uint32_t ofs, uint32_t rofs) {
  if ((nand_file_name_.size() != 0) && (ofs >= 0x21000) && (ofs < 0x42000)) {
    return nand_buffer_[ofs - 0x21000];
  } else {
    uint32_t addr = ofs & (~(sector_.size() - 1));
    if (cursectorloaded != addr) {
      cursectorloaded = addr;
      file->seekg(addr - rofs, std::ios::beg);
      file->read(reinterpret_cast<char *>(&sector_[0]), sector_.size());
    }
    return sector_[(ofs - rofs) & (sector_.size() - 1)];
  }
}

uint8_t GamesList::GetGameRomValue(uint32_t offset) {
  RomEntry *romEntry;
  romEntry = game->roms;
  while (romEntry->type != RomEntryEnd) {
    if (offset >= romEntry->offset &&
        offset < (romEntry->offset + romEntry->length)) {
      switch (romEntry->type) {
        case RomEntryLoad:
          return GetByte(romEntry->file, offset, romEntry->offset);
        case RomEntryLoadXwordSwap:
          return GetByte(romEntry->file, offset ^ 1, romEntry->offset);
      }
    }
    romEntry++;
  }
  return 0;
}

void GamesList::SetGameRomValue(uint32_t offset, uint8_t val) {
  if ((offset >= 0x21000) && (offset < 0x42000)) {
    nand_buffer_[offset - 0x21000] = val;
    return;
  }
}