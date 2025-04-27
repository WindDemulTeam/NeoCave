#include "blitter.h"

#include <emmintrin.h>

#include "sh3.h"

Blitter::Blitter(std::span<uint8_t> ram, counters::Counters &c)
    : ram_(ram), blitting_(false), counters(c) {
  gpu_.fill(0xff);
  gpu_regs_.fill(0);

  v_sync_ =
      new counters::Counter(counters::Counter::kEnable,
                            static_cast<uint32_t>(sh3::Cpu::kHz / 60.0178), 1,
                            std::bind(&Blitter::Vblank, this));

  blit_irq_ = new counters::Counter(counters::Counter::kOneShot, 1, 1,
                                    std::bind(&Blitter::BlitIrq, this));

  running_ = true;
  blit_thread_ = new std::thread(&Blitter::Blit, this);
}

Blitter::~Blitter() {
  running_ = false;
  blit_cv_.notify_one();
  blit_thread_->join();
  delete v_sync_;
  delete blit_irq_;
}

void Blitter::Upload(uint32_t &addr) {
  addr += 6;
  uint32_t x_start = Next16(addr) & 0x1fff;
  uint32_t y_start = Next16(addr) & 0x0fff;
  uint32_t dimx = (Next16(addr) & 0x1fff) + 1;
  uint32_t dimy = (Next16(addr) & 0x0fff) + 1;

  for (uint32_t y = 0; y < dimy; y++) {
    uint16_t *dst = (uint16_t *)&gpu_[(y_start + y) * kSizeX * 2];
    dst += x_start;
    for (uint32_t x = 0; x < dimx; x++) {
      *dst++ = Next16(addr);
    }
  }
}

template <int simple, int blend, int flip_x, int tined, int transparent,
          int s_mode, int d_mode>
void Blitter::Block16(uint16_t **d_mem, uint16_t **s_mem, int blocks16,
                      uint8_t src_alpha, uint8_t dst_alpha, uint32_t tine) {
  if (blocks16 <= 0) return;

  __m128i tine_color;
  __m128i mask_ff;
  __m128i mask_1f;
  __m128i source_alpha;
  __m128i dest_alpha;

  mask_ff = _mm_set1_epi16(0xff);
  mask_1f = _mm_set1_epi16(0x1f);

  if constexpr (blend) {
    if constexpr (s_mode == 0) {
      source_alpha = _mm_set1_epi16(src_alpha);
    } else if constexpr (s_mode == 4) {
      source_alpha = _mm_xor_si128(_mm_set1_epi16(src_alpha), mask_ff);
    }

    if constexpr (d_mode == 0) {
      dest_alpha = _mm_set1_epi16(dst_alpha);
    } else if constexpr (d_mode == 4) {
      dest_alpha = _mm_xor_si128(_mm_set1_epi16(dst_alpha), mask_ff);
    }
  }

  if constexpr (tined) {
    tine_color = _mm_unpacklo_epi8(_mm_set1_epi32(tine), _mm_setzero_si128());
  }

  while (blocks16--) {
    uint16_t color;
    if constexpr (flip_x) {
      (*s_mem)--;
      color = **s_mem;
    } else {
      color = **s_mem;
      (*s_mem)++;
    }
    if constexpr (simple) {
      if constexpr (!transparent) {
        **d_mem = color;
        (*d_mem)++;
        continue;
      } else {
        if (color & 0x8000) {
          **d_mem = color;
        }
        (*d_mem)++;
        continue;
      }
    } else {
      if constexpr (transparent) {
        if (!(color & 0x8000)) {
          (*d_mem)++;
          continue;
        }
      }
      __m128i s_color;
      *(uint64_t *)&s_color = color;
      __m128i s_red = _mm_slli_epi64(s_color, 22);
      __m128i s_green = _mm_slli_epi64(s_color, 11);
      __m128i s_alpha = _mm_slli_epi16(_mm_srli_epi16(s_color, 15), 15);

      s_color = _mm_or_si128(s_color, _mm_or_si128(s_red, s_green));
      s_color = _mm_and_si128(s_color, mask_1f);
      s_color = _mm_slli_epi16(s_color, 3);

      if constexpr (tined) {
        s_color = _mm_srli_epi16(_mm_mullo_epi16(s_color, tine_color), 7);
      }

      if constexpr (blend) {
        __m128i d_color;
        *(uint64_t *)&d_color = **d_mem;
        __m128i d_red = _mm_slli_epi64(d_color, 22);
        __m128i d_green = _mm_slli_epi64(d_color, 11);

        d_color = _mm_or_si128(d_color, _mm_or_si128(d_red, d_green));
        d_color = _mm_and_si128(d_color, mask_1f);
        d_color = _mm_slli_epi16(d_color, 3);

        __m128i pm_s_color;
        __m128i pm_d_color;

        if constexpr (tined) {
          s_color = _mm_min_epi16(s_color, mask_ff);
        }

        if constexpr (d_mode == 0 || d_mode == 4) {
          pm_d_color = _mm_mullo_epi16(d_color, dest_alpha);
        } else if constexpr (d_mode == 1) {
          pm_d_color = _mm_mullo_epi16(d_color, s_color);
        } else if constexpr (d_mode == 2) {
          pm_d_color = _mm_mullo_epi16(d_color, d_color);
        } else if constexpr (d_mode == 3 || d_mode == 7) {
          pm_d_color = _mm_mullo_epi16(d_color, mask_ff);
        } else if constexpr (d_mode == 5) {
          pm_d_color =
              _mm_mullo_epi16(d_color, _mm_xor_si128(s_color, mask_ff));
        } else if constexpr (d_mode == 6) {
          pm_d_color =
              _mm_mullo_epi16(d_color, _mm_xor_si128(d_color, mask_ff));
        }

        if constexpr (s_mode == 0 || s_mode == 4) {
          pm_s_color = _mm_mullo_epi16(s_color, source_alpha);
        } else if constexpr (s_mode == 1) {
          pm_s_color = _mm_mullo_epi16(s_color, s_color);
        } else if constexpr (s_mode == 2) {
          pm_s_color = _mm_mullo_epi16(s_color, d_color);
        } else if constexpr (s_mode == 3 || s_mode == 7) {
          pm_s_color = _mm_mullo_epi16(s_color, mask_ff);
        } else if constexpr (s_mode == 5) {
          pm_s_color =
              _mm_mullo_epi16(s_color, _mm_xor_si128(s_color, mask_ff));
        } else if constexpr (s_mode == 6) {
          pm_s_color =
              _mm_mullo_epi16(s_color, _mm_xor_si128(d_color, mask_ff));
        }
        s_color = _mm_srli_epi16(_mm_adds_epu16(pm_d_color, pm_s_color), 11);
      } else {
        s_color = _mm_srli_epi16(s_color, 3);
      }
      s_color = _mm_min_epi16(s_color, mask_1f);

      __m128i red = _mm_srli_epi64(s_color, 22);
      __m128i green = _mm_srli_epi64(s_color, 11);

      s_color = _mm_or_si128(s_alpha,
                             _mm_or_si128(s_color, _mm_or_si128(green, red)));
      **d_mem = *(uint16_t *)&s_color;
      (*d_mem)++;
    }
  }
}

template <int simple, int blend, int flip_x, int tined, int transparent,
          int s_mode, int d_mode>
void Blitter::Block128(uint16_t **d_mem, uint16_t **s_mem, int blocks128,
                       uint8_t src_alpha, uint8_t dst_alpha, uint32_t tine) {
  if (blocks128 <= 0) return;

  __m128i mask_ff;
  __m128i source_alpha;
  __m128i dest_alpha;
  __m128i tine_red;
  __m128i tine_green;
  __m128i tine_blue;

  mask_ff = _mm_set1_epi16(0xff);

  if constexpr (blend) {
    if constexpr (s_mode == 0) {
      source_alpha = _mm_set1_epi16(src_alpha);
    } else if constexpr (s_mode == 4) {
      source_alpha = _mm_xor_si128(_mm_set1_epi16(src_alpha), mask_ff);
    }

    if constexpr (d_mode == 0) {
      dest_alpha = _mm_set1_epi16(dst_alpha);
    } else if constexpr (d_mode == 4) {
      dest_alpha = _mm_xor_si128(_mm_set1_epi16(dst_alpha), mask_ff);
    }
  }

  if constexpr (tined) {
    tine_red = _mm_set1_epi16((tine >> 16) & 0xff);
    tine_green = _mm_set1_epi16((tine >> 8) & 0xff);
    tine_blue = _mm_set1_epi16((tine >> 0) & 0xff);
  }

  while (blocks128--) {
    __m128i source;
    __m128i dest;
    if constexpr (flip_x) {
      (*s_mem) -= 8;
      source = _mm_loadu_si128(reinterpret_cast<__m128i *>(*s_mem));
      source = _mm_shuffle_epi32(source, _MM_SHUFFLE(0, 1, 2, 3));
      source = _mm_shufflelo_epi16(source, _MM_SHUFFLE(2, 3, 0, 1));
      source = _mm_shufflehi_epi16(source, _MM_SHUFFLE(2, 3, 0, 1));
    } else {
      source = _mm_loadu_si128(reinterpret_cast<__m128i *>(*s_mem));
      (*s_mem) += 8;
    }

    if constexpr (transparent) {
      dest = _mm_loadu_si128(reinterpret_cast<__m128i *>(*d_mem));
    }

    if constexpr (!simple) {
      __m128i s_red =
          _mm_and_si128(_mm_slli_epi16(_mm_srli_epi16(source, 10), 3), mask_ff);
      __m128i s_green =
          _mm_and_si128(_mm_slli_epi16(_mm_srli_epi16(source, 5), 3), mask_ff);
      __m128i s_blue = _mm_and_si128(_mm_slli_epi16(source, 3), mask_ff);
      __m128i s_alpha = _mm_slli_epi16(_mm_srli_epi16(source, 15), 15);

      if constexpr (tined) {
        s_red = _mm_srli_epi16(_mm_mullo_epi16(s_red, tine_red), 7);
        s_green = _mm_srli_epi16(_mm_mullo_epi16(s_green, tine_green), 7);
        s_blue = _mm_srli_epi16(_mm_mullo_epi16(s_blue, tine_blue), 7);
      }

      if constexpr (blend) {
        if constexpr (!transparent) {
          dest = _mm_loadu_si128(reinterpret_cast<__m128i *>(*d_mem));
        }

        if constexpr (tined) {
          s_red = _mm_min_epi16(s_red, mask_ff);
          s_green = _mm_min_epi16(s_green, mask_ff);
          s_blue = _mm_min_epi16(s_blue, mask_ff);
        }

        __m128i d_red =
            _mm_and_si128(_mm_slli_epi16(_mm_srli_epi16(dest, 10), 3), mask_ff);
        __m128i d_green =
            _mm_and_si128(_mm_slli_epi16(_mm_srli_epi16(dest, 5), 3), mask_ff);
        __m128i d_blue = _mm_and_si128(_mm_slli_epi16(dest, 3), mask_ff);

        __m128i pm_s_red;
        __m128i pm_s_green;
        __m128i pm_s_blue;
        __m128i pm_d_red;
        __m128i pm_d_green;
        __m128i pm_d_blue;

        if constexpr (d_mode == 0 || d_mode == 4) {
          pm_d_red = _mm_mullo_epi16(d_red, dest_alpha);
          pm_d_green = _mm_mullo_epi16(d_green, dest_alpha);
          pm_d_blue = _mm_mullo_epi16(d_blue, dest_alpha);
        } else if constexpr (d_mode == 1) {
          pm_d_red = _mm_mullo_epi16(d_red, s_red);
          pm_d_green = _mm_mullo_epi16(d_green, s_green);
          pm_d_blue = _mm_mullo_epi16(d_blue, s_blue);
        } else if constexpr (d_mode == 2) {
          pm_d_red = _mm_mullo_epi16(d_red, d_red);
          pm_d_green = _mm_mullo_epi16(d_green, d_green);
          pm_d_blue = _mm_mullo_epi16(d_blue, d_blue);
        } else if constexpr (d_mode == 3 || d_mode == 7) {
          pm_d_red = _mm_mullo_epi16(d_red, mask_ff);
          pm_d_green = _mm_mullo_epi16(d_green, mask_ff);
          pm_d_blue = _mm_mullo_epi16(d_blue, mask_ff);
        } else if constexpr (d_mode == 5) {
          pm_d_red = _mm_mullo_epi16(d_red, _mm_xor_si128(s_red, mask_ff));
          pm_d_green =
              _mm_mullo_epi16(d_green, _mm_xor_si128(s_green, mask_ff));
          pm_d_blue = _mm_mullo_epi16(d_blue, _mm_xor_si128(s_blue, mask_ff));
        } else if constexpr (d_mode == 6) {
          pm_d_red = _mm_mullo_epi16(d_red, _mm_xor_si128(d_red, mask_ff));
          pm_d_green =
              _mm_mullo_epi16(d_green, _mm_xor_si128(d_green, mask_ff));
          pm_d_blue = _mm_mullo_epi16(d_blue, _mm_xor_si128(d_blue, mask_ff));
        }

        if constexpr (s_mode == 0 || s_mode == 4) {
          pm_s_red = _mm_mullo_epi16(s_red, source_alpha);
          pm_s_green = _mm_mullo_epi16(s_green, source_alpha);
          pm_s_blue = _mm_mullo_epi16(s_blue, source_alpha);
        } else if constexpr (s_mode == 1) {
          pm_s_red = _mm_mullo_epi16(s_red, s_red);
          pm_s_green = _mm_mullo_epi16(s_green, s_green);
          pm_s_blue = _mm_mullo_epi16(s_blue, s_blue);
        } else if constexpr (s_mode == 2) {
          pm_s_red = _mm_mullo_epi16(s_red, d_red);
          pm_s_green = _mm_mullo_epi16(s_green, d_green);
          pm_s_blue = _mm_mullo_epi16(s_blue, d_blue);
        } else if constexpr (s_mode == 3 || s_mode == 7) {
          pm_s_red = _mm_mullo_epi16(s_red, mask_ff);
          pm_s_green = _mm_mullo_epi16(s_green, mask_ff);
          pm_s_blue = _mm_mullo_epi16(s_blue, mask_ff);
        } else if constexpr (s_mode == 5) {
          pm_s_red = _mm_mullo_epi16(s_red, _mm_xor_si128(s_red, mask_ff));
          pm_s_green =
              _mm_mullo_epi16(s_green, _mm_xor_si128(s_green, mask_ff));
          pm_s_blue = _mm_mullo_epi16(s_blue, _mm_xor_si128(s_blue, mask_ff));
        } else if constexpr (s_mode == 6) {
          pm_s_red = _mm_mullo_epi16(s_red, _mm_xor_si128(d_red, mask_ff));
          pm_s_green =
              _mm_mullo_epi16(s_green, _mm_xor_si128(d_green, mask_ff));
          pm_s_blue = _mm_mullo_epi16(s_blue, _mm_xor_si128(d_blue, mask_ff));
        }

        s_red = _mm_srli_epi16(_mm_adds_epu16(pm_s_red, pm_d_red), 11);
        s_green = _mm_srli_epi16(_mm_adds_epu16(pm_s_green, pm_d_green), 11);
        s_blue = _mm_srli_epi16(_mm_adds_epu16(pm_s_blue, pm_d_blue), 11);
      } else {
        s_red = _mm_srli_epi16(s_red, 3);
        s_green = _mm_srli_epi16(s_green, 3);
        s_blue = _mm_srli_epi16(s_blue, 3);
      }
      __m128i mask_1f = _mm_srli_epi16(mask_ff, 3);
      s_red = _mm_min_epi16(s_red, mask_1f);
      s_green = _mm_min_epi16(s_green, mask_1f);
      s_blue = _mm_min_epi16(s_blue, mask_1f);

      s_red = _mm_slli_epi16(s_red, 10);
      s_green = _mm_slli_epi16(s_green, 5);
      source = _mm_or_si128(s_alpha,
                            _mm_or_si128(s_blue, _mm_or_si128(s_red, s_green)));
    }

    if constexpr (transparent) {
      __m128i tr_mask = _mm_cmplt_epi16(source, _mm_setzero_si128());
      source = _mm_and_si128(tr_mask, source);
      dest = _mm_andnot_si128(tr_mask, dest);
      source = _mm_or_si128(source, dest);
    }
    _mm_storeu_si128(reinterpret_cast<__m128i *>(*d_mem), source);
    (*d_mem) += 8;
  }
}

template <int simple, int blend, int flip_x, int tined, int transparent,
          int s_mode, int d_mode>
void Blitter::Draw(int32_t src_x, int32_t src_y, int32_t x_start,
                   int32_t y_start, int32_t dimx, int32_t dimy, uint32_t flip_y,
                   uint8_t src_alpha, uint8_t dst_alpha, uint32_t tine) {
  int y, yf;

  if constexpr (flip_x) {
    src_x += (dimx - 1);
  }

  if (flip_y) {
    yf = -1;
    src_y += (dimy - 1);
  } else {
    yf = +1;
  }

  int starty = 0;
  const int y_end = y_start + dimy;

  if (y_start < clip_.min_y) starty = clip_.min_y - y_start;

  if (y_end > clip_.max_y) dimy -= (y_end - 1) - clip_.max_y;

  if constexpr (flip_x) {
    if ((src_x & 0x1fff) < ((src_x - (dimx - 1)) & 0x1fff)) {
      return;
    }
  } else {
    if ((src_x & 0x1fff) > ((src_x + (dimx - 1)) & 0x1fff)) {
      return;
    }
  }

  int startx = 0;
  const int x_end = x_start + dimx;

  if (x_start < clip_.min_x) startx = clip_.min_x - x_start;

  if (x_end > clip_.max_x) dimx -= (x_end - 1) - clip_.max_x;

  for (y = starty; y < dimy; y++) {
    uint16_t *d_mem =
        (uint16_t *)&gpu_[((y_start + y) * kSizeX + x_start + startx) * 2];
    uint16_t *s_mem =
        (uint16_t *)&gpu_[((src_y + yf * y) & 0x0fff) * kSizeX * 2];

    if constexpr (flip_x) {
      s_mem += (src_x - startx + 1);
    } else {
      s_mem += (src_x + startx);
    }

    int width = dimx - startx;

    if (width <= 0) return;

    Block128<simple, blend, flip_x, tined, transparent, s_mode, d_mode>(
        &d_mem, &s_mem, width >> 3, src_alpha, dst_alpha, tine);

    Block16<simple, blend, flip_x, tined, transparent, s_mode, d_mode>(
        &d_mem, &s_mem, width & 7, src_alpha, dst_alpha, tine);
  }
}

void Blitter::Draw(uint32_t &addr) {
  int32_t attribute = Next16(addr);
  int32_t alpha = Next16(addr);
  int32_t src_x = Next16(addr) & 0x1fff;
  int32_t src_y = Next16(addr) & 0x1fff;
  int32_t x_start = Next16(addr);
  int32_t y_start = Next16(addr);

  int32_t w = Next16(addr);
  int32_t h = Next16(addr);
  int32_t tine = Next32(addr);

  int32_t d_mode = (attribute & 0x0007);
  int32_t s_mode = (attribute & 0x0070) >> 4;

  int32_t transparent = attribute & 0x0100;
  int32_t blend = attribute & 0x0200;

  int32_t flip_y = attribute & 0x0400;
  int32_t flip_x = attribute & 0x0800;

  uint8_t d_alpha = (alpha & 0x00ff);
  uint8_t s_alpha = (alpha & 0xff00) >> 8;

  int32_t x = (x_start & 0x7fff) - (x_start & 0x8000);
  int32_t y = (y_start & 0x7fff) - (y_start & 0x8000);

  int32_t dimx = (w & 0x1fff) + 1;
  int32_t dimy = (h & 0x0fff) + 1;

  int tinted = 0;
  if ((tine & 0x00ffffff) != 0x00808080) {
    tinted = 1;
  }

  if ((s_mode == 0 && s_alpha == 0x1f) && (d_mode == 4 && d_alpha == 0x1f)) {
    blend = 0;
  }

  if (tinted) {
    if (!flip_x) {
      if (transparent) {
        if (!blend) {
          Draw<0, 0, 0, 1, 1, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          (this->*DrawNonFlipTinedTransparentBlend[s_mode | (d_mode << 3)])(
              src_x, src_y, x, y, dimx, dimy, flip_y, s_alpha, d_alpha, tine);
        }
      } else {
        if (!blend) {
          Draw<0, 0, 0, 1, 0, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          (this->*DrawNonFlipTinedNonTransparentBlend[s_mode | (d_mode << 3)])(
              src_x, src_y, x, y, dimx, dimy, flip_y, s_alpha, d_alpha, tine);
        }
      }
    } else {
      if (transparent) {
        if (!blend) {
          Draw<0, 0, 1, 1, 1, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          (this->*DrawFlipTinedTransparentBlend[s_mode | (d_mode << 3)])(
              src_x, src_y, x, y, dimx, dimy, flip_y, s_alpha, d_alpha, tine);
        }
      } else {
        if (!blend) {
          Draw<0, 0, 1, 1, 0, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          (this->*DrawFlipTinedNonTransparentBlend[s_mode | (d_mode << 3)])(
              src_x, src_y, x, y, dimx, dimy, flip_y, s_alpha, d_alpha, tine);
        }
      }
    }
  } else {
    if (!blend && !tinted) {
      if (!flip_x) {
        if (transparent) {
          Draw<1, 0, 0, 0, 1, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          Draw<1, 0, 0, 0, 0, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        }
      } else {
        if (transparent) {
          Draw<1, 0, 1, 0, 1, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          Draw<1, 0, 1, 0, 0, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        }
      }
      return;
    }

    if (!flip_x) {
      if (transparent) {
        if (!blend) {
          Draw<0, 0, 0, 0, 1, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          (this->*DrawNonFlipNonTinedTransparentBlend[s_mode | (d_mode << 3)])(
              src_x, src_y, x, y, dimx, dimy, flip_y, s_alpha, d_alpha, tine);
        }
      } else {
        if (!blend) {
          Draw<0, 0, 0, 0, 0, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          (this->*DrawNonFlipNonTinedNonTransparentBlend[s_mode |
                                                         (d_mode << 3)])(
              src_x, src_y, x, y, dimx, dimy, flip_y, s_alpha, d_alpha, tine);
        }
      }
    } else {
      if (transparent) {
        if (!blend) {
          Draw<0, 0, 1, 0, 1, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          (this->*DrawFlipNonTinedTransparentBlend[s_mode | (d_mode << 3)])(
              src_x, src_y, x, y, dimx, dimy, flip_y, s_alpha, d_alpha, tine);
        }
      } else {
        if (!blend) {
          Draw<0, 0, 1, 0, 0, 0, 0>(src_x, src_y, x, y, dimx, dimy, flip_y,
                                    s_alpha, d_alpha, tine);
        } else {
          (this->*DrawFlipNonTinedNonTransparentBlend[s_mode | (d_mode << 3)])(
              src_x, src_y, x, y, dimx, dimy, flip_y, s_alpha, d_alpha, tine);
        }
      }
    }
  }
}

void Blitter::Run() {
  bool clip_type = true;

  uint32_t addr = Read<uint32_t>(0x0008);

  clip_.min_x = Read<uint32_t>(0x0040);
  clip_.min_y = Read<uint32_t>(0x0044);
  clip_.max_x = clip_.min_x + 320 - 1;
  clip_.max_y = clip_.min_y + 240 - 1;

  while (true) {
    uint16_t value = Next16(addr);

    value &= 0xf000;

    if (value == 0x0000 || value == 0xf000) {
      break;
    } else if (value == 0xc000) {
      value = Next16(addr);
      clip_type = value ? true : false;

      if (clip_type) {
        clip_.min_x = Read<uint32_t>(0x0040);
        clip_.min_y = Read<uint32_t>(0x0044);
        clip_.max_x = clip_.min_x + 320 - 1;
        clip_.max_y = clip_.min_y + 240 - 1;
      } else {
        clip_.min_x = 0;
        clip_.min_y = 0;
        clip_.max_x = 0x2000 - 1;
        clip_.max_y = 0x1000 - 1;
      }
    }

    else if (value == 0x2000) {
      Upload(addr);
    } else if (value == 0x1000) {
      addr -= 2;
      Draw(addr);
      uint32_t offsetx = Read<uint32_t>(0x0014);
      uint32_t offsety = Read<uint32_t>(0x0018);
      uint32_t offx = offsetx + (offsety * kSizeX);

      uint16_t *d = reinterpret_cast<uint16_t *>(screen_.data());
      for (uint32_t y = 0; y < kHeight; y++, offx += kSizeX) {
        uint16_t *s = reinterpret_cast<uint16_t *>(gpu_.data()) + offx;
        for (uint32_t x = 0; x < kWidth; x += 8, d += 8) {
          __m128i value = _mm_loadu_si128(reinterpret_cast<__m128i *>(&s[x]));
          __m128i rgb = _mm_slli_epi16(value, 1);
          __m128i alpha = _mm_srli_epi16(value, 15);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(d),
                           _mm_or_si128(rgb, alpha));
        }
      }
    }
  }
}

void Blitter::Blit() {
  while (true) {
    std::unique_lock lock(blit_mutex_);
    blit_cv_.wait(lock, [this] { return blitting_ || !running_; });
    if (!running_) {
      break;
    }
    Run();
    blitting_ = false;
    lock.unlock();
    blit_cv_.notify_one();
  }
}

void Blitter::Vblank() { irq_(sh3::kIrl2); }

void Blitter::BlitIrq() {
  std::unique_lock lock(blit_mutex_);
  blit_cv_.wait(lock, [this] { return !blitting_; });

  Write<uint32_t>(0x0004, 0);
  irq_(sh3::kIrl1);
}

void Blitter::Init(std::function<void(int32_t)> irq_callbak) {
  irq_ = irq_callbak;
  counters.Insert(v_sync_);
}

uint8_t Blitter::Read8(uint32_t addr) {
  switch (addr & 0xffff) {
    case 0x0050:
      return 0xfe;
    default:
      return -1;
  }
}

uint32_t Blitter::Read32(uint32_t addr) {
  switch (addr & 0xffff) {
    case 0x0000:
      return 0x20051119;
    case 0x0010:
      return (Read<uint32_t>(0x0004) == 1) ? 0x00 : 0x10;
    case 0x0024:
      return 0;
    case 0x0050:
      return 0xfe;
    default:
      return -1;
  }
}

void Blitter::Write8(uint32_t addr, uint8_t value) {
  Write<uint8_t>(addr, value);
}

void Blitter::Write32(uint32_t addr, uint32_t value) {
  Write<uint32_t>(addr, value);

  switch (addr & 0xffff) {
    case 0x0004:
      if (value) {
        std::lock_guard lk(blit_mutex_);
        blitting_ = true;
        blit_cv_.notify_one();
        counters.Insert(blit_irq_);
      }
      break;
    case 0x0024:
      if (value & 1) irq_(0 - sh3::kIrl2);
      if (value & 2) irq_(0 - sh3::kIrl1);
      break;
  }
}