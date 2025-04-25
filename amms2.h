#pragma once

#include <array>
#include <cstdint>

class Amms2Decoder {
 public:
  void Init(uint8_t* ptr);
  bool GetSample(int32_t& sample);

 private:
  int32_t current_sample;
  int16_t samples[32 + 32];

  struct DecodeBuffer {
    std::array<float, 1024> data;
    uint32_t index;
  };

  struct Stream {
    uint8_t* stream_data;
    uint32_t data_byte;
    uint32_t bits_left;
    uint32_t byte_index;
    uint32_t bytes_total;

    int GetStreamBits(uint32_t bits);
    int GetStreamByte();
  };

  DecodeBuffer decoded_buffer_[2];
  Stream stream_;

  uint8_t bitalidx[2][32];
  uint32_t scf[2][3][32];
  float frac[2][3][32];
  int32_t chunknum;
  int32_t sblimit;
  int32_t sblimit2;
  int32_t bframe;
  int32_t iframe;
  int32_t sframe;
  int32_t chunkcnt;
  int32_t fcount;
  uint8_t freq;
  uint8_t chanmode;
  uint8_t modeext;
  uint8_t quality;
  uint8_t channum;

  int32_t Decode(int16_t* out);
  int32_t DecodeFrame(uint32_t* buf);
  void FindHeader();
  int ReadHeader();
  void ReadFraction(int chan, int sb);
  void ReadFractions(uint32_t* buf);
  void ReadScalef();
  void ReadBitalloc();
  int32_t Dct32(float* src, float* dst);

  int32_t Synth(float* in_buf, DecodeBuffer& decoded_buffer, int16_t* result);
};
