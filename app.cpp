
#include "app.h"

#include <vector>

SDL_AppResult SDL_Fail() {
  SDL_LogError(SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError());
  return SDL_APP_FAILURE;
}

void ScanDirectory(App* app, std::string dir) {
  app->ui.game_list.ScanDirectory(dir, app->cave3rd.GetGameList());
  if (app->ui.game_list.GetGameList().size() > 0) {
    app->state = kShowGameList;
  }
}

void SDLCALL AudioStreamCallback(void* userdata, SDL_AudioStream* stream,
                                 int additional_amount, int total_amount) {
  static std::vector<int16_t> samples;

  Cave3rd* cave3rd = (Cave3rd*)userdata;

  samples.clear();
  int sample_count = additional_amount >> 1;
  for (int i = 0; i < sample_count; i++) {
    int16_t sample = cave3rd->GetNextSample();
    samples.push_back(sample);
  }
  SDL_PutAudioStreamData(stream, samples.data(), sample_count << 1);
}

bool InitAudio(App* app) {
  SDL_AudioSpec spec;

  spec.freq = 16000;
  spec.format = SDL_AUDIO_S16;
  spec.channels = 1;

  app->audio = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
  if (!app->audio) {
    return false;
  }
  SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, nullptr);
  if (!stream) {
    return false;
  }
  if (!SDL_BindAudioStream(app->audio, stream)) {
    return false;
  }
  if (!SDL_SetAudioStreamGetCallback(stream, AudioStreamCallback,
                                     &app->cave3rd)) {
    return false;
  }
  return true;
}

bool IsCompiled(GLuint shader) {
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success != GL_TRUE) {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
    SDL_Log("Shader error:  %s",
            std::string(errorLog.begin(), errorLog.end()).c_str());
    return false;
  }
  return true;
}

bool IsLinked(GLuint shader) {
  GLint success;
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (success != GL_TRUE) {
    GLint maxLength = 0;
    glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    std::vector<GLchar> errorLog(maxLength);
    glGetProgramInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
    SDL_Log("Shader error:  %s",
            std::string(errorLog.begin(), errorLog.end()).c_str());
    glDeleteProgram(shader);
    return false;
  }
  return true;
}

bool ShaderCompile(std::vector<const GLchar*>& vertex,
                   std::vector<const GLchar*>& pixel, GLuint& shader) {
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, static_cast<GLsizei>(vertex.size()), &vertex[0],
                 nullptr);
  glCompileShader(vertex_shader);
  if (!IsCompiled(vertex_shader)) {
    return false;
  }

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, static_cast<GLsizei>(pixel.size()), &pixel[0],
                 nullptr);
  glCompileShader(fragment_shader);
  if (!IsCompiled(fragment_shader)) {
    return false;
  }

  shader = glCreateProgram();
  glAttachShader(shader, vertex_shader);
  glAttachShader(shader, fragment_shader);
  glLinkProgram(shader);
  if (!IsLinked(shader)) {
    return false;
  }
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return true;
}

bool InitVideo(App* app) {
  std::vector<const GLchar*> vsh;
  std::vector<const GLchar*> psh;

  SDL_GL_SetSwapInterval(1);

  static GLchar vertex_shader[] =
      "in vec2 a_position;\n"
      "in vec2 a_tex_coord;\n"
      "out vec2 v_tex_coord;\n"
      "void main() {\n"
      "	gl_Position = vec4(a_position, 1, 1);\n"
      "	v_tex_coord = a_tex_coord;\n"
      "}\n";

  static GLchar fragment_shader[] =
      "uniform sampler2D u_texture;\n"
      "in vec2 v_tex_coord;\n"
      "out vec4 FragColor;\n"
      "void main() {\n"
      "	FragColor = texture(u_texture, v_tex_coord.xy);\n"
      "}\n";

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  app->video.gl_context = SDL_GL_CreateContext(app->video.window);
  SDL_GL_MakeCurrent(app->video.window, app->video.gl_context);

  if (!gladLoadGLLoader(
          reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
    SDL_Log("gladLoadGLLoader error");
    return false;
  }

  GLuint shader;

#if defined(__APPLE__)
  const char* version = "#version 150\n";
#else
  const char* version = "#version 130\n";
#endif

  vsh.push_back(version);
  vsh.push_back(vertex_shader);

  psh.push_back(version);
  psh.push_back(fragment_shader);
  if (!ShaderCompile(vsh, psh, shader)) {
    return false;
  }

  static Vertex vertex[] = {{-1.0f, 1.0f, 0.0f, 0.0f},
                            {1.0f, 1.0f, 1.0f, 0.0f},
                            {-1.0f, -1.0f, 0.0f, 1.0f},
                            {1.0f, -1.0f, 1.0f, 1.0f}};

  glGenVertexArrays(1, &app->video.vao);
  glGenBuffers(1, &app->video.vbo);
  glBindVertexArray(app->video.vao);
  glBindBuffer(GL_ARRAY_BUFFER, app->video.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (GLvoid*)offsetof(Vertex, x));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (GLvoid*)offsetof(Vertex, tu));
  glBindVertexArray(0);

  app->video.shader = shader;
  app->video.u_texture = glGetUniformLocation(shader, "u_texture");

  glGenTextures(1, &app->video.texture);
  glBindTexture(GL_TEXTURE_2D, app->video.texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 320, 240, 0, GL_RGBA,
               GL_UNSIGNED_SHORT_5_5_5_1, nullptr);

  return true;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  App* app = new App();

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    return SDL_Fail();
  }

  app->video.window = SDL_CreateWindow("NeoCave", 640, 480, SDL_WINDOW_OPENGL);
  if (!app->video.window) {
    return SDL_Fail();
  }

  if (!InitVideo(app)) {
    return SDL_Fail();
  }

  if (!InitAudio(app)) {
    return SDL_Fail();
  }

  app->ui.Init(app->video.window, app->video.gl_context);

  auto& im = app->ui.ui_input.GetInputManager();
  im.InitializeBindings({"Push1", "Push2", "Push3", "Push4", "Coin", "Start",
                         "Up", "Down", "Left", "Right"});

  app->config.AddConfig("input", app->ui.ui_input.GetInputManager());
  if (app->config.Load()) {
    auto& game_path = app->config.state_.path;
    if (game_path.size()) {
      ScanDirectory(app, game_path);
    }
  }

  *appstate = app;

  return SDL_APP_CONTINUE;
}

uint32_t CkeckInputState(InputManager& im) {
  auto actions = im.GetInputActions();

#define CHECK_KEY(key, mask)    \
  if (im.IsActionActive(key)) { \
    button &= ~(mask);          \
  }

  uint32_t button = ~0;

  CHECK_KEY(actions[0], 0x1000)
  CHECK_KEY(actions[1], 0x2000)
  CHECK_KEY(actions[2], 0x4000)
  CHECK_KEY(actions[3], 0x8000)
  CHECK_KEY(actions[4], 0x0004)
  CHECK_KEY(actions[5], 0x0010)
  CHECK_KEY(actions[6], 0x0100)
  CHECK_KEY(actions[7], 0x0200)
  CHECK_KEY(actions[8], 0x0400)
  CHECK_KEY(actions[9], 0x0800)

  return button;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  auto* app = (App*)appstate;
  Cave3rd& cave3rd = app->cave3rd;

  app->ui.HandleEvent(event);
  cave3rd.SetInputState(CkeckInputState(app->ui.ui_input.GetInputManager()));

  switch (event->type) {
    case SDL_EVENT_QUIT:
      app->app_quit = SDL_APP_SUCCESS;
      break;
    case SDL_EVENT_KEY_DOWN:
      if (event->key.scancode == SDL_SCANCODE_TAB) {
        app->input_setting = !app->input_setting;
      }
      break;
    default:
      break;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  auto* app = (App*)appstate;

  if (app->state != kRunning || app->input_setting) {
    app->ui.Begin();

    if (app->state == kLoadGameList) {
      std::string dir;
      auto status = app->ui.file_dialog.Open(dir);
      if (status == ui::FileDialog::kOk) {
        if (app->config.state_.path != dir) {
          app->config.state_.path = dir;
          ScanDirectory(app, dir);
          app->config.Save();
        }
      }
    } else if (app->input_setting) {
      app->ui.ui_input.Show();
      app->config.Save();
    } else if (app->state == kShowGameList) {
      auto game = app->ui.ShowGameList();
      if (nullptr != game) {
        app->cave3rd.SetGame(game->game_id, game->path);
        app->cave3rd.Start();
        app->state = kRunning;
      }
    }
    app->ui.End();
  } else {
    glViewport(0, 0, 640, 480);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(app->video.shader);
    glBindVertexArray(app->video.vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, app->video.texture);
    glUniform1i(app->video.texture, 0);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 320, 240, GL_RGBA,
                    GL_UNSIGNED_SHORT_5_5_5_1, app->cave3rd.GetBlitterData());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  SDL_GL_SwapWindow(app->video.window);
  return app->app_quit;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  auto* app = (App*)appstate;
  if (app) {
    SDL_CloseAudioDevice(app->audio);
    app->cave3rd.Stop();
    app->ui.Close();
    delete app;
  }
}