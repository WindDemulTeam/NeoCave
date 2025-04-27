
#include "ui.h"

#include <iostream>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

namespace ui {
bool Ui::Init(SDL_Window* window, void* gl_context) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL3_InitForOpenGL(window, gl_context);

#if defined(__APPLE__)
  const char* version = "#version 150\n";
#else
  const char* version = "#version 130\n";
#endif

  ImGui_ImplOpenGL3_Init(version);

  return true;
}

void Ui::Close() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void Ui::HandleEvent(SDL_Event* event) {
  ui_input.HandleEvent(*event);
  ImGui_ImplSDL3_ProcessEvent(event);
}

void Ui::Begin() {
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}
void Ui::End() {
  ImGuiIO& io = ImGui::GetIO();
  ImGui::Render();
  glViewport(0, 0, static_cast<GLsizei>(io.DisplaySize.x),
             static_cast<GLsizei>(io.DisplaySize.y));

  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Game* Ui::ShowGameList() {
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::Begin("User Interface", nullptr, flags);

  float total_width = ImGui::GetWindowWidth();

  Game* result = RenderGameList();
  ImGui::End();

  return result;
}

Game* Ui::RenderGameList() {
  Game* game = nullptr;
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false,
                    ImGuiWindowFlags_NoScrollbar);

  auto& style = ImGui::GetStyle();

  ImVec2 region_min = ImGui::GetWindowContentRegionMin();
  ImVec2 region_max = ImGui::GetWindowContentRegionMax();

  float margin = 5.0f;
  float tile_size = 192.0f;
  auto region_width =
      region_max.x - region_min.x - 2.0f * style.WindowPadding.x;

  auto& games = game_list.GetGameList();
  int item_count = static_cast<int>(games.size());

  int tiles_per_row = static_cast<int>(region_width / (tile_size + margin));
  if (tiles_per_row < 1) {
    tiles_per_row = 1;
  }

  float total_tiles_width =
      tiles_per_row * tile_size + (tiles_per_row - 1) * margin;
  float remaining_space = region_width - total_tiles_width;
  float extra_margin = remaining_space / (tiles_per_row + 1);

  for (int i = 0; i < item_count; i++) {
    if (i > 0 && (i % tiles_per_row == 0)) {
      ImGui::NewLine();
    }
    if (i % tiles_per_row != 0) {
      ImGui::SameLine(0, extra_margin);
    } else {
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + extra_margin);
    }

    ImGui::BeginGroup();
    ImGui::PushID(i);
    if (ImGui::ImageButton("", (ImTextureID)(intptr_t)games[i].poster_id,
                           ImVec2(tile_size, tile_size))) {
      game = &games[i];
    }
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + tile_size);
    ImGui::Text(games[i].name.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopID();
    ImGui::EndGroup();
  }
  ImGui::EndChild();

  return game;
}

void Ui::RenderFpsCounter(float fps, float rps) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();

  const char* fps_text = "FPS: 999.9 RPS: 999.9";
  ImVec2 text_size = ImGui::CalcTextSize(fps_text);
  ImVec2 window_size(text_size.x + 20.0f, text_size.y + 10.0f);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::SetNextWindowBgAlpha(0.3f);
  ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
  ImGui::SetNextWindowSize(window_size);

  ImGui::Begin("FPS Counter", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_NoInputs);

  ImGui::Text("FPS: %.1f RPS: %.1f", fps, rps);
  ImGui::End();
  ImGui::PopStyleVar();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace ui
