
#include "ymz770.h"

#include <algorithm>
#include <string>

Ymz770 ::Ymz770() {
  for (int i = 0; i < kMaxChannels; i++) {
    channel_state_[i] = kStop;
    sequences_[i].state_ = kSeqIdle;
  }
  spu_.fill(0);
  std::memset(regs, 0, sizeof(regs));
  reg_num = 0;
}

void Ymz770::StartChannel(int i) { channel_state_[i] = kInit; }
void Ymz770::StopChannel(int i) { channel_state_[i] = kStop; }

void Ymz770::InitChannel(int i) {
  uint32_t offset = Read(channels_data[i].msn * 4);
  decoder_.Init(&spu_[offset]);
  channel_state_[i] = kPlay;
}

int32_t Ymz770::ReadChannel(int i) {
  int32_t sample = 0;

  auto &channel = channels_data[i];

  if (kInit == channel_state_[i]) {
    InitChannel(i);
  }
  if (kPlay == channel_state_[i]) {
    if (!decoder_.GetSample(sample)) {
      if (!channel.loop) {
        StopChannel(i);
      } else {
        InitChannel(i);
        decoder_.GetSample(sample);
      }
    }
  }
  return sample * channel.vlm / 128;
}

int16_t Ymz770::GetNextSample() {
  int32_t sample = 0;

  Sequencer();
  for (int i = 0; i < kMaxChannels; i++) {
    sample += ReadChannel(i);
  }

  sample *= vlma;
  sample >>= 7 - bsl;

  constexpr int32_t clip_max3 = 32768 * 75 / 100;
  constexpr int32_t clip_max2 = 32768 * 875 / 1000;
  switch (cpl) {
    case 3:
      sample = std::clamp(sample, -clip_max3, clip_max3);
      break;
    case 2:
      sample = std::clamp(sample, -clip_max2, clip_max2);
      break;
    case 1:
      sample = std::clamp(sample, -32768, 32767);
      break;
  }

  if (mute) {
    sample = 0;
  }

  return static_cast<int16_t>(sample);
}

void Ymz770::Sequencer() {
  for (int i = 0; i < kMaxChannels; i++) {
    SeqState state = sequences_[i].state_;
    if (kSeqIdle == state) {
      continue;
    } else if (kSeqWait == state) {
      if (!--sequences_[i].idle_cnt_) {
        sequences_[i].state_ = kSeqPlaying;
      }
    } else if (kSeqStart == state) {
      uint32_t offset = Read(0x400 + sequences_data[i].sqsn * 4);
      sequences_[i].state_ = kSeqPlaying;
      sequences_[i].seqptr_ = reinterpret_cast<uint16_t *>(&spu_[offset]);
    }

    if (kSeqPlaying == state) {
      uint16_t seqdata = *sequences_[i].seqptr_;
      sequences_[i].seqptr_++;
      switch (seqdata & 0xff) {
        case 0x0e:
          sequences_[i].idle_cnt_ = 32 - 1;
          sequences_[i].state_ = kSeqWait;
          break;
        case 0x0f:
          sequences_[i].state_ = sequences_data[i].sqlp ? kSeqStart : kSeqIdle;
          break;
        default:
          RegWrite(seqdata & 0xff, seqdata >> 8);
          break;
      }
    }
  }
}

void Ymz770::RegWrite(uint32_t reg, uint8_t value) {
  regs[reg] = value;

  if ((reg & 0xe3) == 0x43) {
    uint8_t chan = (reg & 0x1c) >> 2;

    if (value & 6) {
      StartChannel(chan);
    } else {
      StopChannel(chan);
    }
  }

  if ((reg & 0x8f) == 0x81) {
    uint8_t sqn = (reg & 0x70) >> 4;
    sequences_[sqn].state_ = (value & 6) ? kSeqStart : kSeqIdle;
  }
}

void Ymz770::Write(uint32_t reg, uint8_t value) {
  if (reg & 1) {
    RegWrite(reg_num, value);
  } else {
    reg_num = value;
  }
}