#pragma once

#include <array>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <functional>
#include <span>
#include <thread>

#include "counters.h"

struct Clip {
  int32_t min_x;
  int32_t min_y;
  int32_t max_x;
  int32_t max_y;
};

class Blitter {
 public:
  Blitter(std::span<uint8_t> ram, counters::Counters &c);
  ~Blitter();

  void Init(std::function<void(int32_t)> irq_callbak);
  uint16_t *GetBlitterData() { return screen_.data(); }

  uint8_t Read8(uint32_t addr);
  uint32_t Read32(uint32_t addr);

  void Write8(uint32_t addr, uint8_t value);
  void Write32(uint32_t addr, uint32_t value);

 private:
  enum : uint32_t {
    kBlockSize = 256,
    kSizeX = kBlockSize * 32,
    kSizeY = kBlockSize * 16,
    kVramSize = kSizeX * kSizeY * 2,
    kWidth = 320,
    kHeight = 240
  };

  bool running_;
  bool blitting_;
  std::thread *blit_thread_;
  std::mutex blit_mutex_;
  std::condition_variable blit_cv_;

  Clip clip_;
  void Run();
  void Upload(uint32_t &addr);
  void Draw(uint32_t &addr);

  counters::Counters &counters;

  std::span<uint8_t> ram_;
  std::array<uint8_t, kVramSize> gpu_;
  std::array<uint8_t, 0x00000100> gpu_regs_;
  std::function<void(int32_t)> irq_;
  std::array<uint16_t, kWidth * kHeight> screen_;

  template <typename T>
  T Read(uint32_t addr) {
    addr &= gpu_regs_.size() - 1;
    return *(T *)&gpu_regs_[addr];
  }

  template <typename T>
  void Write(uint32_t addr, T value) {
    addr &= gpu_regs_.size() - 1;
    *(T *)&gpu_regs_[addr] = value;
  }

  uint16_t Next16(uint32_t &addr) {
    addr &= ram_.size() - 1;
    uint16_t v = *(uint16_t *)&ram_[ram_.size() - addr - 2];
    addr += 2;
    return v;
  }

  uint32_t Next32(uint32_t &addr) {
    addr &= ram_.size() - 1;
    uint32_t v = *(uint32_t *)&ram_[ram_.size() - addr - 4];
    addr += 4;
    return v;
  }

  void Vblank();
  void BlitIrq();

  void Blit();

  counters::Counter *v_sync_;
  counters::Counter *blit_irq_;

  template <int simple, int blend, int flip_x, int tined, int transparent,
            int s_mode, int d_mode>
  void Block128(uint16_t **d_mem, uint16_t **s_mem, int blocks128,
                uint8_t src_alpha, uint8_t dst_alpha, uint32_t tine);

  template <int simple, int blend, int flip_x, int tined, int transparent,
            int s_mode, int d_mode>
  void Block16(uint16_t **d_mem, uint16_t **s_mem, int blocks16,
               uint8_t src_alpha, uint8_t dst_alpha, uint32_t tine);

  template <int simple, int blend, int flip_x, int tined, int transparent,
            int s_mode, int d_mode>
  void Draw(int32_t src_x, int32_t src_y, int32_t x_start, int32_t y_start,
            int32_t dimx, int32_t dimy, uint32_t flip_y, uint8_t s_alpha,
            uint8_t d_alpha, uint32_t tine);

  typedef void (Blitter::*DrawMode)(int32_t src_x, int32_t src_y,
                                    int32_t x_start, int32_t y_start,
                                    int32_t dimx, int32_t dimy, uint32_t flip_y,
                                    uint8_t s_alpha, uint8_t d_alpha,
                                    uint32_t tine);

  static constexpr DrawMode DrawNonFlipTinedTransparentBlend[64] = {
      &Blitter::Draw<0, 1, 0, 1, 1, 0, 0>, &Blitter::Draw<0, 1, 0, 1, 1, 1, 0>,
      &Blitter::Draw<0, 1, 0, 1, 1, 2, 0>, &Blitter::Draw<0, 1, 0, 1, 1, 3, 0>,
      &Blitter::Draw<0, 1, 0, 1, 1, 4, 0>, &Blitter::Draw<0, 1, 0, 1, 1, 5, 0>,
      &Blitter::Draw<0, 1, 0, 1, 1, 6, 0>, &Blitter::Draw<0, 1, 0, 1, 1, 7, 0>,
      &Blitter::Draw<0, 1, 0, 1, 1, 0, 1>, &Blitter::Draw<0, 1, 0, 1, 1, 1, 1>,
      &Blitter::Draw<0, 1, 0, 1, 1, 2, 1>, &Blitter::Draw<0, 1, 0, 1, 1, 3, 1>,
      &Blitter::Draw<0, 1, 0, 1, 1, 4, 1>, &Blitter::Draw<0, 1, 0, 1, 1, 5, 1>,
      &Blitter::Draw<0, 1, 0, 1, 1, 6, 1>, &Blitter::Draw<0, 1, 0, 1, 1, 7, 1>,
      &Blitter::Draw<0, 1, 0, 1, 1, 0, 2>, &Blitter::Draw<0, 1, 0, 1, 1, 1, 2>,
      &Blitter::Draw<0, 1, 0, 1, 1, 2, 2>, &Blitter::Draw<0, 1, 0, 1, 1, 3, 2>,
      &Blitter::Draw<0, 1, 0, 1, 1, 4, 2>, &Blitter::Draw<0, 1, 0, 1, 1, 5, 2>,
      &Blitter::Draw<0, 1, 0, 1, 1, 6, 2>, &Blitter::Draw<0, 1, 0, 1, 1, 7, 2>,
      &Blitter::Draw<0, 1, 0, 1, 1, 0, 3>, &Blitter::Draw<0, 1, 0, 1, 1, 1, 3>,
      &Blitter::Draw<0, 1, 0, 1, 1, 2, 3>, &Blitter::Draw<0, 1, 0, 1, 1, 3, 3>,
      &Blitter::Draw<0, 1, 0, 1, 1, 4, 3>, &Blitter::Draw<0, 1, 0, 1, 1, 5, 3>,
      &Blitter::Draw<0, 1, 0, 1, 1, 6, 3>, &Blitter::Draw<0, 1, 0, 1, 1, 7, 3>,
      &Blitter::Draw<0, 1, 0, 1, 1, 0, 4>, &Blitter::Draw<0, 1, 0, 1, 1, 1, 4>,
      &Blitter::Draw<0, 1, 0, 1, 1, 2, 4>, &Blitter::Draw<0, 1, 0, 1, 1, 3, 4>,
      &Blitter::Draw<0, 1, 0, 1, 1, 4, 4>, &Blitter::Draw<0, 1, 0, 1, 1, 5, 4>,
      &Blitter::Draw<0, 1, 0, 1, 1, 6, 4>, &Blitter::Draw<0, 1, 0, 1, 1, 7, 4>,
      &Blitter::Draw<0, 1, 0, 1, 1, 0, 5>, &Blitter::Draw<0, 1, 0, 1, 1, 1, 5>,
      &Blitter::Draw<0, 1, 0, 1, 1, 2, 5>, &Blitter::Draw<0, 1, 0, 1, 1, 3, 5>,
      &Blitter::Draw<0, 1, 0, 1, 1, 4, 5>, &Blitter::Draw<0, 1, 0, 1, 1, 5, 5>,
      &Blitter::Draw<0, 1, 0, 1, 1, 6, 5>, &Blitter::Draw<0, 1, 0, 1, 1, 7, 5>,
      &Blitter::Draw<0, 1, 0, 1, 1, 0, 6>, &Blitter::Draw<0, 1, 0, 1, 1, 1, 6>,
      &Blitter::Draw<0, 1, 0, 1, 1, 2, 6>, &Blitter::Draw<0, 1, 0, 1, 1, 3, 6>,
      &Blitter::Draw<0, 1, 0, 1, 1, 4, 6>, &Blitter::Draw<0, 1, 0, 1, 1, 5, 6>,
      &Blitter::Draw<0, 1, 0, 1, 1, 6, 6>, &Blitter::Draw<0, 1, 0, 1, 1, 7, 6>,
      &Blitter::Draw<0, 1, 0, 1, 1, 0, 7>, &Blitter::Draw<0, 1, 0, 1, 1, 1, 7>,
      &Blitter::Draw<0, 1, 0, 1, 1, 2, 7>, &Blitter::Draw<0, 1, 0, 1, 1, 3, 7>,
      &Blitter::Draw<0, 1, 0, 1, 1, 4, 7>, &Blitter::Draw<0, 1, 0, 1, 1, 5, 7>,
      &Blitter::Draw<0, 1, 0, 1, 1, 6, 7>, &Blitter::Draw<0, 1, 0, 1, 1, 7, 7>,
  };

  static constexpr DrawMode DrawNonFlipTinedNonTransparentBlend[64] = {
      &Blitter::Draw<0, 1, 0, 1, 0, 0, 0>, &Blitter::Draw<0, 1, 0, 1, 0, 1, 0>,
      &Blitter::Draw<0, 1, 0, 1, 0, 2, 0>, &Blitter::Draw<0, 1, 0, 1, 0, 3, 0>,
      &Blitter::Draw<0, 1, 0, 1, 0, 4, 0>, &Blitter::Draw<0, 1, 0, 1, 0, 5, 0>,
      &Blitter::Draw<0, 1, 0, 1, 0, 6, 0>, &Blitter::Draw<0, 1, 0, 1, 0, 7, 0>,
      &Blitter::Draw<0, 1, 0, 1, 0, 0, 1>, &Blitter::Draw<0, 1, 0, 1, 0, 1, 1>,
      &Blitter::Draw<0, 1, 0, 1, 0, 2, 1>, &Blitter::Draw<0, 1, 0, 1, 0, 3, 1>,
      &Blitter::Draw<0, 1, 0, 1, 0, 4, 1>, &Blitter::Draw<0, 1, 0, 1, 0, 5, 1>,
      &Blitter::Draw<0, 1, 0, 1, 0, 6, 1>, &Blitter::Draw<0, 1, 0, 1, 0, 7, 1>,
      &Blitter::Draw<0, 1, 0, 1, 0, 0, 2>, &Blitter::Draw<0, 1, 0, 1, 0, 1, 2>,
      &Blitter::Draw<0, 1, 0, 1, 0, 2, 2>, &Blitter::Draw<0, 1, 0, 1, 0, 3, 2>,
      &Blitter::Draw<0, 1, 0, 1, 0, 4, 2>, &Blitter::Draw<0, 1, 0, 1, 0, 5, 2>,
      &Blitter::Draw<0, 1, 0, 1, 0, 6, 2>, &Blitter::Draw<0, 1, 0, 1, 0, 7, 2>,
      &Blitter::Draw<0, 1, 0, 1, 0, 0, 3>, &Blitter::Draw<0, 1, 0, 1, 0, 1, 3>,
      &Blitter::Draw<0, 1, 0, 1, 0, 2, 3>, &Blitter::Draw<0, 1, 0, 1, 0, 3, 3>,
      &Blitter::Draw<0, 1, 0, 1, 0, 4, 3>, &Blitter::Draw<0, 1, 0, 1, 0, 5, 3>,
      &Blitter::Draw<0, 1, 0, 1, 0, 6, 3>, &Blitter::Draw<0, 1, 0, 1, 0, 7, 3>,
      &Blitter::Draw<0, 1, 0, 1, 0, 0, 4>, &Blitter::Draw<0, 1, 0, 1, 0, 1, 4>,
      &Blitter::Draw<0, 1, 0, 1, 0, 2, 4>, &Blitter::Draw<0, 1, 0, 1, 0, 3, 4>,
      &Blitter::Draw<0, 1, 0, 1, 0, 4, 4>, &Blitter::Draw<0, 1, 0, 1, 0, 5, 4>,
      &Blitter::Draw<0, 1, 0, 1, 0, 6, 4>, &Blitter::Draw<0, 1, 0, 1, 0, 7, 4>,
      &Blitter::Draw<0, 1, 0, 1, 0, 0, 5>, &Blitter::Draw<0, 1, 0, 1, 0, 1, 5>,
      &Blitter::Draw<0, 1, 0, 1, 0, 2, 5>, &Blitter::Draw<0, 1, 0, 1, 0, 3, 5>,
      &Blitter::Draw<0, 1, 0, 1, 0, 4, 5>, &Blitter::Draw<0, 1, 0, 1, 0, 5, 5>,
      &Blitter::Draw<0, 1, 0, 1, 0, 6, 5>, &Blitter::Draw<0, 1, 0, 1, 0, 7, 5>,
      &Blitter::Draw<0, 1, 0, 1, 0, 0, 6>, &Blitter::Draw<0, 1, 0, 1, 0, 1, 6>,
      &Blitter::Draw<0, 1, 0, 1, 0, 2, 6>, &Blitter::Draw<0, 1, 0, 1, 0, 3, 6>,
      &Blitter::Draw<0, 1, 0, 1, 0, 4, 6>, &Blitter::Draw<0, 1, 0, 1, 0, 5, 6>,
      &Blitter::Draw<0, 1, 0, 1, 0, 6, 6>, &Blitter::Draw<0, 1, 0, 1, 0, 7, 6>,
      &Blitter::Draw<0, 1, 0, 1, 0, 0, 7>, &Blitter::Draw<0, 1, 0, 1, 0, 1, 7>,
      &Blitter::Draw<0, 1, 0, 1, 0, 2, 7>, &Blitter::Draw<0, 1, 0, 1, 0, 3, 7>,
      &Blitter::Draw<0, 1, 0, 1, 0, 4, 7>, &Blitter::Draw<0, 1, 0, 1, 0, 5, 7>,
      &Blitter::Draw<0, 1, 0, 1, 0, 6, 7>, &Blitter::Draw<0, 1, 0, 1, 0, 7, 7>,
  };

  static constexpr DrawMode DrawFlipTinedTransparentBlend[64] = {
      &Blitter::Draw<0, 1, 1, 1, 1, 0, 0>, &Blitter::Draw<0, 1, 1, 1, 1, 1, 0>,
      &Blitter::Draw<0, 1, 1, 1, 1, 2, 0>, &Blitter::Draw<0, 1, 1, 1, 1, 3, 0>,
      &Blitter::Draw<0, 1, 1, 1, 1, 4, 0>, &Blitter::Draw<0, 1, 1, 1, 1, 5, 0>,
      &Blitter::Draw<0, 1, 1, 1, 1, 6, 0>, &Blitter::Draw<0, 1, 1, 1, 1, 7, 0>,
      &Blitter::Draw<0, 1, 1, 1, 1, 0, 1>, &Blitter::Draw<0, 1, 1, 1, 1, 1, 1>,
      &Blitter::Draw<0, 1, 1, 1, 1, 2, 1>, &Blitter::Draw<0, 1, 1, 1, 1, 3, 1>,
      &Blitter::Draw<0, 1, 1, 1, 1, 4, 1>, &Blitter::Draw<0, 1, 1, 1, 1, 5, 1>,
      &Blitter::Draw<0, 1, 1, 1, 1, 6, 1>, &Blitter::Draw<0, 1, 1, 1, 1, 7, 1>,
      &Blitter::Draw<0, 1, 1, 1, 1, 0, 2>, &Blitter::Draw<0, 1, 1, 1, 1, 1, 2>,
      &Blitter::Draw<0, 1, 1, 1, 1, 2, 2>, &Blitter::Draw<0, 1, 1, 1, 1, 3, 2>,
      &Blitter::Draw<0, 1, 1, 1, 1, 4, 2>, &Blitter::Draw<0, 1, 1, 1, 1, 5, 2>,
      &Blitter::Draw<0, 1, 1, 1, 1, 6, 2>, &Blitter::Draw<0, 1, 1, 1, 1, 7, 2>,
      &Blitter::Draw<0, 1, 1, 1, 1, 0, 3>, &Blitter::Draw<0, 1, 1, 1, 1, 1, 3>,
      &Blitter::Draw<0, 1, 1, 1, 1, 2, 3>, &Blitter::Draw<0, 1, 1, 1, 1, 3, 3>,
      &Blitter::Draw<0, 1, 1, 1, 1, 4, 3>, &Blitter::Draw<0, 1, 1, 1, 1, 5, 3>,
      &Blitter::Draw<0, 1, 1, 1, 1, 6, 3>, &Blitter::Draw<0, 1, 1, 1, 1, 7, 3>,
      &Blitter::Draw<0, 1, 1, 1, 1, 0, 4>, &Blitter::Draw<0, 1, 1, 1, 1, 1, 4>,
      &Blitter::Draw<0, 1, 1, 1, 1, 2, 4>, &Blitter::Draw<0, 1, 1, 1, 1, 3, 4>,
      &Blitter::Draw<0, 1, 1, 1, 1, 4, 4>, &Blitter::Draw<0, 1, 1, 1, 1, 5, 4>,
      &Blitter::Draw<0, 1, 1, 1, 1, 6, 4>, &Blitter::Draw<0, 1, 1, 1, 1, 7, 4>,
      &Blitter::Draw<0, 1, 1, 1, 1, 0, 5>, &Blitter::Draw<0, 1, 1, 1, 1, 1, 5>,
      &Blitter::Draw<0, 1, 1, 1, 1, 2, 5>, &Blitter::Draw<0, 1, 1, 1, 1, 3, 5>,
      &Blitter::Draw<0, 1, 1, 1, 1, 4, 5>, &Blitter::Draw<0, 1, 1, 1, 1, 5, 5>,
      &Blitter::Draw<0, 1, 1, 1, 1, 6, 5>, &Blitter::Draw<0, 1, 1, 1, 1, 7, 5>,
      &Blitter::Draw<0, 1, 1, 1, 1, 0, 6>, &Blitter::Draw<0, 1, 1, 1, 1, 1, 6>,
      &Blitter::Draw<0, 1, 1, 1, 1, 2, 6>, &Blitter::Draw<0, 1, 1, 1, 1, 3, 6>,
      &Blitter::Draw<0, 1, 1, 1, 1, 4, 6>, &Blitter::Draw<0, 1, 1, 1, 1, 5, 6>,
      &Blitter::Draw<0, 1, 1, 1, 1, 6, 6>, &Blitter::Draw<0, 1, 1, 1, 1, 7, 6>,
      &Blitter::Draw<0, 1, 1, 1, 1, 0, 7>, &Blitter::Draw<0, 1, 1, 1, 1, 1, 7>,
      &Blitter::Draw<0, 1, 1, 1, 1, 2, 7>, &Blitter::Draw<0, 1, 1, 1, 1, 3, 7>,
      &Blitter::Draw<0, 1, 1, 1, 1, 4, 7>, &Blitter::Draw<0, 1, 1, 1, 1, 5, 7>,
      &Blitter::Draw<0, 1, 1, 1, 1, 6, 7>, &Blitter::Draw<0, 1, 1, 1, 1, 7, 7>,
  };

  static constexpr DrawMode DrawFlipTinedNonTransparentBlend[64] = {
      &Blitter::Draw<0, 1, 1, 1, 0, 0, 0>, &Blitter::Draw<0, 1, 1, 1, 0, 1, 0>,
      &Blitter::Draw<0, 1, 1, 1, 0, 2, 0>, &Blitter::Draw<0, 1, 1, 1, 0, 3, 0>,
      &Blitter::Draw<0, 1, 1, 1, 0, 4, 0>, &Blitter::Draw<0, 1, 1, 1, 0, 5, 0>,
      &Blitter::Draw<0, 1, 1, 1, 0, 6, 0>, &Blitter::Draw<0, 1, 1, 1, 0, 7, 0>,
      &Blitter::Draw<0, 1, 1, 1, 0, 0, 1>, &Blitter::Draw<0, 1, 1, 1, 0, 1, 1>,
      &Blitter::Draw<0, 1, 1, 1, 0, 2, 1>, &Blitter::Draw<0, 1, 1, 1, 0, 3, 1>,
      &Blitter::Draw<0, 1, 1, 1, 0, 4, 1>, &Blitter::Draw<0, 1, 1, 1, 0, 5, 1>,
      &Blitter::Draw<0, 1, 1, 1, 0, 6, 1>, &Blitter::Draw<0, 1, 1, 1, 0, 7, 1>,
      &Blitter::Draw<0, 1, 1, 1, 0, 0, 2>, &Blitter::Draw<0, 1, 1, 1, 0, 1, 2>,
      &Blitter::Draw<0, 1, 1, 1, 0, 2, 2>, &Blitter::Draw<0, 1, 1, 1, 0, 3, 2>,
      &Blitter::Draw<0, 1, 1, 1, 0, 4, 2>, &Blitter::Draw<0, 1, 1, 1, 0, 5, 2>,
      &Blitter::Draw<0, 1, 1, 1, 0, 6, 2>, &Blitter::Draw<0, 1, 1, 1, 0, 7, 2>,
      &Blitter::Draw<0, 1, 1, 1, 0, 0, 3>, &Blitter::Draw<0, 1, 1, 1, 0, 1, 3>,
      &Blitter::Draw<0, 1, 1, 1, 0, 2, 3>, &Blitter::Draw<0, 1, 1, 1, 0, 3, 3>,
      &Blitter::Draw<0, 1, 1, 1, 0, 4, 3>, &Blitter::Draw<0, 1, 1, 1, 0, 5, 3>,
      &Blitter::Draw<0, 1, 1, 1, 0, 6, 3>, &Blitter::Draw<0, 1, 1, 1, 0, 7, 3>,
      &Blitter::Draw<0, 1, 1, 1, 0, 0, 4>, &Blitter::Draw<0, 1, 1, 1, 0, 1, 4>,
      &Blitter::Draw<0, 1, 1, 1, 0, 2, 4>, &Blitter::Draw<0, 1, 1, 1, 0, 3, 4>,
      &Blitter::Draw<0, 1, 1, 1, 0, 4, 4>, &Blitter::Draw<0, 1, 1, 1, 0, 5, 4>,
      &Blitter::Draw<0, 1, 1, 1, 0, 6, 4>, &Blitter::Draw<0, 1, 1, 1, 0, 7, 4>,
      &Blitter::Draw<0, 1, 1, 1, 0, 0, 5>, &Blitter::Draw<0, 1, 1, 1, 0, 1, 5>,
      &Blitter::Draw<0, 1, 1, 1, 0, 2, 5>, &Blitter::Draw<0, 1, 1, 1, 0, 3, 5>,
      &Blitter::Draw<0, 1, 1, 1, 0, 4, 5>, &Blitter::Draw<0, 1, 1, 1, 0, 5, 5>,
      &Blitter::Draw<0, 1, 1, 1, 0, 6, 5>, &Blitter::Draw<0, 1, 1, 1, 0, 7, 5>,
      &Blitter::Draw<0, 1, 1, 1, 0, 0, 6>, &Blitter::Draw<0, 1, 1, 1, 0, 1, 6>,
      &Blitter::Draw<0, 1, 1, 1, 0, 2, 6>, &Blitter::Draw<0, 1, 1, 1, 0, 3, 6>,
      &Blitter::Draw<0, 1, 1, 1, 0, 4, 6>, &Blitter::Draw<0, 1, 1, 1, 0, 5, 6>,
      &Blitter::Draw<0, 1, 1, 1, 0, 6, 6>, &Blitter::Draw<0, 1, 1, 1, 0, 7, 6>,
      &Blitter::Draw<0, 1, 1, 1, 0, 0, 7>, &Blitter::Draw<0, 1, 1, 1, 0, 1, 7>,
      &Blitter::Draw<0, 1, 1, 1, 0, 2, 7>, &Blitter::Draw<0, 1, 1, 1, 0, 3, 7>,
      &Blitter::Draw<0, 1, 1, 1, 0, 4, 7>, &Blitter::Draw<0, 1, 1, 1, 0, 5, 7>,
      &Blitter::Draw<0, 1, 1, 1, 0, 6, 7>, &Blitter::Draw<0, 1, 1, 1, 0, 7, 7>,
  };

  static constexpr DrawMode DrawNonFlipNonTinedTransparentBlend[64] = {
      &Blitter::Draw<0, 1, 0, 0, 1, 0, 0>, &Blitter::Draw<0, 1, 0, 0, 1, 1, 0>,
      &Blitter::Draw<0, 1, 0, 0, 1, 2, 0>, &Blitter::Draw<0, 1, 0, 0, 1, 3, 0>,
      &Blitter::Draw<0, 1, 0, 0, 1, 4, 0>, &Blitter::Draw<0, 1, 0, 0, 1, 5, 0>,
      &Blitter::Draw<0, 1, 0, 0, 1, 6, 0>, &Blitter::Draw<0, 1, 0, 0, 1, 7, 0>,
      &Blitter::Draw<0, 1, 0, 0, 1, 0, 1>, &Blitter::Draw<0, 1, 0, 0, 1, 1, 1>,
      &Blitter::Draw<0, 1, 0, 0, 1, 2, 1>, &Blitter::Draw<0, 1, 0, 0, 1, 3, 1>,
      &Blitter::Draw<0, 1, 0, 0, 1, 4, 1>, &Blitter::Draw<0, 1, 0, 0, 1, 5, 1>,
      &Blitter::Draw<0, 1, 0, 0, 1, 6, 1>, &Blitter::Draw<0, 1, 0, 0, 1, 7, 1>,
      &Blitter::Draw<0, 1, 0, 0, 1, 0, 2>, &Blitter::Draw<0, 1, 0, 0, 1, 1, 2>,
      &Blitter::Draw<0, 1, 0, 0, 1, 2, 2>, &Blitter::Draw<0, 1, 0, 0, 1, 3, 2>,
      &Blitter::Draw<0, 1, 0, 0, 1, 4, 2>, &Blitter::Draw<0, 1, 0, 0, 1, 5, 2>,
      &Blitter::Draw<0, 1, 0, 0, 1, 6, 2>, &Blitter::Draw<0, 1, 0, 0, 1, 7, 2>,
      &Blitter::Draw<0, 1, 0, 0, 1, 0, 3>, &Blitter::Draw<0, 1, 0, 0, 1, 1, 3>,
      &Blitter::Draw<0, 1, 0, 0, 1, 2, 3>, &Blitter::Draw<0, 1, 0, 0, 1, 3, 3>,
      &Blitter::Draw<0, 1, 0, 0, 1, 4, 3>, &Blitter::Draw<0, 1, 0, 0, 1, 5, 3>,
      &Blitter::Draw<0, 1, 0, 0, 1, 6, 3>, &Blitter::Draw<0, 1, 0, 0, 1, 7, 3>,
      &Blitter::Draw<0, 1, 0, 0, 1, 0, 4>, &Blitter::Draw<0, 1, 0, 0, 1, 1, 4>,
      &Blitter::Draw<0, 1, 0, 0, 1, 2, 4>, &Blitter::Draw<0, 1, 0, 0, 1, 3, 4>,
      &Blitter::Draw<0, 1, 0, 0, 1, 4, 4>, &Blitter::Draw<0, 1, 0, 0, 1, 5, 4>,
      &Blitter::Draw<0, 1, 0, 0, 1, 6, 4>, &Blitter::Draw<0, 1, 0, 0, 1, 7, 4>,
      &Blitter::Draw<0, 1, 0, 0, 1, 0, 5>, &Blitter::Draw<0, 1, 0, 0, 1, 1, 5>,
      &Blitter::Draw<0, 1, 0, 0, 1, 2, 5>, &Blitter::Draw<0, 1, 0, 0, 1, 3, 5>,
      &Blitter::Draw<0, 1, 0, 0, 1, 4, 5>, &Blitter::Draw<0, 1, 0, 0, 1, 5, 5>,
      &Blitter::Draw<0, 1, 0, 0, 1, 6, 5>, &Blitter::Draw<0, 1, 0, 0, 1, 7, 5>,
      &Blitter::Draw<0, 1, 0, 0, 1, 0, 6>, &Blitter::Draw<0, 1, 0, 0, 1, 1, 6>,
      &Blitter::Draw<0, 1, 0, 0, 1, 2, 6>, &Blitter::Draw<0, 1, 0, 0, 1, 3, 6>,
      &Blitter::Draw<0, 1, 0, 0, 1, 4, 6>, &Blitter::Draw<0, 1, 0, 0, 1, 5, 6>,
      &Blitter::Draw<0, 1, 0, 0, 1, 6, 6>, &Blitter::Draw<0, 1, 0, 0, 1, 7, 6>,
      &Blitter::Draw<0, 1, 0, 0, 1, 0, 7>, &Blitter::Draw<0, 1, 0, 0, 1, 1, 7>,
      &Blitter::Draw<0, 1, 0, 0, 1, 2, 7>, &Blitter::Draw<0, 1, 0, 0, 1, 3, 7>,
      &Blitter::Draw<0, 1, 0, 0, 1, 4, 7>, &Blitter::Draw<0, 1, 0, 0, 1, 5, 7>,
      &Blitter::Draw<0, 1, 0, 0, 1, 6, 7>, &Blitter::Draw<0, 1, 0, 0, 1, 7, 7>,
  };

  static constexpr DrawMode DrawNonFlipNonTinedNonTransparentBlend[64] = {
      &Blitter::Draw<0, 1, 0, 0, 0, 4, 0>, &Blitter::Draw<0, 1, 0, 0, 0, 5, 0>,
      &Blitter::Draw<0, 1, 0, 0, 0, 6, 0>, &Blitter::Draw<0, 1, 0, 0, 0, 7, 0>,
      &Blitter::Draw<0, 1, 0, 0, 0, 0, 1>, &Blitter::Draw<0, 1, 0, 0, 0, 1, 1>,
      &Blitter::Draw<0, 1, 0, 0, 0, 2, 1>, &Blitter::Draw<0, 1, 0, 0, 0, 3, 1>,
      &Blitter::Draw<0, 1, 0, 0, 0, 4, 1>, &Blitter::Draw<0, 1, 0, 0, 0, 5, 1>,
      &Blitter::Draw<0, 1, 0, 0, 0, 6, 1>, &Blitter::Draw<0, 1, 0, 0, 0, 7, 1>,
      &Blitter::Draw<0, 1, 0, 0, 0, 0, 2>, &Blitter::Draw<0, 1, 0, 0, 0, 1, 2>,
      &Blitter::Draw<0, 1, 0, 0, 0, 2, 2>, &Blitter::Draw<0, 1, 0, 0, 0, 3, 2>,
      &Blitter::Draw<0, 1, 0, 0, 0, 4, 2>, &Blitter::Draw<0, 1, 0, 0, 0, 5, 2>,
      &Blitter::Draw<0, 1, 0, 0, 0, 6, 2>, &Blitter::Draw<0, 1, 0, 0, 0, 7, 2>,
      &Blitter::Draw<0, 1, 0, 0, 0, 0, 3>, &Blitter::Draw<0, 1, 0, 0, 0, 1, 3>,
      &Blitter::Draw<0, 1, 0, 0, 0, 2, 3>, &Blitter::Draw<0, 1, 0, 0, 0, 3, 3>,
      &Blitter::Draw<0, 1, 0, 0, 0, 4, 3>, &Blitter::Draw<0, 1, 0, 0, 0, 5, 3>,
      &Blitter::Draw<0, 1, 0, 0, 0, 6, 3>, &Blitter::Draw<0, 1, 0, 0, 0, 7, 3>,
      &Blitter::Draw<0, 1, 0, 0, 0, 0, 4>, &Blitter::Draw<0, 1, 0, 0, 0, 1, 4>,
      &Blitter::Draw<0, 1, 0, 0, 0, 2, 4>, &Blitter::Draw<0, 1, 0, 0, 0, 3, 4>,
      &Blitter::Draw<0, 1, 0, 0, 0, 4, 4>, &Blitter::Draw<0, 1, 0, 0, 0, 5, 4>,
      &Blitter::Draw<0, 1, 0, 0, 0, 6, 4>, &Blitter::Draw<0, 1, 0, 0, 0, 7, 4>,
      &Blitter::Draw<0, 1, 0, 0, 0, 0, 5>, &Blitter::Draw<0, 1, 0, 0, 0, 1, 5>,
      &Blitter::Draw<0, 1, 0, 0, 0, 2, 5>, &Blitter::Draw<0, 1, 0, 0, 0, 3, 5>,
      &Blitter::Draw<0, 1, 0, 0, 0, 4, 5>, &Blitter::Draw<0, 1, 0, 0, 0, 5, 5>,
      &Blitter::Draw<0, 1, 0, 0, 0, 6, 5>, &Blitter::Draw<0, 1, 0, 0, 0, 7, 5>,
      &Blitter::Draw<0, 1, 0, 0, 0, 0, 6>, &Blitter::Draw<0, 1, 0, 0, 0, 1, 6>,
      &Blitter::Draw<0, 1, 0, 0, 0, 2, 6>, &Blitter::Draw<0, 1, 0, 0, 0, 3, 6>,
      &Blitter::Draw<0, 1, 0, 0, 0, 4, 6>, &Blitter::Draw<0, 1, 0, 0, 0, 5, 6>,
      &Blitter::Draw<0, 1, 0, 0, 0, 6, 6>, &Blitter::Draw<0, 1, 0, 0, 0, 7, 6>,
      &Blitter::Draw<0, 1, 0, 0, 0, 0, 7>, &Blitter::Draw<0, 1, 0, 0, 0, 1, 7>,
      &Blitter::Draw<0, 1, 0, 0, 0, 2, 7>, &Blitter::Draw<0, 1, 0, 0, 0, 3, 7>,
      &Blitter::Draw<0, 1, 0, 0, 0, 4, 7>, &Blitter::Draw<0, 1, 0, 0, 0, 5, 7>,
      &Blitter::Draw<0, 1, 0, 0, 0, 6, 7>, &Blitter::Draw<0, 1, 0, 0, 0, 7, 7>,
  };

  static constexpr DrawMode DrawFlipNonTinedTransparentBlend[64] = {
      &Blitter::Draw<0, 1, 1, 0, 1, 0, 0>, &Blitter::Draw<0, 1, 1, 0, 1, 1, 0>,
      &Blitter::Draw<0, 1, 1, 0, 1, 2, 0>, &Blitter::Draw<0, 1, 1, 0, 1, 3, 0>,
      &Blitter::Draw<0, 1, 1, 0, 1, 4, 0>, &Blitter::Draw<0, 1, 1, 0, 1, 5, 0>,
      &Blitter::Draw<0, 1, 1, 0, 1, 6, 0>, &Blitter::Draw<0, 1, 1, 0, 1, 7, 0>,
      &Blitter::Draw<0, 1, 1, 0, 1, 0, 1>, &Blitter::Draw<0, 1, 1, 0, 1, 1, 1>,
      &Blitter::Draw<0, 1, 1, 0, 1, 2, 1>, &Blitter::Draw<0, 1, 1, 0, 1, 3, 1>,
      &Blitter::Draw<0, 1, 1, 0, 1, 4, 1>, &Blitter::Draw<0, 1, 1, 0, 1, 5, 1>,
      &Blitter::Draw<0, 1, 1, 0, 1, 6, 1>, &Blitter::Draw<0, 1, 1, 0, 1, 7, 1>,
      &Blitter::Draw<0, 1, 1, 0, 1, 0, 2>, &Blitter::Draw<0, 1, 1, 0, 1, 1, 2>,
      &Blitter::Draw<0, 1, 1, 0, 1, 2, 2>, &Blitter::Draw<0, 1, 1, 0, 1, 3, 2>,
      &Blitter::Draw<0, 1, 1, 0, 1, 4, 2>, &Blitter::Draw<0, 1, 1, 0, 1, 5, 2>,
      &Blitter::Draw<0, 1, 1, 0, 1, 6, 2>, &Blitter::Draw<0, 1, 1, 0, 1, 7, 2>,
      &Blitter::Draw<0, 1, 1, 0, 1, 0, 3>, &Blitter::Draw<0, 1, 1, 0, 1, 1, 3>,
      &Blitter::Draw<0, 1, 1, 0, 1, 2, 3>, &Blitter::Draw<0, 1, 1, 0, 1, 3, 3>,
      &Blitter::Draw<0, 1, 1, 0, 1, 4, 3>, &Blitter::Draw<0, 1, 1, 0, 1, 5, 3>,
      &Blitter::Draw<0, 1, 1, 0, 1, 6, 3>, &Blitter::Draw<0, 1, 1, 0, 1, 7, 3>,
      &Blitter::Draw<0, 1, 1, 0, 1, 0, 4>, &Blitter::Draw<0, 1, 1, 0, 1, 1, 4>,
      &Blitter::Draw<0, 1, 1, 0, 1, 2, 4>, &Blitter::Draw<0, 1, 1, 0, 1, 3, 4>,
      &Blitter::Draw<0, 1, 1, 0, 1, 4, 4>, &Blitter::Draw<0, 1, 1, 0, 1, 5, 4>,
      &Blitter::Draw<0, 1, 1, 0, 1, 6, 4>, &Blitter::Draw<0, 1, 1, 0, 1, 7, 4>,
      &Blitter::Draw<0, 1, 1, 0, 1, 0, 5>, &Blitter::Draw<0, 1, 1, 0, 1, 1, 5>,
      &Blitter::Draw<0, 1, 1, 0, 1, 2, 5>, &Blitter::Draw<0, 1, 1, 0, 1, 3, 5>,
      &Blitter::Draw<0, 1, 1, 0, 1, 4, 5>, &Blitter::Draw<0, 1, 1, 0, 1, 5, 5>,
      &Blitter::Draw<0, 1, 1, 0, 1, 6, 5>, &Blitter::Draw<0, 1, 1, 0, 1, 7, 5>,
      &Blitter::Draw<0, 1, 1, 0, 1, 0, 6>, &Blitter::Draw<0, 1, 1, 0, 1, 1, 6>,
      &Blitter::Draw<0, 1, 1, 0, 1, 2, 6>, &Blitter::Draw<0, 1, 1, 0, 1, 3, 6>,
      &Blitter::Draw<0, 1, 1, 0, 1, 4, 6>, &Blitter::Draw<0, 1, 1, 0, 1, 5, 6>,
      &Blitter::Draw<0, 1, 1, 0, 1, 6, 6>, &Blitter::Draw<0, 1, 1, 0, 1, 7, 6>,
      &Blitter::Draw<0, 1, 1, 0, 1, 0, 7>, &Blitter::Draw<0, 1, 1, 0, 1, 1, 7>,
      &Blitter::Draw<0, 1, 1, 0, 1, 2, 7>, &Blitter::Draw<0, 1, 1, 0, 1, 3, 7>,
      &Blitter::Draw<0, 1, 1, 0, 1, 4, 7>, &Blitter::Draw<0, 1, 1, 0, 1, 5, 7>,
      &Blitter::Draw<0, 1, 1, 0, 1, 6, 7>, &Blitter::Draw<0, 1, 1, 0, 1, 7, 7>,
  };

  static constexpr DrawMode DrawFlipNonTinedNonTransparentBlend[64] = {
      &Blitter::Draw<0, 1, 1, 0, 0, 0, 0>, &Blitter::Draw<0, 1, 1, 0, 0, 1, 0>,
      &Blitter::Draw<0, 1, 1, 0, 0, 2, 0>, &Blitter::Draw<0, 1, 1, 0, 0, 3, 0>,
      &Blitter::Draw<0, 1, 1, 0, 0, 4, 0>, &Blitter::Draw<0, 1, 1, 0, 0, 5, 0>,
      &Blitter::Draw<0, 1, 1, 0, 0, 6, 0>, &Blitter::Draw<0, 1, 1, 0, 0, 7, 0>,
      &Blitter::Draw<0, 1, 1, 0, 0, 0, 1>, &Blitter::Draw<0, 1, 1, 0, 0, 1, 1>,
      &Blitter::Draw<0, 1, 1, 0, 0, 2, 1>, &Blitter::Draw<0, 1, 1, 0, 0, 3, 1>,
      &Blitter::Draw<0, 1, 1, 0, 0, 4, 1>, &Blitter::Draw<0, 1, 1, 0, 0, 5, 1>,
      &Blitter::Draw<0, 1, 1, 0, 0, 6, 1>, &Blitter::Draw<0, 1, 1, 0, 0, 7, 1>,
      &Blitter::Draw<0, 1, 1, 0, 0, 0, 2>, &Blitter::Draw<0, 1, 1, 0, 0, 1, 2>,
      &Blitter::Draw<0, 1, 1, 0, 0, 2, 2>, &Blitter::Draw<0, 1, 1, 0, 0, 3, 2>,
      &Blitter::Draw<0, 1, 1, 0, 0, 4, 2>, &Blitter::Draw<0, 1, 1, 0, 0, 5, 2>,
      &Blitter::Draw<0, 1, 1, 0, 0, 6, 2>, &Blitter::Draw<0, 1, 1, 0, 0, 7, 2>,
      &Blitter::Draw<0, 1, 1, 0, 0, 0, 3>, &Blitter::Draw<0, 1, 1, 0, 0, 1, 3>,
      &Blitter::Draw<0, 1, 1, 0, 0, 2, 3>, &Blitter::Draw<0, 1, 1, 0, 0, 3, 3>,
      &Blitter::Draw<0, 1, 1, 0, 0, 4, 3>, &Blitter::Draw<0, 1, 1, 0, 0, 5, 3>,
      &Blitter::Draw<0, 1, 1, 0, 0, 6, 3>, &Blitter::Draw<0, 1, 1, 0, 0, 7, 3>,
      &Blitter::Draw<0, 1, 1, 0, 0, 0, 4>, &Blitter::Draw<0, 1, 1, 0, 0, 1, 4>,
      &Blitter::Draw<0, 1, 1, 0, 0, 2, 4>, &Blitter::Draw<0, 1, 1, 0, 0, 3, 4>,
      &Blitter::Draw<0, 1, 1, 0, 0, 4, 4>, &Blitter::Draw<0, 1, 1, 0, 0, 5, 4>,
      &Blitter::Draw<0, 1, 1, 0, 0, 6, 4>, &Blitter::Draw<0, 1, 1, 0, 0, 7, 4>,
      &Blitter::Draw<0, 1, 1, 0, 0, 0, 5>, &Blitter::Draw<0, 1, 1, 0, 0, 1, 5>,
      &Blitter::Draw<0, 1, 1, 0, 0, 2, 5>, &Blitter::Draw<0, 1, 1, 0, 0, 3, 5>,
      &Blitter::Draw<0, 1, 1, 0, 0, 4, 5>, &Blitter::Draw<0, 1, 1, 0, 0, 5, 5>,
      &Blitter::Draw<0, 1, 1, 0, 0, 6, 5>, &Blitter::Draw<0, 1, 1, 0, 0, 7, 5>,
      &Blitter::Draw<0, 1, 1, 0, 0, 0, 6>, &Blitter::Draw<0, 1, 1, 0, 0, 1, 6>,
      &Blitter::Draw<0, 1, 1, 0, 0, 2, 6>, &Blitter::Draw<0, 1, 1, 0, 0, 3, 6>,
      &Blitter::Draw<0, 1, 1, 0, 0, 4, 6>, &Blitter::Draw<0, 1, 1, 0, 0, 5, 6>,
      &Blitter::Draw<0, 1, 1, 0, 0, 6, 6>, &Blitter::Draw<0, 1, 1, 0, 0, 7, 6>,
      &Blitter::Draw<0, 1, 1, 0, 0, 0, 7>, &Blitter::Draw<0, 1, 1, 0, 0, 1, 7>,
      &Blitter::Draw<0, 1, 1, 0, 0, 2, 7>, &Blitter::Draw<0, 1, 1, 0, 0, 3, 7>,
      &Blitter::Draw<0, 1, 1, 0, 0, 4, 7>, &Blitter::Draw<0, 1, 1, 0, 0, 5, 7>,
      &Blitter::Draw<0, 1, 1, 0, 0, 6, 7>, &Blitter::Draw<0, 1, 1, 0, 0, 7, 7>,
  };
};
