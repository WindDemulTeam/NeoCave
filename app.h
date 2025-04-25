#pragma once

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad.h>

#include "cave.h"
#include "config.h"
#include "ui.h"

enum State {
  kLoadGameList,
  kShowGameList,
  kRunning,
};

struct Video {
  SDL_Window* window;
  SDL_GLContext gl_context;

  GLuint shader;

  GLuint a_position;
  GLuint a_tex_coord;
  GLuint u_texture;
  GLuint texture;
};

struct Vertex {
  GLfloat x, y;
  GLfloat tu, tv;
};

struct App {
  Video video;
  SDL_AudioDeviceID audio;
  Cave3rd cave3rd;
  config::Config config;
  ui::Ui ui;
  SDL_AppResult app_quit = SDL_APP_CONTINUE;
  State state = kLoadGameList;
  bool input_setting = false;
};
