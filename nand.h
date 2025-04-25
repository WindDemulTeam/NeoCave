#pragma once

#include <cstdint>

class GamesList;

class Nand {
 public:
  void Init(GamesList *l);
  int Read();
  void Write(int byte);
  int Ack();

 private:
  GamesList *gameList;
  enum {
    kCmdNone = 0,
    kCmdReady = 1,
    kCmdReset = 2,
    kCmdRead = 3,
    kCmdReadAddr = 30,
    kCmdReadCycle1 = 31,
    kCmdReadData = 32,
    kCmdId = 4,
    kCmdIdCycle1 = 40,
    kCmdIdSend = 41,
    kCmdStatus = 5,
    kCmdErase = 6,
    kCmdEraseAddr = 60,
    kCmdEraseCycle1 = 61,
    kCmdWrite = 7,
    kCmdWriteAddr = 70,
    kCmdWriteData = 71,
    kCmdWriteCycle1 = 72,
  };

  uint8_t nand_ack;
  uint8_t nand_ack_delay;
  uint8_t nand_state;
  uint8_t nand_substate;
  uint8_t nand_count;
  uint16_t nand_column;
  uint16_t nand_row;
};