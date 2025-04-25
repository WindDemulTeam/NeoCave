#include "ui_input.h"

void UiInput::Show() {
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::Begin("Inputs settings", nullptr, flags);

  auto actions = input_manager_.GetInputActions();
  for (const auto& action : actions) {
    DrawAction(action);
  }

  ImGui::End();
}

void UiInput::DrawAction(const std::string& action) {
  ImGui::PushID(action.c_str());

  std::string binding_name;

  bool rebinding_ = is_rebinding_ && current_rebinding_action_ == action;

  if (rebinding_) {
    binding_name = "waiting...";
  } else {
    binding_name = input_manager_.GetNameBindingsForAction(action);
  }

  float width = ImGui::GetContentRegionAvail().x;
  float status_width = ImGui::CalcTextSize(binding_name.c_str()).x + 20;
  float label_width = width - status_width;

  if (ImGui::Selectable("##strip", false, 0, ImVec2(width, 0))) {
    if (!rebinding_) {
      StartRebinding(action);
    } else {
      is_rebinding_ = false;
    }
  }

  ImGui::SameLine(0, 0);
  ImGui::SetCursorPosX(0);
  ImGui::Text("  %s", action.c_str());

  ImGui::SameLine(0, 0);
  ImGui::SetCursorPosX(label_width);
  ImGui::Text("%s", binding_name.c_str());
  ImGui::PopID();
}

void UiInput::StartRebinding(const std::string& action) {
  current_rebinding_action_ = action;
  is_rebinding_ = true;
}

void UiInput::HandleEvent(const SDL_Event& event) {
  if (!is_rebinding_) return;

  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
      input_manager_.BindKeyboard(current_rebinding_action_, event.key.key);
      is_rebinding_ = false;
      break;

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      is_rebinding_ = false;
      break;
  }
}