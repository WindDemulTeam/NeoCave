#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "iconfig.h"

namespace config {
struct State {
  std::string path;
};

class Config {
 public:
  bool Load();
  void Save();

  State state_;

  template <typename T>
  void AddConfig(const std::string& name, T& config) {
    configs_[name] = &config;
  }

 private:
  constexpr static const char* kConfigName = "config.toml";

  std::unordered_map<std::string, IConfig*> configs_;
};
}  // namespace config