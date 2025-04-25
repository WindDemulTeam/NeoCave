#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

enum EGameField {
  eGameFieldName = 0,
  eGameFieldFullName = 1,
  eGameFieldCompany = 2,
  eGameFieldYear = 3,
  eGameFieldMachine = 4
};

enum RomEntryType {
  RomEntryLoad,
  RomEntryLoadXwordSwap,
  RomEntryReload,
  RomEntryEnd
};

struct RomEntry {
  RomEntryType type;
  const char *name;
  uint32_t offset;
  uint32_t length;
  uint32_t dest;
  uint32_t width;
  uint32_t crc;
  std::fstream *file;
};

struct GameEntry {
  const char *name;
  const char *parent;
  const char *full_name;
  const char *company;
  const char *year;
  uint32_t mode;
  RomEntry *roms;
};

#define ROM_START(name) static RomEntry roms_##name[] = {
#define ROM_LOAD(name, offset, length, hash) \
  {RomEntryLoad, name, offset, length, 0, 0, GetRomHash(hash), nullptr},
#define ROM_LOADX_WORD_SWAP(name, offset, length, width, hash) \
  {RomEntryLoadXwordSwap, name,   offset, length, 0, width,    \
   GetRomHash(hash),      nullptr},
#define ROM_LOAD_SWAP(name, offset, length, hash) \
  ROM_LOADX_WORD_SWAP(name, offset, length, 2, hash)
#define ROM_RELOAD(offset, length) \
  {RomEntryReload, nullptr, offset, length, 0, 0, GetRomHash(0), nullptr},
#define ROM_END                                       \
  { RomEntryEnd, nullptr, 0, 0, 0, 0, GetRomHash(0) } \
  }                                                   \
  ;

#define ROT0 0
#define ROT90 1
#define ROT180 2
#define ROT270 3

#define PL1 0
#define PL2 4
#define PL3 8
#define PL4 12

#define CRC(n) 0x##n
#define SHA1(n)

#define init_0 NULL

#define GAME(YEAR, NAME, PARENT, INPUT, INIT, MODE, COMPANY, FULLNAME)       \
  static const GameEntry game_##NAME = {#NAME, #PARENT, FULLNAME,   COMPANY, \
                                        #YEAR, MODE,    roms_##NAME};

#define INPUT_PORT(NAME) static void input_##NAME()

enum {
  PLAYER_1 = 0,
  PLAYER_2,
  PLAYER_3,
  PLAYER_4,
};

#define ACTIVEHI (0)
#define ACTIVELOW (1)

typedef enum {
  IPT_PUSH1 = 0,
  IPT_PUSH2,
  IPT_PUSH3,
  IPT_PUSH4,
  IPT_PUSH5,
  IPT_PUSH6,
  IPT_PUSH7,
  IPT_PUSH8,
  IPT_START,
  IPT_SERVICE,
  IPT_COIN,
  IPT_UP,
  IPT_DOWN,
  IPT_RIGHT,
  IPT_LEFT,
} IPT_BTNS;

typedef enum {
  IPT_AUP = 0,
  IPT_ADOWN,
  IPT_ALEFT,
  IPT_ARIGHT,
  IPT_A2UP,
  IPT_A2DOWN,
  IPT_A2LEFT,
  IPT_A2RIGHT,
} IPT_ANALOGS;

typedef enum {
  IPT_MJA = 0,
  IPT_MJB,
  IPT_MJC,
  IPT_MJD,
  IPT_MJE,
  IPT_MJF,
  IPT_MJG,
  IPT_MJH,
  IPT_MJI,
  IPT_MJJ,
  IPT_MJK,
  IPT_MJL,
  IPT_MJM,
  IPT_MJN,
  IPT_MJFF,
  IPT_MJSTR,
  IPT_MJBET,
  IPT_MJLST,
  IPT_MJKAN,
  IPT_MJPON,
  IPT_MJCHI,
  IPT_MJRCH,
  IPT_MJRON,
} IPT_MAHJONG;

#define INPUT_PORTS_START(NAME) static inputEntry inpport_##NAME[] = {
#define INPUT_PORTS_END(NAME)                 \
  { End, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL } \
  }                                           \
  ;                                           \
  demulInfo.inputPorts = inpport_##NAME;
#define PORT_START(num) {Port, num, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL},
#define BIT_BUTTON(mask, inv, player, key) \
  {Button, key, 0, player, mask, inv, 0, 0, 0, 0, 0, NULL},
#define BIT_SWITCH(mask, inv, player, key) \
  {Switch, key, 0, player, mask, inv, 0, 0, 0, 0, 0, NULL},
#define BIT_UNUSED(mask, inv) {Unused, 0, 0, 0, mask, inv, 0, 0, 0, 0, 0, NULL},
#define BIT_HOOK(mask, inv, hook) \
  {Hook, 0, 0, 0, mask, inv, 0, 0, 0, 0, 0, hook},
#define BIT_TEST(mask, inv) BIT_BUTTON(mask, inv, 0, (1 << 6) | 0x00)
#define BIT_SERVICE(mask, inv) BIT_BUTTON(mask, inv, 0, (1 << 6) | 0x01)
#define BIT_MAHJONG(mask, inv, key) BIT_BUTTON(mask, inv, 0, (2 << 6) | key)
#define BIT_LGUN(mask, inv, player, key) \
  BIT_BUTTON(mask, inv, player, (3 << 6) | key)
#define PORT_AXISS(nport, ntype, nanalogm, nanalogp, nplayer, ncenter, nmin, \
                   nmax)                                                     \
  PORT_START(nport){ntype,    0,        0,    nplayer, 0,       0,           \
                    nanalogm, nanalogp, nmin, nmax,    ncenter, NULL},
#define PORT_PEDAL(naxis, nanalog, nplayer, ncenter, nmax) \
  PORT_AXISS(naxis, Pedal, 0, nanalog, nplayer, ncenter, ncenter, nmax)
#define PORT_AXIS(naxis, nanalogm, nanalogp, nplayer, ncenter, nmin, nmax) \
  PORT_AXISS(naxis, Axis, nanalogm, nanalogp, nplayer, ncenter, nmin, nmax)
#define PORT_LGUNX(naxis, nplayer, ncenter, nmin, nmax) \
  PORT_AXISS(naxis, LGun, 0, 0, nplayer, ncenter, nmin, nmax)
#define PORT_LGUNY(naxis, nplayer, ncenter, nmin, nmax) \
  PORT_AXISS(naxis, LGun, 1, 0, nplayer, ncenter, nmin, nmax)
#define PORT_ROTARY(naxis, nplayer, nrotary) \
  PORT_AXISS(naxis, Rotary, nrotary, 0, nplayer, 0, 0, 0)

struct LoadedEntry {
  uint32_t from_offset;
  uint32_t to_offset;
  uint8_t *buffer;
};

class GamesList {
 public:
  GamesList() : count_(0), head_(nullptr) {}
  ~GamesList() { FreeGameRoms(); }

  uint32_t GetCount() const { return count_; }

  const GameEntry *GetInfo(uint32_t idx) const {
    if (idx >= count_) return NULL;

    GameEntry **entry = (GameEntry **)head_;
    return entry[idx];
  }
  const char *GetGameField(uint32_t idx, EGameField field) const;
  uint32_t IndexOfName(const char *name) const;

  void Init();

  bool LoadGame(uint32_t idx, std::string path, bool open);
  void FreeGameRoms();
  uint8_t GetGameRomValue(uint32_t offset);
  void SetGameRomValue(uint32_t offset, uint8_t val);

 private:
  uint32_t count_;
  void *head_;
  const GameEntry *game = nullptr;

  std::string nand_file_name_;
  std::array<uint8_t, 0x21000> nand_buffer_;

  int32_t cursectorloaded = -1;
  std::array<uint8_t, 32768> sector_;
  uint8_t GetByte(std::fstream *file, uint32_t ofs, uint32_t rofs);
  bool Load(RomEntry *rom_entry, std::filesystem::path path, bool open);
};
