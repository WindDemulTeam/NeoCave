#pragma once

#include <filesystem>

#include "imgui.h"

namespace ui {
class FileDialog {
 public:
  enum State : int { kRunning, kOk, kCancel };

  State Open(std::string &out, std::filesystem::path directory = ".");

 private:
  State state_ = kOk;
  std::filesystem::path directory_;
};

}  // namespace ui