#include <cstring>
#include <assert.h>
#include <glad/glad.h>
#include <linmath.h>
#include "renderer.h"

#include "atlas.inl"

#define BUFFER_SIZE 16384

static GLfloat   tex_buf[BUFFER_SIZE *  8];
static GLfloat  vert_buf[BUFFER_SIZE *  8];
static GLubyte color_buf[BUFFER_SIZE * 16];
static GLuint  index_buf[BUFFER_SIZE *  6];

static GLuint atlas_tex_id;
static GLuint vertex_shader, fragment_shader, program;
static GLint mvp_location;
static GLuint VAO, VBO[3], EBO;

static int width  = 800;
static int height = 600;
static int buf_idx;

const char* vertex_shader_text = "#version 330 core\n"
"uniform mat4 MVP;\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec4 aColor;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"out vec4 fColor;\n"
"out vec2 texCoord;\n"
"void main()\n"
"{\n"
"   gl_Position = MVP * vec4(aPos, 0.0, 1.0);\n"
"   fColor = aColor;\n"
"   texCoord = aTexCoord;\n"
"}\0";
//Fragment Shader source code
const char* fragment_shader_text = "#version 330 core\n"
"in vec4 fColor;\n"
"in vec2 texCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D tex0;\n"
"void main()\n"
"{\n"
"   //FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);\n"
"   //FragColor = fColor;\n"
"   FragColor = vec4(1.0, 1.0, 1.0, texture(tex0, texCoord).r) * fColor;\n"
"}\n\0";

void r_init(void) {
  /* init gl */
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  assert(glGetError() == 0);

  /* init texture */
  glGenTextures(1, &atlas_tex_id);
  glBindTexture(GL_TEXTURE_2D, atlas_tex_id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_WIDTH, ATLAS_HEIGHT, 0,
    GL_RED, GL_UNSIGNED_BYTE, atlas_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  assert(glGetError() == 0);

  // buffer
  {
    // Create reference containers for the Vartex Array Object, the Vertex Buffer Object, and the Element Buffer Object

    // Generate the VAO, VBO, and EBO with only 1 object each
    glGenVertexArrays(1, &VAO);
    glGenBuffers(3, VBO);
    glGenBuffers(1, &EBO);

    // Make the VAO the current Vertex Array Object by binding it
    glBindVertexArray(VAO);

    // Bind the EBO specifying it's a GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // Bind the VBO specifying it's a GL_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    // Introduce the vertices into the VBO
    // Configure the Vertex Attribute so that OpenGL knows how to read the VBO
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    // Enable the Vertex Attribute so that OpenGL knows to use it
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(GLubyte), (void*)0);
    glEnableVertexAttribArray(1);
    // tex coord
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);

    // Bind both the VBO and VAO to 0 so that we don't accidentally modify the VAO and VBO we created
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Bind the EBO to 0 so that we don't accidentally modify it
    // MAKE SURE TO UNBIND IT AFTER UNBINDING THE VAO, as the EBO is linked in the VAO
    // This does not apply to the VBO because the VBO is already linked to the VAO during glVertexAttribPointer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(glGetError() == 0);
  }

  // setup shader
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  assert(glGetError() == 0);

  mvp_location = glGetUniformLocation(program, "MVP");
  assert(glGetError() == 0);
}


static void flush(void) {
  if (buf_idx == 0) { return; }

  const float ratio = width / (float)height;
  glViewport(0, 0, width, height);

  mat4x4 m, p, mvp;
  mat4x4_identity(m);
  mat4x4_ortho(p, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f, -1.f);
  mat4x4_mul(mvp, p, m);

  glUseProgram(program);
  glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)mvp);

  glBindTexture(GL_TEXTURE_2D, atlas_tex_id);
  glBindVertexArray(VAO);
  // Bind the VBO specifying it's a GL_ARRAY_BUFFER
  // vertex
  glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buf_idx * 8, vert_buf, GL_STATIC_DRAW);
  // color
  glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLubyte) * buf_idx * 16, color_buf, GL_STATIC_DRAW);
  // tex coord
  glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buf_idx * 8, tex_buf, GL_STATIC_DRAW);
  // Bind the EBO specifying it's a GL_ELEMENT_ARRAY_BUFFER
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * buf_idx * 6, index_buf, GL_STATIC_DRAW);
  glDrawElements(GL_TRIANGLES, buf_idx * 6, GL_UNSIGNED_INT, 0);

  buf_idx = 0;
}


static void push_quad(mui::Rect dst, mui::Rect src, mui::Color color) {
  if (buf_idx == BUFFER_SIZE) { flush(); }

  int texvert_idx = buf_idx *  8;
  int   color_idx = buf_idx * 16;
  int element_idx = buf_idx *  4;
  int   index_idx = buf_idx *  6;
  buf_idx++;

  /* update texture buffer */
  float x = src.x / (float) ATLAS_WIDTH;
  float y = src.y / (float) ATLAS_HEIGHT;
  float w = src.w / (float) ATLAS_WIDTH;
  float h = src.h / (float) ATLAS_HEIGHT;
  tex_buf[texvert_idx + 0] = x;
  tex_buf[texvert_idx + 1] = y;
  tex_buf[texvert_idx + 2] = x + w;
  tex_buf[texvert_idx + 3] = y;
  tex_buf[texvert_idx + 4] = x;
  tex_buf[texvert_idx + 5] = y + h;
  tex_buf[texvert_idx + 6] = x + w;
  tex_buf[texvert_idx + 7] = y + h;

  /* update vertex buffer */
  vert_buf[texvert_idx + 0] = static_cast<GLfloat>(dst.x);
  vert_buf[texvert_idx + 1] = static_cast<GLfloat>(dst.y);
  vert_buf[texvert_idx + 2] = static_cast<GLfloat>(dst.x + dst.w);
  vert_buf[texvert_idx + 3] = static_cast<GLfloat>(dst.y);
  vert_buf[texvert_idx + 4] = static_cast<GLfloat>(dst.x);
  vert_buf[texvert_idx + 5] = static_cast<GLfloat>(dst.y + dst.h);
  vert_buf[texvert_idx + 6] = static_cast<GLfloat>(dst.x + dst.w);
  vert_buf[texvert_idx + 7] = static_cast<GLfloat>(dst.y + dst.h);

  /* update color buffer */
  std::memcpy(color_buf + color_idx +  0, &color, 4);
  std::memcpy(color_buf + color_idx +  4, &color, 4);
  std::memcpy(color_buf + color_idx +  8, &color, 4);
  std::memcpy(color_buf + color_idx + 12, &color, 4);

  /* update index buffer */
  index_buf[index_idx + 0] = element_idx + 0;
  index_buf[index_idx + 1] = element_idx + 1;
  index_buf[index_idx + 2] = element_idx + 2;
  index_buf[index_idx + 3] = element_idx + 2;
  index_buf[index_idx + 4] = element_idx + 3;
  index_buf[index_idx + 5] = element_idx + 1;
}


void r_draw_rect(mui::Rect rect, mui::Color color) {
  push_quad(rect, atlas[ATLAS_WHITE], color);
}


void r_draw_text(const char *text, mui::Vec2 pos, mui::Color color) {
  mui::Rect dst = { pos.x, pos.y, 0, 0 };
  for (const char *p = text; *p; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = std::min<uint8_t>((unsigned char) *p, 127);
    mui::Rect src = atlas[ATLAS_FONT + chr];
    dst.w = src.w;
    dst.h = src.h;
    push_quad(dst, src, color);
    dst.x += dst.w;
  }
}


void r_draw_icon(int id, mui::Rect rect, mui::Color color) {
  mui::Rect src = atlas[id];
  int x = rect.x + (rect.w - src.w) / 2;
  int y = rect.y + (rect.h - src.h) / 2;
  push_quad(mui::Rect(x, y, src.w, src.h), src, color);
}


int r_get_text_width(const char *text, int len) {
  int res = 0;
  for (const char *p = text; *p && len--; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = std::min<uint8_t>((unsigned char) *p, 127);
    res += atlas[ATLAS_FONT + chr].w;
  }
  return res;
}


int r_get_text_height(void) {
  return 18;
}


void r_set_clip_rect(mui::Rect rect) {
  flush();
  glScissor(rect.x, height - (rect.y + rect.h), rect.w, rect.h);
}


void r_clear(mui::Color clr) {
  flush();
  glClearColor(static_cast<GLfloat>(clr.r / 255.), static_cast<GLfloat>(clr.g / 255.), static_cast<GLfloat>(clr.b / 255.), static_cast<GLfloat>(clr.a / 255.));
  glClear(GL_COLOR_BUFFER_BIT);
}


void r_present(void) {
  flush();
}
