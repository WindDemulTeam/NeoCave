#pragma once

#include <cstdint>

#include "toml.hpp"

namespace config {
struct IConfig {
  virtual void LoadConfig(const toml::table& data) = 0;
  virtual void SaveConfig(toml::table& data) = 0;
  virtual ~IConfig() = default;
};
}  // namespace config