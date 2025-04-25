#include "config.h"

#include <fstream>
#include <iostream>

#include "toml.hpp"

namespace config {

bool Config::Load() {
  try {
    const auto tbl = toml::parse(kConfigName);

    const auto& game = toml::find(tbl, "game");
    state_.path = toml::find<std::string>(game, "path");

    for (const auto& [name, config] : configs_) {
      auto node = toml::find(tbl, name);
      if (node.is_table()) {
        config->LoadConfig(node.as_table());
      }
    }
  } catch (const std::exception&) {
    return false;
  }

  return true;
}

void Config::Save() {
  toml::value tbl(toml::table{
      {"game", toml::table{{"path", state_.path}}},
  });

  for (auto& [name, config] : configs_) {
    toml::table table;
    config->SaveConfig(table);
    tbl[name] = table;
  }

  std::ofstream ofs(kConfigName);
  ofs << toml::format(tbl);
  ofs.close();
}

}  // namespace config