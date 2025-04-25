#pragma once
#include <imgui.h>

#include <string>

#include "input_manager.h"

class UiInput {
 public:
  void Show();
  void HandleEvent(const SDL_Event& event);
  InputManager& GetInputManager() { return input_manager_; }

 private:
  InputManager input_manager_;
  std::string current_rebinding_action_;
  bool is_rebinding_ = false;

  void DrawAction(const std::string& action);
  void StartRebinding(const std::string& action);
};