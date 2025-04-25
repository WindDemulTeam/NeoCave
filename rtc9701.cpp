
#include "rtc9701.h"

#include <array>
#include <cstring>
#include <ctime>

#define TOBCD(i) ((i) / 10 * 16 + (i) % 10)

Rtc9701Interface rtc9701_interface = {
    3 + 8 + 1,           // address bits
    16,                  // data bits
    4,                   // RTC address bits
    8,                   // RTC data bits
    "1010",              // read		1010 xxx aaaaaaaa 0
    "0010",              // write	0010 xxx aaaaaaaa 0 dddddddddddddddd
    "1000",              // read rtc		1000 aaaa
    "0000",              // write rtc	0000 aaaa dddddddd
    "0110x10011xxxxxx",  // ewen	0110x10011xxxxxx
    0};

void Rtc9701::Init() {
  std::memset(eeprom_, 0, sizeof(eeprom_));
  rtc9701_intf = &rtc9701_interface;
}

int Rtc9701::ReadBit() {
  int res;

  if (rtc9701_sending) {
    res = (rtc9701_data_bits >> rtc9701_send_bits) & 1;
  } else {
    if (rtc9701_reset_delay > 0) {
      rtc9701_reset_delay--;
      res = 0;
    } else
      res = 1;
  }

  return res;
}

void Rtc9701::WriteBit(int bit) { rtc9701_latch = bit; }

void Rtc9701::Reset() {
  rtc9701_serial_count = 0;
  rtc9701_sending = 0;
  rtc9701_reset_delay = rtc9701_intf->reset_delay;
}

void Rtc9701::SetCsLine(int state) {
  rtc9701_reset_line = state;

  if (rtc9701_reset_line != kClearLine) {
    Reset();
  }
}

int Rtc9701::CommandMatch(const char *buf, const char *cmd, int len) {
  if (cmd == 0) return 0;
  if (len == 0) return 0;

  for (; len > 0;) {
    char b = *buf;
    char c = *cmd;

    if ((b == 0) || (c == 0)) return (b == c);

    switch (c) {
      case '0':
      case '1':
        if (b != c) return 0;
      case 'X':
      case 'x':
        buf++;
        len--;
        cmd++;
        break;

      case '*':
        c = cmd[1];
        switch (c) {
          case '0':
          case '1':
            if (b == c) {
              cmd++;
            } else {
              buf++;
              len--;
            }
            break;
          default:
            return 0;
        }
    }
  }
  return (*cmd == 0);
}

void Rtc9701::Write(int bit) {
  if (rtc9701_serial_count >= std::size(rtc9701_serial_buffer) - 1) {
    return;
  }

  rtc9701_serial_buffer[rtc9701_serial_count++] = (bit ? '1' : '0');
  rtc9701_serial_buffer[rtc9701_serial_count] = 0;

  if ((rtc9701_serial_count > rtc9701_intf->address_bits) &&
      CommandMatch((char *)rtc9701_serial_buffer, rtc9701_intf->cmd_read,
                   (int)strlen((char *)rtc9701_serial_buffer) -
                       rtc9701_intf->address_bits)) {
    int i, address;

    address = 0;
    for (i = rtc9701_serial_count - rtc9701_intf->address_bits;
         i < rtc9701_serial_count; i++) {
      address <<= 1;
      if (rtc9701_serial_buffer[i] == '1') address |= 1;
    }
    address &= 0x01fe;
    rtc9701_data_bits = ((eeprom_[address + 0] << 8) + eeprom_[address + 1])
                        << 1;
    rtc9701_clock_count = 0;
    rtc9701_sending = 1;
    rtc9701_send_bits = rtc9701_intf->data_bits;
    rtc9701_serial_count = 0;
  } else if ((rtc9701_serial_count > rtc9701_intf->rtc_address_bits) &&
             CommandMatch((char *)rtc9701_serial_buffer,
                          rtc9701_intf->cmd_readrtc,
                          (int)strlen((char *)rtc9701_serial_buffer) -
                              rtc9701_intf->rtc_address_bits)) {
    int i, address;

    address = 0;
    for (i = rtc9701_serial_count - rtc9701_intf->rtc_address_bits;
         i < rtc9701_serial_count; i++) {
      address <<= 1;
      if (rtc9701_serial_buffer[i] == '1') address |= 1;
    }
    if (address < 8) {
      time_t rawtime;
      struct tm *timeinfo;
      rawtime = std::time(NULL);
      timeinfo = std::localtime(&rawtime);

      switch (address) {
        case 0:
          rtc9701_data_bits = TOBCD(timeinfo->tm_sec);
          break;
        case 1:
          rtc9701_data_bits = TOBCD(timeinfo->tm_min);
          break;
        case 2:
          rtc9701_data_bits = TOBCD(timeinfo->tm_hour);
          break;
        case 3:
          rtc9701_data_bits = 1 << timeinfo->tm_wday;
          break;
        case 4:
          rtc9701_data_bits = TOBCD(timeinfo->tm_mday);
          break;
        case 5:
          rtc9701_data_bits = TOBCD(timeinfo->tm_mon);
          break;
        case 6:
          rtc9701_data_bits = TOBCD(timeinfo->tm_year % 100);
          break;
        case 7:
          rtc9701_data_bits = 0x200;
          break;
        default:
          rtc9701_data_bits = 0;
          break;
      }
    } else {
      rtc9701_data_bits = rtc_data[address];
    }
    rtc9701_data_bits = rtc9701_data_bits << 1;
    rtc9701_clock_count = 0;
    rtc9701_sending = 1;
    rtc9701_send_bits = rtc9701_intf->rtc_data_bits;
    rtc9701_serial_count = 0;
  } else if ((rtc9701_serial_count >
              (rtc9701_intf->address_bits + rtc9701_intf->data_bits)) &&
             CommandMatch(
                 (char *)rtc9701_serial_buffer, rtc9701_intf->cmd_write,
                 (int)strlen((char *)rtc9701_serial_buffer) -
                     (rtc9701_intf->address_bits + rtc9701_intf->data_bits))) {
    int i, address, data;

    address = 0;
    for (i = rtc9701_serial_count - rtc9701_intf->data_bits -
             rtc9701_intf->address_bits;
         i < (rtc9701_serial_count - rtc9701_intf->data_bits); i++) {
      address <<= 1;
      if (rtc9701_serial_buffer[i] == '1') address |= 1;
    }
    address &= 0x01fe;
    data = 0;
    for (i = rtc9701_serial_count - rtc9701_intf->data_bits;
         i < rtc9701_serial_count; i++) {
      data <<= 1;
      if (rtc9701_serial_buffer[i] == '1') data |= 1;
    }
    if (rtc9701_locked == 0) {
      eeprom_[address + 0] = data >> 8;
      eeprom_[address + 1] = data & 0xff;
    } else {
      rtc9701_serial_count = 0;
    }
  } else if ((rtc9701_serial_count >
              (rtc9701_intf->rtc_address_bits + rtc9701_intf->rtc_data_bits)) &&
             CommandMatch((char *)rtc9701_serial_buffer,
                          rtc9701_intf->cmd_writertc,
                          (int)strlen((char *)rtc9701_serial_buffer) -
                              (rtc9701_intf->rtc_address_bits +
                               rtc9701_intf->rtc_data_bits))) {
    int i, address, data;

    address = 0;
    for (i = rtc9701_serial_count - rtc9701_intf->rtc_data_bits -
             rtc9701_intf->rtc_address_bits;
         i < (rtc9701_serial_count - rtc9701_intf->rtc_data_bits); i++) {
      address <<= 1;
      if (rtc9701_serial_buffer[i] == '1') address |= 1;
    }
    data = 0;
    for (i = rtc9701_serial_count - rtc9701_intf->rtc_data_bits;
         i < rtc9701_serial_count; i++) {
      data <<= 1;
      if (rtc9701_serial_buffer[i] == '1') data |= 1;
    }
    rtc_data[address] = data;
    rtc9701_serial_count = 0;
  } else if (CommandMatch((char *)rtc9701_serial_buffer,
                          rtc9701_intf->cmd_unlock,
                          (int)strlen((char *)rtc9701_serial_buffer))) {
    rtc9701_locked = 0;
    rtc9701_serial_count = 0;
  }
}

void Rtc9701::SetClockLine(int state) {
  if (state == kPulseLine ||
      (rtc9701_clock_line == kClearLine && state != kClearLine)) {
    if (rtc9701_reset_line == kClearLine) {
      if (rtc9701_sending) {
        if (rtc9701_clock_count == rtc9701_send_bits) rtc9701_sending = 0;
        rtc9701_data_bits = (rtc9701_data_bits << 1) | 1;
        rtc9701_clock_count++;
      } else
        Write(rtc9701_latch);
    }
  }

  rtc9701_clock_line = state;
}

uint8_t Rtc9701::Read8(uint32_t addr) {
  switch (addr & 0xffff) {
    case 0x0001:
      return 0x06 | ReadBit();
    default:
      return -1;
  }
}

void Rtc9701::Write8(uint32_t addr, uint8_t value) {
  switch (addr & 0xffff) {
    case 0x0001:
      WriteBit(value & 0x01);
      SetCsLine((value & 0x04) ? kClearLine : kAssertLine);
      SetClockLine((value & 0x02) ? kAssertLine : kClearLine);
      break;
    case 0x0002:
    case 0x0003:
      break;
    default:
      break;
  }
}