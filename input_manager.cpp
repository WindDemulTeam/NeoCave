#include "input_manager.h"

#include <fstream>

InputManager::InputManager() {
  SDL_InitSubSystem(SDL_INIT_GAMEPAD);
  Update();
}

InputManager::~InputManager() {
  for (auto& [id, gamepad] : gamepads_) {
    SDL_CloseGamepad(gamepad);
  }
  SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

void InputManager::Update() {
  keyboard_state_ = SDL_GetKeyboardState(nullptr);
  UpdateGamepads();
}

void InputManager::InitializeBindings(const std::vector<std::string>& actions) {
  Binding binding = {};
  binding.device = DeviceType::kNotBinding;
  for (const auto& action : actions) {
    action_bindings_[action] = binding;
  }
  actions_ = actions;
}

bool InputManager::IsActionActive(const std::string& action) {
  auto binding = action_bindings_[action];
  Update();
  switch (binding.device) {
    case DeviceType::kKeyboard:
      if (keyboard_state_[binding.key]) {
        return true;
      }
      break;

    case DeviceType::kGamepadButton:
      break;

    case DeviceType::kGamepadAxis:
      break;
  }
  return false;
}

void InputManager::UpdateGamepads() {}

void InputManager::BindKeyboard(const std::string& action, SDL_Keycode key) {
  Binding binding;

  binding.device = DeviceType::kKeyboard;
  binding.key = SDL_GetScancodeFromKey(key, nullptr);
  binding.gamepad_id = 0;
  action_bindings_[action] = binding;
}

void InputManager::BindGamepadButton(const std::string& action, int button,
                                     SDL_JoystickID id) {
  // Binding binding;

  // binding.device = DeviceType::GamepadButton;
  // binding.key = 0;
  // binding.index = button;
  // binding.gamepad_id = id;

  // action_bindings_.emplace(action, binding);
}

void InputManager::BindGamepadAxis(const std::string& action, int axis,
                                   float threshold, SDL_JoystickID id) {
  // Binding binding;

  // binding.device = DeviceType::GamepadButton;
  // binding.key = 0;
  // binding.index = axis;
  // binding.gamepad_id = id;

  // action_bindings_.emplace(action, binding);
}

const std::string InputManager::GetNameBindingsForAction(
    const std::string& action) const {
  std::string name_binding = "Not binding";
  auto binding = action_bindings_.find(action);
  if (binding != action_bindings_.end()) {
    switch (binding->second.device) {
      case DeviceType::kKeyboard:
        name_binding = SDL_GetScancodeName(binding->second.key);
        break;
      case DeviceType::kGamepadButton:
        break;
      case DeviceType::kGamepadAxis:
        break;
    }
  }
  return name_binding;
}

void InputManager::LoadConfig(const toml::table& data) {
  for (const auto& [action_name, value] : data) {
    Binding binding = TableToBinding(value.as_table());
    action_bindings_[action_name] = binding;
  }
}

void InputManager::SaveConfig(toml::table& data) {
  for (const auto& [name, binding] : action_bindings_) {
    data[name] = BindingToTable(binding);
  }
}