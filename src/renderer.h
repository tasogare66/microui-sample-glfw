#pragma once
#include "microuipp.h"

void r_init(void);
void r_draw_rect(mui::Rect rect, mui::Color color);
void r_draw_text(const char *text, mui::Vec2 pos, mui::Color color);
void r_draw_icon(int id, mui::Rect rect, mui::Color color);
 int r_get_text_width(const char *text, int len);
 int r_get_text_height(void);
void r_set_clip_rect(mui::Rect rect);
void r_clear(mui::Color color);
void r_present(void);
