#pragma once

#include <cstdint>
#include <cstring>

struct Rtc9701Interface {
  int address_bits;
  int data_bits;
  int rtc_address_bits;
  int rtc_data_bits;
  const char *cmd_read;
  const char *cmd_write;
  const char *cmd_readrtc;
  const char *cmd_writertc;
  const char *cmd_unlock;
  int reset_delay;
};

class Rtc9701 {
 public:
  void Init();
  uint8_t Read8(uint32_t addr);
  void Write8(uint32_t addr, uint8_t value);

 private:
  enum {
    kClearLine = 0,
    kAssertLine = 1,
    kPulseLine = 3

  };

  Rtc9701Interface *rtc9701_intf;

  std::size_t rtc9701_serial_count = 0;
  uint8_t rtc9701_serial_buffer[40];

  int rtc9701_data_bits;
  int rtc9701_send_bits;
  int rtc9701_clock_count;

  uint8_t eeprom_[512];
  uint8_t rtc_data[16];

  int rtc9701_latch = 0;
  int rtc9701_locked = 1;
  int rtc9701_sending = 0;
  int rtc9701_reset_line = kAssertLine;
  int rtc9701_clock_line = kAssertLine;
  int rtc9701_reset_delay;

  int ReadBit();
  void WriteBit(int bit);
  void SetCsLine(int state);
  void SetClockLine(int state);

  void Reset();
  void Write(int bit);
  int CommandMatch(const char *buf, const char *cmd, int len);
};