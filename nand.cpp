
#include "nand.h"

#include "roms.h"

void Nand::Init(GamesList *l) {
  nand_ack = 0x00;
  nand_state = kCmdReady;
  nand_substate = kCmdNone;
  nand_count = 0;
  nand_ack_delay = 1;
  nand_column = 0;
  nand_row = 0;
  gameList = l;
}

int Nand::Read() {
  switch (nand_state) {
    case kCmdIdSend:
      switch (nand_count) {
        case 0:
          nand_count++;
          return 0xec;
        case 1:
          nand_count++;
          nand_state = kCmdReady;
          return 0xf1;
      }
      break;
    case kCmdReadData: {
      int ret = gameList->GetGameRomValue((nand_row * 0x840) + nand_column);
      nand_column++;
      if (nand_column == 0x840) nand_state = kCmdReady;
      return ret;
    }
    case kCmdStatus:
      nand_state = kCmdReady;
      return 0xe0;
  }
  return 0xff;
}

void Nand::Write(int byte) {
  switch (nand_state) {
    case kCmdReadData:
    case kCmdReady:
      switch (byte) {
        case 0xff:  // RESET
          nand_ack = 0x20;
          nand_ack_delay = 1;
          break;
        case 0x90:  // DEVICE ID
          nand_state = kCmdId;
          nand_substate = kCmdIdCycle1;
          break;
        case 0x00:  // READ
          nand_state = kCmdRead;
          nand_substate = kCmdReadAddr;
          nand_count = 0;
          break;
        case 0x70:  // STATUS
          nand_state = kCmdStatus;
          nand_substate = kCmdNone;
          break;
        case 0x60:  // ERASE
          nand_state = kCmdErase;
          nand_substate = kCmdEraseAddr;
          nand_column = 0;
          nand_count = 0;
          break;
        case 0x80:  // WRITE
          nand_state = kCmdWrite;
          nand_substate = kCmdWriteAddr;
          nand_count = 0;
          break;
        default:
          break;
      }
      break;
    case kCmdId:
      switch (nand_substate) {
        case kCmdIdCycle1:
          switch (byte) {
            case 0x00:
              nand_state = kCmdIdSend;
              nand_substate = kCmdNone;
              nand_count = 0;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    case kCmdRead:
      switch (nand_substate) {
        case kCmdReadAddr:
          switch (nand_count) {
            case 0:
              nand_column = byte;
              nand_count++;
              break;
            case 1:
              nand_column |= (byte << 8);
              nand_column &= 0x0fff;
              nand_count++;
              break;
            case 2:
              nand_row = byte;
              nand_count++;
              break;
            case 3:
              nand_row |= (byte << 8);
              nand_substate = kCmdReadCycle1;
              nand_count++;
              break;
          }
          break;
        case kCmdReadCycle1:
          switch (byte) {
            case 0x30:
              nand_state = kCmdReadData;
              nand_substate = kCmdNone;
              nand_count = 0;
              nand_ack = 0x20;
              nand_ack_delay = 1;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    case kCmdErase:
      switch (nand_substate) {
        case kCmdEraseAddr:
          switch (nand_count) {
            case 0:
              nand_row = byte;
              nand_count++;
              break;
            case 1:
              nand_row |= (byte << 8);
              nand_substate = kCmdEraseCycle1;
              nand_count++;
              break;
          }
          break;
        case kCmdEraseCycle1:
          switch (byte) {
            case 0xd0:
              nand_state = kCmdReady;
              nand_ack = 0x20;
              nand_ack_delay = 1;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;

    case kCmdWrite:
      switch (nand_substate) {
        case kCmdWriteAddr:
          switch (nand_count) {
            case 0:
              nand_column = byte;
              nand_count++;
              break;
            case 1:
              nand_column |= (byte << 8);
              nand_column &= 0x0fff;
              nand_count++;
              break;
            case 2:
              nand_row = byte;
              nand_count++;
              break;
            case 3:
              nand_row |= (byte << 8);
              nand_substate = kCmdWriteData;
              nand_count++;
              break;
          }
          break;
        case kCmdWriteData: {
          gameList->SetGameRomValue((nand_row * 0x840) + nand_column, byte);
          nand_column++;
          if (nand_column == 0x840) nand_substate = kCmdWriteCycle1;
          break;
        }
        case kCmdWriteCycle1:
          switch (byte) {
            case 0x10:
              nand_state = kCmdReady;
              nand_ack = 0x20;
              nand_ack_delay = 1;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
  }
}

int Nand::Ack() {
  int res = nand_ack;
  if (!(--nand_ack_delay)) nand_ack = 0x00;
  return res;
}