#include "amms2.h"

#include <string>

#include "amm2_table.h"

int Amms2Decoder::Stream::GetStreamByte() {
  bytes_total++;
  return stream_data[byte_index++];
}

int Amms2Decoder::Stream::GetStreamBits(uint32_t bits) {
  int v2;
  int v4;

  v2 = v4 = data_byte;

  while (bits_left < bits) {
    v2 = GetStreamByte();
    bits_left += 8;
    v4 = v2 | (v4 << 8);
  }

  int bitsleft = bits_left - bits;
  data_byte = v2;
  bits_left = bitsleft;
  return ((1 << bits) - 1) & (v4 >> bitsleft);
}

void Amms2Decoder::FindHeader() {
  int val;
  do {
    stream_.bits_left = 0;
    do {
      val = stream_.GetStreamBits(8);
    } while (val != 0xff);
    val = stream_.GetStreamBits(4);
  } while (val != 0xf);
}

int Amms2Decoder::ReadHeader() {
  int version, layer, prot, bitrate, freq, padding, temp;

  version = stream_.GetStreamBits(1);
  layer = stream_.GetStreamBits(2);
  prot = stream_.GetStreamBits(1);

  if (layer != 2 || prot) {
    return -1;
  }

  bitrate = stream_.GetStreamBits(4);
  if (bitrate > 12) {
    return -1;
  }

  freq = stream_.GetStreamBits(2);
  if (freq == 3) {
    return -1;
  }

  freq = freq + 4 * (1 - version);

  padding = stream_.GetStreamBits(2);
  if (padding == 3) {
    return -1;
  }

  chunknum = padding + 3 * bitrate;

  chanmode = stream_.GetStreamBits(2);
  modeext = stream_.GetStreamBits(2);

  temp = stream_.GetStreamBits(3);
  if (temp >= 5) {
    return -1;
  }

  quality = temp;

  temp = stream_.GetStreamBits(1);
  if (temp == 1) {
    return -1;
  }

  channum = (chanmode != 3) + 1;
  temp = (int32_t)(int16_t)sblimits[quality];
  sblimit2 = temp;
  if (chanmode == 1) {
    sblimit = sblimits2[modeext];
    if (sblimit > temp) sblimit = temp;
  } else {
    sblimit = temp;
  }
  return 1;
}

void Amms2Decoder::ReadFraction(int chan, int sb) {
  int32_t v0, v1, v2, v3, v4;

  int32_t idx = bitalidx[chan][sb];
  int32_t bits = bitalloc[idx];
  int32_t pow = (1ul << (bits - 1));

  switch (idx) {
    case 0:
      frac[chan][0][sb] = frac[chan][1][sb] = frac[chan][2][sb] = 0;
      return;
    case 1:
    case 2:
    case 4:
      v3 = stream_.GetStreamBits(bitalloc2[idx]);
      v4 = bitmask[idx];
      v0 = v3 % v4;
      v1 = (v3 / v4) % v4;
      v2 = (v3 / v4) / v4 % v4;
      break;
    default:
      v0 = stream_.GetStreamBits(bits);
      v1 = stream_.GetStreamBits(bits);
      v2 = stream_.GetStreamBits(bits);
      break;
  }

  float scale = *(float*)&fracscale[idx];
  float offs = *(float*)&fracoffs[idx];

  frac[chan][0][sb] = (((float)(v0 - pow) / (float)pow) + offs) * scale;
  frac[chan][1][sb] = (((float)(v1 - pow) / (float)pow) + offs) * scale;
  frac[chan][2][sb] = (((float)(v2 - pow) / (float)pow) + offs) * scale;
}

void Amms2Decoder::ReadFractions(uint32_t* buf) {
  int i;

  if (!sframe) {
    for (i = 0; i < sblimit; i++) {
      for (int j = 0; j < channum; j++) {
        ReadFraction(j, i);
        float v0 = *(float*)&scf[j][bframe][i];
        frac[j][0][i] *= v0;
        frac[j][1][i] *= v0;
        frac[j][2][i] *= v0;
      }
    }

    for (; i < 32; i++) {
      frac[0][0][i] = frac[0][1][i] = frac[0][2][i] = 0;
      frac[1][0][i] = frac[1][1][i] = frac[1][2][i] = 0;
    }
  }

  for (int j = 0; j < channum; j++) {
    uint32_t* src = (uint32_t*)&frac[j][sframe][0];
    for (i = 0; i < 32; i += 8) {
      *buf++ = *src++;
      *buf++ = *src++;
      *buf++ = *src++;
      *buf++ = *src++;
      *buf++ = *src++;
      *buf++ = *src++;
      *buf++ = *src++;
      *buf++ = *src++;
    }
  }

  sframe += 1;
  chunkcnt += 1;

  if (sframe >= 3) {
    iframe += 1;
    sframe = 0;
    if (iframe >= 4) {
      bframe += 1;
      iframe = 0;
      if (bframe >= 3) {
        bframe = 0;
        chunkcnt = 0;
      }
    }
  }
}

void Amms2Decoder::ReadScalef() {
  uint8_t scfsi[2][32];

  for (int i = 0; i < sblimit2; i++)
    for (int j = 0; j < channum; j++)
      if (bitalidx[j][i] != 0) scfsi[j][i] = stream_.GetStreamBits(2);

  for (int i = 0; i < sblimit2; i++)
    for (int j = 0; j < channum; j++)
      if (bitalidx[j][i] != 0) {
        switch (scfsi[j][i]) {
          case 0:
            scf[j][0][i] = sbscale[stream_.GetStreamBits(6)];
            scf[j][1][i] = sbscale[stream_.GetStreamBits(6)];
            scf[j][2][i] = sbscale[stream_.GetStreamBits(6)];
            break;
          case 1:
            scf[j][0][i] = scf[j][1][i] = sbscale[stream_.GetStreamBits(6)];
            scf[j][2][i] = sbscale[stream_.GetStreamBits(6)];
            break;
          case 2:
            scf[j][0][i] = scf[j][1][i] = scf[j][2][i] =
                sbscale[stream_.GetStreamBits(6)];
            break;
          case 3:
            scf[j][0][i] = sbscale[stream_.GetStreamBits(6)];
            scf[j][1][i] = scf[j][2][i] = sbscale[stream_.GetStreamBits(6)];
            break;
        }
      } else {
        scf[j][0][i] = scf[j][1][i] = scf[j][2][i] = 0;
      }
}

void Amms2Decoder::ReadBitalloc() {
  int i;

  for (i = 0; i < sblimit; i++) {
    for (int j = 0; j < channum; j++) {
      bitalidx[j][i] =
          sballoc[quality][i][stream_.GetStreamBits(sbbits[quality][i])];
    }
  }

  for (; i < 32; i++) {
    bitalidx[0][i] = bitalidx[1][i] = 0;
  }
}

int32_t Amms2Decoder::DecodeFrame(uint32_t* buf) {
  int32_t cnt = chunkcnt;
  if (!cnt) {
    FindHeader();
    if (ReadHeader() < 0) return -1;
    ReadBitalloc();
    ReadScalef();
  }
  ReadFractions(buf);
  ++fcount;
  return cnt < chunknum;
}

int32_t Amms2Decoder::Dct32(float* src, float* dst) {
  double v53, v58, v59, v60, v74, v78, v79, v80, v81;
  double v82, v83, v129, v142, v143, v144, v145, v146;
  double v147, v148, v149, v150, v151, v152, v153, v154;
  double v155, v200, v201, v202, v203, v204, v205, v206;
  double t0, t1, t2, t3, t4, t5, t6, t7;

  t0 = src[0] + src[31];
  v142 = src[0] - src[31];
  t1 = src[1] + src[30];
  v152 = src[1] - src[30];
  v74 = src[2] + src[29];
  v200 = src[2] - src[29];
  v146 = src[3] + src[28];
  v144 = src[3] - src[28];
  v81 = src[4] + src[27];
  v150 = src[4] - src[27];
  v79 = src[5] + src[26];
  v154 = src[5] - src[26];
  v201 = src[6] + src[25];
  v129 = src[6] - src[25];
  v82 = src[7] + src[24];
  v148 = src[7] - src[24];
  v53 = src[8] + src[23];
  v153 = src[8] - src[23];
  v60 = src[9] + src[22];
  v151 = src[9] - src[22];
  v202 = src[10] + src[21];
  v203 = src[10] - src[21];
  v204 = src[11] + src[20];
  v145 = src[11] - src[20];
  v205 = src[12] + src[19];
  v149 = src[12] - src[19];
  v58 = src[13] + src[18];
  v155 = src[13] - src[18];
  t2 = src[17] + src[14];
  v143 = src[14] - src[17];
  t3 = src[16] + src[15];
  v147 = src[15] - src[16];

  v83 = t0 + t3;
  v80 = t0 - t3;
  v59 = t1 + t2;
  v78 = t1 - t2;
  t0 = v74 + v58;
  v74 = v74 - v58;
  t1 = v146 + v205;
  v146 = v146 - v205;
  t2 = v81 + v204;
  v81 = v81 - v204;
  t3 = v79 + v202;
  v79 = v79 - v202;
  t4 = v201 + v60;
  v201 = v201 - v60;
  t5 = v82 + v53;
  v82 = v82 - v53;

  v53 = v83 + t5;
  v83 = v83 - t5;
  v60 = v59 + t4;
  v59 = v59 - t4;
  v202 = t0 + t3;
  v206 = t0 - t3;
  v204 = t1 + t2;
  v58 = t1 - t2;

  t0 = v53 + v204;
  v53 = v53 - v204;
  t1 = v60 + v202;
  v60 = v60 - v202;

  v202 = t0 + t1;

  t1 = (t0 - t1) * sincos[_sin_pi_div_4];
  t2 = v53 * sincos[_cos_pi_div_8] + v60 * sincos[_sin_pi_div_8];
  v53 = v53 * sincos[_sin_pi_div_8] - v60 * sincos[_cos_pi_div_8];
  v60 = v83 * sincos[_cos_pi_div_16] + v58 * sincos[_sin_pi_div_16];
  v83 = v83 * sincos[_sin_pi_div_16] - v58 * sincos[_cos_pi_div_16];
  v58 = v206 * sincos[_sin_3pi_div_16] + v59 * sincos[_cos_3pi_div_16];
  v59 = v206 * sincos[_cos_3pi_div_16] - v59 * sincos[_sin_3pi_div_16];
  t3 = v60 + v58;
  t4 = (v60 - v58) * sincos[_sin_pi_div_4];
  v58 = v83 + v59;
  t5 = (v83 - v59) * sincos[_sin_pi_div_4];
  v59 = t4 + t5;
  v60 = t4 - t5;
  t4 = v80 * sincos[_cos_pi_div_32] + v82 * sincos[_sin_pi_div_32];
  v80 = v80 * sincos[_sin_pi_div_32] - v82 * sincos[_cos_pi_div_32];
  t5 = v201 * sincos[_sin_3pi_div_32] + v78 * sincos[_cos_3pi_div_32];
  v78 = v201 * sincos[_cos_3pi_div_32] - v78 * sincos[_sin_3pi_div_32];
  t6 = v74 * sincos[_cos_5pi_div_32] + v79 * sincos[_sin_5pi_div_32];
  v74 = v74 * sincos[_sin_5pi_div_32] - v79 * sincos[_cos_5pi_div_32];
  v79 = v81 * sincos[_sin_7pi_div_32] + v146 * sincos[_cos_7pi_div_32];
  v146 = v81 * sincos[_cos_7pi_div_32] - v146 * sincos[_sin_7pi_div_32];
  v81 = t4 + v79;
  v83 = t4 - v79;
  v79 = t5 + t6;
  v82 = t5 - t6;
  t4 = v81 + v79;
  t5 = (v81 - v79) * sincos[_sin_pi_div_4];
  t6 = v83 * sincos[_cos_pi_div_8] + v82 * sincos[_sin_pi_div_8];
  v83 = v83 * sincos[_sin_pi_div_8] - v82 * sincos[_cos_pi_div_8];
  t7 = v80 + v146;
  v80 = v80 - v146;
  v146 = v78 + v74;
  v78 = v78 - v74;
  v74 = t7 + v146;
  v82 = (t7 - v146) * sincos[_sin_pi_div_4];
  v146 = v80 * sincos[_cos_pi_div_8] + v78 * sincos[_sin_pi_div_8];
  v80 = v80 * sincos[_sin_pi_div_8] - v78 * sincos[_cos_pi_div_8];
  v78 = t6 + v80;
  v79 = t6 - v80;
  v80 = t5 + v82;
  v81 = t5 - v82;
  v82 = v83 + v146;
  v83 = v83 - v146;
  t5 = v142 * sincos[_cos_pi_div_64] + v147 * sincos[_sin_pi_div_64];
  v142 = v142 * sincos[_sin_pi_div_64] - v147 * sincos[_cos_pi_div_64];
  t6 = v143 * sincos[_sin_3pi_div_64] + v152 * sincos[_cos_3pi_div_64];
  v152 = v143 * sincos[_cos_3pi_div_64] - v152 * sincos[_sin_3pi_div_64];
  v143 = v200 * sincos[_cos_5pi_div_64] + v155 * sincos[_sin_5pi_div_64];
  v200 = v200 * sincos[_sin_5pi_div_64] - v155 * sincos[_cos_5pi_div_64];
  v155 = v149 * sincos[_sin_7pi_div_64] + v144 * sincos[_cos_7pi_div_64];
  v144 = v149 * sincos[_cos_7pi_div_64] - v144 * sincos[_sin_7pi_div_64];
  v149 = v150 * sincos[_cos_9pi_div_64] + v145 * sincos[_sin_9pi_div_64];
  v150 = v150 * sincos[_sin_9pi_div_64] - v145 * sincos[_cos_9pi_div_64];
  v145 = v203 * sincos[_sin_11pi_div_64] + v154 * sincos[_cos_11pi_div_64];
  v154 = v203 * sincos[_cos_11pi_div_64] - v154 * sincos[_sin_11pi_div_64];
  v203 = v129 * sincos[_cos_13pi_div_64] + v151 * sincos[_sin_13pi_div_64];
  v129 = v129 * sincos[_sin_13pi_div_64] - v151 * sincos[_cos_13pi_div_64];
  v151 = v153 * sincos[_sin_15pi_div_64] + v148 * sincos[_cos_15pi_div_64];
  v148 = v153 * sincos[_cos_15pi_div_64] - v148 * sincos[_sin_15pi_div_64];
  v153 = t5 + v151;
  v146 = t5 - v151;
  v151 = t6 + v203;
  v147 = t6 - v203;
  t5 = v143 + v145;
  v143 = v143 - v145;
  t6 = v155 + v149;
  v155 = v155 - v149;
  v149 = v153 + t6;
  v153 = v153 - t6;
  v145 = v151 + t5;
  v151 = v151 - t5;
  t5 = v149 + v145;
  t6 = (v149 - v145) * sincos[_sin_pi_div_4];
  v145 = v153 * sincos[_cos_pi_div_8] + v151 * sincos[_sin_pi_div_8];
  v153 = v153 * sincos[_sin_pi_div_8] - v151 * sincos[_cos_pi_div_8];
  v151 = v146 * sincos[_cos_pi_div_16] + v155 * sincos[_sin_pi_div_16];
  v146 = v146 * sincos[_sin_pi_div_16] - v155 * sincos[_cos_pi_div_16];
  v155 = v143 * sincos[_sin_3pi_div_16] + v147 * sincos[_cos_3pi_div_16];
  v147 = v143 * sincos[_cos_3pi_div_16] - v147 * sincos[_sin_3pi_div_16];
  v143 = v155 + v151;
  t7 = (v151 - v155) * sincos[_sin_pi_div_4];
  v155 = v147 + v146;
  v146 = (v146 - v147) * sincos[_sin_pi_div_4];
  v147 = t7 + v146;
  v151 = t7 - v146;
  t7 = v142 + v148;
  v142 = v142 - v148;
  v148 = v152 + v129;
  v152 = v152 - v129;
  v129 = v200 + v154;
  v200 = v200 - v154;
  v154 = v144 + v150;
  v144 = v144 - v150;
  v150 = t7 + v154;
  v146 = t7 - v154;
  t7 = v148 + v129;
  v148 = v148 - v129;
  v129 = v150 + t7;
  v150 = (v150 - t7) * sincos[_sin_pi_div_4];
  v154 = v146 * sincos[_cos_pi_div_8] + v148 * sincos[_sin_pi_div_8];
  v146 = v146 * sincos[_sin_pi_div_8] - v148 * sincos[_cos_pi_div_8];
  v148 = v142 * sincos[_cos_pi_div_16] + v144 * sincos[_sin_pi_div_16];
  v142 = v142 * sincos[_sin_pi_div_16] - v144 * sincos[_cos_pi_div_16];
  v144 = v200 * sincos[_sin_3pi_div_16] + v152 * sincos[_cos_3pi_div_16];
  v152 = v200 * sincos[_cos_3pi_div_16] - v152 * sincos[_sin_3pi_div_16];
  t7 = v148 + v144;
  v148 = (v148 - v144) * sincos[_sin_pi_div_4];
  v144 = v142 + v152;
  v142 = (v142 - v152) * sincos[_sin_pi_div_4];
  v152 = v148 + v142;
  v148 = v148 - v142;
  v142 = v143 + v144;
  v143 = v143 - v144;
  dst[3] = (float)v142;
  dst[6] = (float)v78;
  v144 = v145 + v146;
  v145 = v145 - v146;
  dst[5] = (float)v143;
  dst[7] = (float)v144;
  dst[10] = (float)v79;
  dst[9] = (float)v145;
  dst[12] = (float)v59;
  v146 = v147 + v148;
  v147 = v147 - v148;
  dst[14] = (float)v80;
  dst[11] = (float)v146;
  dst[13] = (float)v147;
  v148 = t6 + v150;
  v149 = t6 - v150;
  dst[15] = (float)v148;
  dst[18] = (float)v81;
  dst[17] = (float)v149;
  dst[20] = (float)v60;
  v150 = v151 + v152;
  v151 = v151 - v152;
  dst[19] = (float)v150;
  dst[21] = (float)v151;
  v152 = v153 + v154;
  v153 = v153 - v154;
  v154 = v155 + t7;
  v155 = v155 - t7;
  dst[0] = (float)v202;
  dst[1] = (float)t5;
  dst[2] = (float)t4;
  dst[4] = (float)t3;
  dst[8] = (float)t2;
  dst[16] = (float)t1;
  dst[22] = (float)v82;
  dst[23] = (float)v152;
  dst[24] = (float)v53;
  dst[25] = (float)v153;
  dst[26] = (float)v83;
  dst[27] = (float)v154;
  dst[28] = (float)v58;
  dst[29] = (float)v155;
  dst[30] = (float)v74;
  dst[31] = (float)v129;
  return 0;
}

int32_t Amms2Decoder::Synth(float* in_buf, DecodeBuffer& decoded_buffer,
                            int16_t* result) {
  float* out_buf = &decoded_buffer.data[decoded_buffer.index];

  Dct32(in_buf, out_buf);
  out_buf += 16;

  float buffer[32] = {0};
  float* dtable = (float*)synth_table;

  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 16; i++) {
      buffer[i] += (out_buf[i] * dtable[i] - out_buf[32 - i] * dtable[32 + i]);
    }

    buffer[16] -= out_buf[16] * dtable[16 + 32];

    for (int i = 17; i < 32; i++) {
      buffer[i] -= (out_buf[32 - i] * dtable[i] + out_buf[i] * dtable[32 + i]);
    }

    out_buf += 64;
    dtable += 64;
  }

  for (int j = 0; j < 32; j++) {
    int32_t val = (int32_t)((buffer[j] * 32768.f) + 32768.5f);

    if (val >= 65536) {
      val = 32767;
    } else if (val < 0) {
      val = -32768;
    } else
      val += -32768;

    result[j] = val;
  }

  if (decoded_buffer.index == 0) {
    uint32_t* dst = (uint32_t*)&decoded_buffer.data[544];
    uint32_t* src = (uint32_t*)&decoded_buffer.data[0];

    for (int j = 0; j < 48; j++) {
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
      *dst++ = *src++;
    }

    decoded_buffer.index = 512;

  } else {
    decoded_buffer.index -= 32;
  }

  return 0;
}

int32_t Amms2Decoder::Decode(int16_t* out) {
  int32_t result;

  float buf[32 + 32] = {0};

  result = DecodeFrame((uint32_t*)buf);
  if (result <= 0) {
    return -1;
  }

  if (channum == 1) {
    Synth(buf, decoded_buffer_[0], out);
    return 32;
  } else {
    int16_t data[32];
    for (int i = 0; i < 64; i += 32) {
      Synth(&buf[i], decoded_buffer_[i / 32], data);
      for (int j = 0; j < 32; j++) {
        out[j * 2] = data[j];
      }
      out++;
    }

    return 64;
  }

  return 0;
}

void Amms2Decoder::Init(uint8_t* ptr) {
  stream_.stream_data = ptr;
  stream_.byte_index = 0;

  chunknum = 0;
  sblimit = 0;
  sblimit2 = 0;
  bframe = 0;
  iframe = 0;
  sframe = 0;
  chunkcnt = 0;
  fcount = 0;
  freq = 0;
  chanmode = 0;
  modeext = 0;
  quality = 0;
  channum = 0;

  decoded_buffer_->data.fill(0);
  decoded_buffer_->index = 0;
  current_sample = 0;
}

bool Amms2Decoder::GetSample(int32_t& sample) {
  if (current_sample == 0) {
    int32_t result = Decode(samples);
    if (result < 0) {
      return false;
    }
  }

  sample = samples[current_sample++];
  current_sample &= 0x1f;
  return true;
}