#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

#include "amms2.h"

struct YmzChannelData {
  uint8_t msn;
  uint8_t vlm;
  uint8_t pan : 6;
  uint8_t : 2;
  uint8_t loop : 1;
  uint8_t kon : 2;
  uint8_t : 5;
};

struct YmzSequenceData {
  uint8_t sqsn;
  uint8_t sqlp : 1;
  uint8_t sqon : 2;
  uint8_t : 5;
  uint8_t tmrh;
  uint8_t tmrl;
  uint8_t tgst;
  uint8_t tgen;
  uint8_t sqof;
  uint8_t unused[9];
};

enum ChanState : uint32_t { kInit, kPlay, kStop };
enum SeqState : uint32_t { kSeqIdle, kSeqStart, kSeqPlaying, kSeqWait };

struct YmzSequence {
  std::atomic<SeqState> state_;
  uint32_t idle_cnt_;
  uint16_t *seqptr_;
};

class Ymz770 {
 public:
  static const uint32_t kSpuSize = 0x00800000;

  void WriteRom(uint32_t offset, uint8_t v) { spu_[offset] = v; }
  void Write(uint32_t reg, uint8_t value);
  int16_t GetNextSample();

  Ymz770();

 private:
  static const int kMaxChannels = 8;

  std::array<uint8_t, kSpuSize> spu_;
  Amms2Decoder decoder_;

  uint32_t Read(uint32_t addr) {
    addr &= spu_.size() - 1;
    uint32_t v = *(uint32_t *)&spu_[addr];
    return (v << 24) | ((v << 8) & 0x00FF0000) | ((v >> 8) & 0x0000FF00) |
           (v >> 24);
  }

  union {
    struct {
      uint8_t mute : 1;
      uint8_t doen : 1;
      uint8_t : 6;
      uint8_t vlma : 8;
      uint8_t bsl : 3;
      uint8_t : 1;
      uint8_t cpl : 2;
      uint8_t : 2;
      uint8_t unused0[0x3d];
      YmzChannelData channels_data[kMaxChannels];
      uint8_t unused1[0x20];
      YmzSequenceData sequences_data[kMaxChannels];
    };
    uint8_t regs[256];
  };
  uint8_t reg_num;

  YmzSequence sequences_[kMaxChannels];

  void Sequencer();
  void RegWrite(uint32_t reg, uint8_t value);

  void InitChannel(int i);
  void StartChannel(int i);
  void StopChannel(int i);
  int32_t ReadChannel(int i);

  std::atomic<ChanState> channel_state_[kMaxChannels];
};
