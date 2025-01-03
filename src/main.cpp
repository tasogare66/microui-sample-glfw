﻿#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <string>
extern "C" {
#include "microui.h"
}
#include "renderer.h"

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

static double mouse_wheel = 0.0;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  mouse_wheel = yoffset;
}

static uint32_t character_codepoint = 0;
static void character_callback(GLFWwindow* window, unsigned int codepoint)
{
  character_codepoint = codepoint;
}

static  char logbuf[64000];
static   int logbuf_updated = 0;
static float bg[3] = { 90, 95, 100 };

static void write_log(const char* text) {
  //FIXME:strcat_s  
  if (logbuf[0]) { strcat_s(logbuf, "\n"); }
  strcat_s(logbuf, text);
  logbuf_updated = 1;
}

static void test_window(mu_Context* ctx) {
  /* do window */
  if (mu_begin_window(ctx, "Demo Window", mu_rect(40, 40, 300, 450))) {
    mu_Container* win = mu_get_current_container(ctx);
    win->rect.w = mu_max(win->rect.w, 240);
    win->rect.h = mu_max(win->rect.h, 300);

    /* window info */
    if (mu_header(ctx, "Window Info")) {
      mu_Container* win = mu_get_current_container(ctx);
      char buf[64];
      const int widths[] = { 54, -1 };
      mu_layout_row(ctx, 2, widths, 0);
      mu_label(ctx, "Position:");
      snprintf(buf, sizeof(buf), "%d, %d", win->rect.x, win->rect.y); mu_label(ctx, buf);
      mu_label(ctx, "Size:");
      snprintf(buf, sizeof(buf), "%d, %d", win->rect.w, win->rect.h); mu_label(ctx, buf);
    }

    /* labels + buttons */
    if (mu_header_ex(ctx, "Test Buttons", MU_OPT_EXPANDED)) {
      const int widths[] = { 86, -110, -1 };
      mu_layout_row(ctx, 3, widths, 0);
      mu_label(ctx, "Test buttons 1:");
      if (mu_button(ctx, "Button 1")) { write_log("Pressed button 1"); }
      if (mu_button(ctx, "Button 2")) { write_log("Pressed button 2"); }
      mu_label(ctx, "Test buttons 2:");
      if (mu_button(ctx, "Button 3")) { write_log("Pressed button 3"); }
      if (mu_button(ctx, "Popup")) { mu_open_popup(ctx, "Test Popup"); }
      if (mu_begin_popup(ctx, "Test Popup")) {
        mu_button(ctx, "Hello");
        mu_button(ctx, "World");
        mu_end_popup(ctx);
      }
    }

    /* tree */
    if (mu_header_ex(ctx, "Tree and Text", MU_OPT_EXPANDED)) {
      const int widths0[] = { 140, -1 };
      mu_layout_row(ctx, 2, widths0, 0);
      mu_layout_begin_column(ctx);
      if (mu_begin_treenode(ctx, "Test 1")) {
        if (mu_begin_treenode(ctx, "Test 1a")) {
          mu_label(ctx, "Hello");
          mu_label(ctx, "world");
          mu_end_treenode(ctx);
        }
        if (mu_begin_treenode(ctx, "Test 1b")) {
          if (mu_button(ctx, "Button 1")) { write_log("Pressed button 1"); }
          if (mu_button(ctx, "Button 2")) { write_log("Pressed button 2"); }
          mu_end_treenode(ctx);
        }
        mu_end_treenode(ctx);
      }
      if (mu_begin_treenode(ctx, "Test 2")) {
        const int widths1[] = { 54, 54 };
        mu_layout_row(ctx, 2, widths1, 0);
        if (mu_button(ctx, "Button 3")) { write_log("Pressed button 3"); }
        if (mu_button(ctx, "Button 4")) { write_log("Pressed button 4"); }
        if (mu_button(ctx, "Button 5")) { write_log("Pressed button 5"); }
        if (mu_button(ctx, "Button 6")) { write_log("Pressed button 6"); }
        mu_end_treenode(ctx);
      }
      if (mu_begin_treenode(ctx, "Test 3")) {
        static int checks[3] = { 1, 0, 1 };
        mu_checkbox(ctx, "Checkbox 1", &checks[0]);
        mu_checkbox(ctx, "Checkbox 2", &checks[1]);
        mu_checkbox(ctx, "Checkbox 3", &checks[2]);
        mu_end_treenode(ctx);
      }
      mu_layout_end_column(ctx);

      mu_layout_begin_column(ctx);
      const int widths2[] = { -1 };
      mu_layout_row(ctx, 1, widths2, 0);
      mu_text(ctx, "Lorem ipsum dolor sit amet, consectetur adipiscing "
        "elit. Maecenas lacinia, sem eu lacinia molestie, mi risus faucibus "
        "ipsum, eu varius magna felis a nulla.");
      mu_layout_end_column(ctx);
    }

    /* background color sliders */
    if (mu_header_ex(ctx, "Background Color", MU_OPT_EXPANDED)) {
      const int widths0[] = { -78, -1 };
      mu_layout_row(ctx, 2, widths0, 74);
      /* sliders */
      mu_layout_begin_column(ctx);
      const int widths1[] = { 46, -1 };
      mu_layout_row(ctx, 2, widths1, 0);
      mu_label(ctx, "Red:");   mu_slider(ctx, &bg[0], 0, 255);
      mu_label(ctx, "Green:"); mu_slider(ctx, &bg[1], 0, 255);
      mu_label(ctx, "Blue:");  mu_slider(ctx, &bg[2], 0, 255);
      mu_layout_end_column(ctx);
      /* color preview */
      mu_Rect r = mu_layout_next(ctx);
      mu_draw_rect(ctx, r, mu_color(static_cast<int>(bg[0]), static_cast<int>(bg[1]), static_cast<int>(bg[2]), 255));
      char buf[32];
      snprintf(buf, sizeof(buf), "#%02X%02X%02X", (int)bg[0], (int)bg[1], (int)bg[2]);
      mu_draw_control_text(ctx, buf, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
    }

    mu_end_window(ctx);
  }
}

static void log_window(mu_Context* ctx) {
  if (mu_begin_window(ctx, "Log Window", mu_rect(350, 40, 300, 200))) {
    /* output text panel */
    const int widths0[] = { -1 };
    mu_layout_row(ctx, 1, widths0, -25);
    mu_begin_panel(ctx, "Log Output");
    mu_Container* panel = mu_get_current_container(ctx);
    const int widths1[] = { -1 };
    mu_layout_row(ctx, 1, widths1, -1);
    mu_text(ctx, logbuf);
    mu_end_panel(ctx);
    if (logbuf_updated) {
      panel->scroll.y = panel->content_size.y;
      logbuf_updated = 0;
    }

    /* input textbox + submit button */
    static char buf[128];
    int submitted = 0;
    const int widths2[] = { -70, -1 };
    mu_layout_row(ctx, 2, widths2, 0);
    if (mu_textbox(ctx, buf, sizeof(buf)) & MU_RES_SUBMIT) {
      mu_set_focus(ctx, ctx->last_id);
      submitted = 1;
    }
    if (mu_button(ctx, "Submit")) { submitted = 1; }
    if (submitted) {
      write_log(buf);
      buf[0] = '\0';
    }

    mu_end_window(ctx);
  }
}

static int uint8_slider(mu_Context* ctx, unsigned char* value, int low, int high) {
  static float tmp;
  mu_push_id(ctx, &value, sizeof(value));
  tmp = *value;
  int res = mu_slider_ex(ctx, &tmp, static_cast<mu_Real>(low), static_cast<mu_Real>(high), 0, "%.0f", MU_OPT_ALIGNCENTER);
  *value = static_cast<uint8_t>(tmp);
  mu_pop_id(ctx);
  return res;
}

static void style_window(mu_Context* ctx) {
  static struct { const char* label; int idx; } colors[] = {
    { "text:",         MU_COLOR_TEXT        },
    { "border:",       MU_COLOR_BORDER      },
    { "windowbg:",     MU_COLOR_WINDOWBG    },
    { "titlebg:",      MU_COLOR_TITLEBG     },
    { "titletext:",    MU_COLOR_TITLETEXT   },
    { "panelbg:",      MU_COLOR_PANELBG     },
    { "button:",       MU_COLOR_BUTTON      },
    { "buttonhover:",  MU_COLOR_BUTTONHOVER },
    { "buttonfocus:",  MU_COLOR_BUTTONFOCUS },
    { "base:",         MU_COLOR_BASE        },
    { "basehover:",    MU_COLOR_BASEHOVER   },
    { "basefocus:",    MU_COLOR_BASEFOCUS   },
    { "scrollbase:",   MU_COLOR_SCROLLBASE  },
    { "scrollthumb:",  MU_COLOR_SCROLLTHUMB },
    { NULL }
  };

  if (mu_begin_window(ctx, "Style Editor", mu_rect(350, 250, 300, 240))) {
    int sw = static_cast<int>(mu_get_current_container(ctx)->body.w * 0.14);
    //mu_layout_row(ctx, 6, (int[]) { 80, sw, sw, sw, sw, -1 }, 0);
    const int widths[] = { 80, sw, sw, sw, sw, -1 };
    mu_layout_row(ctx, 6, widths, 0);
    for (int i = 0; colors[i].label; i++) {
      mu_label(ctx, colors[i].label);
      uint8_slider(ctx, &ctx->style->colors[i].r, 0, 255);
      uint8_slider(ctx, &ctx->style->colors[i].g, 0, 255);
      uint8_slider(ctx, &ctx->style->colors[i].b, 0, 255);
      uint8_slider(ctx, &ctx->style->colors[i].a, 0, 255);
      mu_draw_rect(ctx, mu_layout_next(ctx), ctx->style->colors[i]);
    }
    mu_end_window(ctx);
  }
}

void process_frame(mu_Context* ctx) {
  mu_begin(ctx);
  style_window(ctx);
  log_window(ctx);
  test_window(ctx);
  mu_end(ctx);
}

static int text_width(mu_Font font, const char* text, int len) {
  if (len == -1) { len = static_cast<int>(strlen(text)); }
  return r_get_text_width(text, len);
}

static int text_height(mu_Font font) {
  return r_get_text_height();
}

int main(void)
{
  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(800, 600, "microui-sample", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);
  glfwSetCharCallback(window, character_callback);
  glfwSetScrollCallback(window, scroll_callback);

  glfwMakeContextCurrent(window);
  gladLoadGL(); // gladLoadGL(glfwGetProcAddress) if glad generated without a loader
  glfwSwapInterval(1);

  //glClearColor(0.0, 0.0, 0.0, 1.0);
  r_init();
  /* init microui */
  mu_Context* ctx = reinterpret_cast<mu_Context*>(malloc(sizeof(mu_Context)));
  mu_init(ctx);
  ctx->text_width = text_width;
  ctx->text_height = text_height;

  double prv_xpos = 0.0, prv_ypos = 0.0;
  uint32_t prv_mousedown = 0, prv_mouseup = 0;
  uint32_t prv_keydown = 0, prv_keyup = 0;
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    {
      // mouse
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);
      prv_xpos = xpos;
      prv_ypos = ypos;
      uint32_t mousedown = 0, mouseup = 0;
      constexpr std::pair<int, int> tbl[] = {
        {GLFW_MOUSE_BUTTON_LEFT, MU_MOUSE_LEFT},
        {GLFW_MOUSE_BUTTON_MIDDLE, MU_MOUSE_MIDDLE},
        {GLFW_MOUSE_BUTTON_RIGHT, MU_MOUSE_RIGHT},
      };
      for (const auto& p : tbl) {
        auto b = glfwGetMouseButton(window, p.first);
        if (b == GLFW_PRESS) {
          mousedown |= p.second;
        } else if (b == GLFW_RELEASE) {
          mouseup |= p.second;
        }
      }
      auto trig_mousedown = ~prv_mousedown & mousedown;
      auto trig_mouseup = ~prv_mouseup & mouseup;
      mu_input_mousedown(ctx, static_cast<int>(xpos), static_cast<int>(ypos), trig_mousedown);
      mu_input_mouseup(ctx, static_cast<int>(xpos), static_cast<int>(ypos), trig_mouseup);
      prv_mousedown = mousedown;
      prv_mouseup = mouseup;
      // wheel
      if (mouse_wheel != 0.0) {
        mu_input_scroll(ctx, 0, static_cast<int>(mouse_wheel * -30));
      }
      mouse_wheel = 0.0;
      // text
      if (character_codepoint) {
        char text[16] = {};
        memcpy(text, &character_codepoint, sizeof(character_codepoint));
        mu_input_text(ctx, text);
        character_codepoint = 0;
      }
      // key
      {
        uint32_t keydown = 0, keyup = 0;
        constexpr std::pair<int, int> keytbl[] = {
          {GLFW_KEY_LEFT_SHIFT, MU_KEY_SHIFT},
          {GLFW_KEY_RIGHT_SHIFT, MU_KEY_SHIFT},
          {GLFW_KEY_LEFT_CONTROL, MU_KEY_CTRL},
          {GLFW_KEY_RIGHT_CONTROL, MU_KEY_CTRL},
          {GLFW_KEY_LEFT_ALT, MU_KEY_ALT},
          {GLFW_KEY_RIGHT_ALT, MU_KEY_ALT},
          {GLFW_KEY_ENTER, MU_KEY_RETURN},
          {GLFW_KEY_BACKSPACE, MU_KEY_BACKSPACE},
        };
        for (const auto& p : keytbl) {
          int state = glfwGetKey(window, p.first);
          if (state == GLFW_PRESS) { keydown |= p.second; }
          else if (state == GLFW_RELEASE) { keyup |= p.second; }
        }
        auto trig_keydown = ~prv_keydown & keydown;
        auto trig_keyup = ~prv_keyup & keyup;
        mu_input_keydown(ctx, trig_keydown);
        mu_input_keyup(ctx, trig_keyup);
        prv_keydown = keydown;
        prv_keyup = keyup;
      }
    }

    glClear(GL_COLOR_BUFFER_BIT);

    /* process frame */
    process_frame(ctx);

    /* render */
    r_clear(mu_color(static_cast<int>(bg[0]), static_cast<int>(bg[1]), static_cast<int>(bg[2]), 255));
    mu_Command* cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
      switch (cmd->type) {
      case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
      case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
      case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
      case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
      }
    }

    r_present();
    glfwSwapBuffers(window);
  }

  glfwDestroyWindow(window);

  glfwTerminate();
  exit(EXIT_SUCCESS);
}