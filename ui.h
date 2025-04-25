#pragma once

#include "SDL3/SDL.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include "SDL3/SDL_opengles2.h"
#else
#include "SDL3/SDL_opengl.h"
#endif

#include <cstdint>

#include "ui_file_dialog.h"
#include "ui_game_list.h"
#include "ui_input.h"

namespace ui {

class Ui {
 public:
  bool Init(SDL_Window *window, void *gl_context);
  void Close();
  void Begin();
  void End();
  void HandleEvent(SDL_Event *event);
  Game *ShowGameList();

  FileDialog file_dialog;
  UiGameList game_list;
  UiInput ui_input;

 private:
  Game *RenderGameList();
  void RenderFpsCounter(float fps, float rps);
};
}  // namespace ui