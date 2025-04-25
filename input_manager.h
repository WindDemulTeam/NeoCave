#pragma once
#include <SDL3/SDL.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "iconfig.h"

class InputManager : public config::IConfig {
 public:
  enum class DeviceType {
    kNotBinding,
    kKeyboard,
    kGamepadButton,
    kGamepadAxis
  };

  struct Binding {
    DeviceType device;
    union {
      SDL_Scancode key;
      struct {
        int index;
        float threshold;
      };
    };
    SDL_JoystickID gamepad_id;
  };

  InputManager();
  ~InputManager();

  void InitializeBindings(const std::vector<std::string>& actions);

  void Update();
  bool IsActionActive(const std::string& action);

  void BindKeyboard(const std::string& action, SDL_Keycode key);
  void BindGamepadButton(const std::string& action, int button,
                         SDL_JoystickID id);
  void BindGamepadAxis(const std::string& action, int axis, float threshold,
                       SDL_JoystickID id);

  const std::string GetNameBindingsForAction(const std::string& action) const;

  const std::unordered_map<SDL_JoystickID, SDL_Gamepad*>& GetGamepads() const {
    return gamepads_;
  }

  const std::vector<std::string> GetInputActions() const { return actions_; }

  void LoadConfig(const toml::table& data) override;
  void SaveConfig(toml::table& data) override;

 private:
  std::vector<std::string> actions_;
  std::unordered_map<std::string, Binding> action_bindings_;
  const bool* keyboard_state_ = nullptr;
  std::unordered_map<SDL_JoystickID, SDL_Gamepad*> gamepads_;

  void UpdateGamepads();

  int GetGamepadDisplayId(SDL_JoystickID id) const {
    int index = 1;
    for (const auto& [jid, _] : gamepads_) {
      if (jid == id) return index;
      index++;
    }
    return 0;
  }

  std::string ScancodeToString(SDL_Scancode code) {
    return SDL_GetScancodeName(code);
  }

  SDL_Scancode StringToScancode(const std::string& name) {
    return SDL_GetScancodeFromName(name.c_str());
  }

  toml::table BindingToTable(const Binding& binding) {
    toml::table tbl;

    switch (binding.device) {
      case DeviceType::kKeyboard:
        tbl.insert_or_assign("device", "keyboard");
        tbl.insert_or_assign("key", ScancodeToString(binding.key));
        break;
        // case DeviceType::GAMEPAD_BUTTON:
        //   tbl.insert("device", "gamepad");
        //   tbl.insert("button", binding.button);
        //   break;
        // case DeviceType::GAMEPAD_AXIS:
        //   tbl.insert("device", "gamepad_axis");
        //   tbl.insert("axis", binding.axis.index);
        //   tbl.insert("threshold", binding.axis.threshold);
        //   break;
    }
    return tbl;
  }

  Binding TableToBinding(const toml::table& tbl) {
    Binding binding{};

    auto device = tbl.find("device");
    if (device != tbl.end()) {
      auto name = device->second.as_string();
      if (name == "keyboard") {
        binding.device = DeviceType::kKeyboard;
        if (tbl.contains("key")) {
          binding.key = StringToScancode(tbl.at("key").as_string());
        }
      } /*else if (device == "gamepad") {
        binding.device = DeviceType::GAMEPAD_BUTTON;
        binding.button = tbl["button"].value_or(0);
      } else if (device == "gamepad_axis") {
        binding.device = DeviceType::GAMEPAD_AXIS;
        binding.axis.index = tbl["axis"].value_or(0);
        binding.axis.threshold = tbl["threshold"].value_or(0.5f);
      }*/
    }
    return binding;
  }
};