#include "ui_file_dialog.h"

#include <filesystem>

#ifdef _WIN32
#include <Windows.h>

#include <vector>
#endif

namespace ui {
FileDialog::State FileDialog::Open(std::string& out,
                                   std::filesystem::path directory) {
  ImGuiIO& io = ImGui::GetIO();
  ImGuiStyle& style = ImGui::GetStyle();

  if (state_ != kRunning) {
    directory_ = std::filesystem::absolute(directory);
    state_ = kRunning;
  }

  auto display_size = ImGui::GetIO().DisplaySize;

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(display_size);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x80000000);
  ImGui::Begin("File Dialog", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

  ImVec2 dialog_size = ImVec2(display_size.x * 0.7f, display_size.y * 0.7f);
  ImVec2 dialog_position = ImVec2((display_size.x - dialog_size.x) * 0.5f,
                                  (display_size.y - dialog_size.y) * 0.5f);

  ImGui::SetCursorPos(dialog_position);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff201e19);
  ImGui::BeginChild(
      "Dialog", dialog_size, false,
      ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NavFlattened);

  auto content_min = ImGui::GetWindowContentRegionMin();
  auto content_max = ImGui::GetWindowContentRegionMax();
  auto width = (content_max.x - content_min.x) - style.ItemSpacing.x;
  auto height = (content_max.y - content_min.y) - style.ItemSpacing.y - 16.0f;

  ImVec2 entries_size = {width * 0.75f, height};
  ImVec2 action_size = {width * 0.25f, height};

  ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff201e19);
  ImGui::BeginChild(
      "Entries", entries_size, false,
      ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NavFlattened);

  ImGui::PushStyleColor(ImGuiCol_Text, 0xffd0d0d0);
  ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xffa9583e);
  ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0.5f));

  std::string folder = "..";
  if (ImGui::Button(folder.c_str(), ImVec2(-1, 0))) {
    directory_ = directory_.parent_path();
  }

  for (const auto& entry : std::filesystem::directory_iterator(directory_)) {
    std::string folder = entry.path().filename().string();
    if (ImGui::Button(folder.c_str(), ImVec2(-1, 0))) {
      if (entry.is_directory()) {
        directory_ = entry.path();
      }
    }
  }

  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(3);

  ImGui::EndChild();
  ImGui::PopStyleColor(1);

  ImGui::SameLine(0.0f);

  ImGui::BeginChild("actions", action_size, false,
                    ImGuiWindowFlags_NavFlattened);

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.5, 1.5));
  ImGui::PushStyleColor(ImGuiCol_Text, 0xffd0d0d0);
  ImGui::PushStyleColor(ImGuiCol_Button, 0xff181611);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xffa9583e);

#ifdef _WIN32
  DWORD drives = GetLogicalDrives();
  std::vector<std::string> items;
  auto root_name = directory_.root_name().string();

  for (int i = 0; i < 26; i++) {
    if (drives & (1 << i)) {
      std::string drive(1, char(i + 65));
      drive.append(":");
      items.push_back(drive);
    }
  }
  int selected_item = 0;
  for (int i = 0; i < items.size(); i++) {
    if (items[i] == root_name) {
      selected_item = i;
      break;
    }
  }

  ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xff181611);
  ImGui::Text("Drive:");
  ImGui::SameLine();
  if (ImGui::BeginCombo("##Drive", items[selected_item].c_str())) {
    for (int i = 0; i < items.size(); i++) {
      bool is_selected = selected_item == i;
      if (ImGui::Selectable(items[i].c_str(), is_selected)) {
        if (selected_item != i) {
          directory_ = std::filesystem::absolute(items[i]);
        }
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopStyleColor(1);
#endif

  if (ImGui::Button("Open", ImVec2(-1.0f, 0.0f))) {
    state_ = kOk;
    out = directory_.string();
  }
  if (ImGui::Button("Cancel", ImVec2(-1.0f, 0.0f))) {
    state_ = kCancel;
  }
  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(3);
  ImGui::EndChild();

  ImGui::Text(directory_.string().c_str());

  ImGui::EndChild();
  ImGui::PopStyleColor(1);
  ImGui::End();
  ImGui::PopStyleColor(1);

  return state_;
}
}  // namespace ui