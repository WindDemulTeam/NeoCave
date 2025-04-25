#pragma once

#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "roms.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include "SDL3/SDL_opengles2.h"
#else
#include "SDL3/SDL_opengl.h"
#endif

namespace ui {

struct Game {
  uint32_t game_id;
  std::string name;
  std::string path;
  GLuint poster_id = 0;
};

class UiGameList {
 public:
  UiGameList();
  void ScanDirectory(std::string dir, GamesList &game_list);

  std::vector<Game> &GetGameList() { return game_list_; }

 private:
  std::vector<Game> game_list_;
  std::unordered_map<std::string, std::span<unsigned char>> poster_map_;

  GLuint LoadPoster(std::string name);
};

}  // namespace ui