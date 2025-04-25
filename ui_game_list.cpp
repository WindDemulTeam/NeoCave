
#include "ui_game_list.h"

#include <filesystem>

#include "assets/deathsml.h"
#include "assets/dfk.h"
#include "assets/espgal.h"
#include "assets/futari.h"
#include "assets/ibara.h"
#include "assets/mmmbanc.h"
#include "assets/mmpork.h"
#include "assets/mushisam.h"
#include "assets/mushitam.h"
#include "assets/pinkswts.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ui {

UiGameList::UiGameList() {
  poster_map_["deathsml"] =
      std::span<unsigned char>(deathsml, sizeof(deathsml));
  poster_map_["dthsmlbl"] =
      std::span<unsigned char>(deathsml, sizeof(deathsml));

  poster_map_["espgal2"] = std::span<unsigned char>(espgal, sizeof(espgal));

  poster_map_["futari10"] = std::span<unsigned char>(futari, sizeof(futari));
  poster_map_["futari15"] = std::span<unsigned char>(futari, sizeof(futari));
  poster_map_["futari15a"] = std::span<unsigned char>(futari, sizeof(futari));
  poster_map_["futariblk"] = std::span<unsigned char>(futari, sizeof(futari));

  poster_map_["ibara"] = std::span<unsigned char>(ibara, sizeof(ibara));
  poster_map_["ibarablk"] = std::span<unsigned char>(ibara, sizeof(ibara));
  poster_map_["ibarablka"] = std::span<unsigned char>(ibara, sizeof(ibara));

  poster_map_["mmmbanc"] = std::span<unsigned char>(mmmbanc, sizeof(mmmbanc));

  poster_map_["mmpork"] = std::span<unsigned char>(mmpork, sizeof(mmpork));

  poster_map_["mushisam"] =
      std::span<unsigned char>(mushisam, sizeof(mushisam));
  poster_map_["mushisama"] =
      std::span<unsigned char>(mushisam, sizeof(mushisam));
  poster_map_["mushisamb"] =
      std::span<unsigned char>(mushisam, sizeof(mushisam));

  poster_map_["mushitam"] =
      std::span<unsigned char>(mushitam, sizeof(mushitam));
  poster_map_["mushitama"] =
      std::span<unsigned char>(mushitam, sizeof(mushitam));

  poster_map_["pinkswts"] =
      std::span<unsigned char>(pinkswts, sizeof(pinkswts));
  poster_map_["pinkswtsa"] =
      std::span<unsigned char>(pinkswts, sizeof(pinkswts));
  poster_map_["pinkswtsb"] =
      std::span<unsigned char>(pinkswts, sizeof(pinkswts));
  poster_map_["pinkswtsx"] =
      std::span<unsigned char>(pinkswts, sizeof(pinkswts));

  poster_map_["dfk10"] = std::span<unsigned char>(dfk, sizeof(dfk));
  poster_map_["dfk15"] = std::span<unsigned char>(dfk, sizeof(dfk));
}

GLuint UiGameList::LoadPoster(std::string name) {
  GLuint poster_id;

  auto image = poster_map_[name];

  int width, height, channels;
  unsigned char* data = stbi_load_from_memory(image.data(), image.size(),
                                              &width, &height, &channels, 3);
  if (data) {
    glGenTextures(1, &poster_id);
    glBindTexture(GL_TEXTURE_2D, poster_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
  }
  return poster_id;
}

void UiGameList::ScanDirectory(std::string dir, GamesList& game_list) {
  auto count = game_list.GetCount();

  for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
    auto path = entry.path();
    bool found = false;
    std::string name;
    std::string game_path;
    uint32_t idx = 0;
    for (idx = 0; idx < count; idx++) {
      name = game_list.GetGameField(idx, eGameFieldName);
      game_path = path.string();
      if (path.stem().string() == name) {
        found = true;
        break;
      }
    }
    if (!found) {
      continue;
    }
    if (game_list.LoadGame(idx, path.string(), false)) {
      Game game;

      game.game_id = idx;
      game.path = game_path;
      game.name = game_list.GetGameField(idx, eGameFieldFullName);
      game.poster_id = LoadPoster(name);
      game_list_.push_back(game);
    }
  }
}

}  // namespace ui