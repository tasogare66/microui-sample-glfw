#include "microuipp.h"
namespace mui {
#define unused(x) ((void) (x))

#define expect(x) do {                                               \
    if (!(x)) {                                                      \
      fprintf(stderr, "Fatal error: %s:%d: assertion '%s' failed\n", \
        __FILE__, __LINE__, #x);                                     \
      abort();                                                       \
    }                                                                \
  } while (0)

static constexpr Rect unclipped_rect = { 0, 0, 0x1000000, 0x1000000 };
static constexpr Style default_style = {
  /* font | size | padding | spacing | indent */
  nullptr, { 68, 10 }, 5, 4, 24,
  /* title_height | scrollbar_size | thumb_size */
  24, 12, 8,
  {
    { 230, 230, 230, 255 }, /* MU_COLOR_TEXT */
    { 25,  25,  25,  255 }, /* MU_COLOR_BORDER */
    { 50,  50,  50,  255 }, /* MU_COLOR_WINDOWBG */
    { 25,  25,  25,  255 }, /* MU_COLOR_TITLEBG */
    { 240, 240, 240, 255 }, /* MU_COLOR_TITLETEXT */
    { 0,   0,   0,   0   }, /* MU_COLOR_PANELBG */
    { 75,  75,  75,  255 }, /* MU_COLOR_BUTTON */
    { 95,  95,  95,  255 }, /* MU_COLOR_BUTTONHOVER */
    { 115, 115, 115, 255 }, /* MU_COLOR_BUTTONFOCUS */
    { 30,  30,  30,  255 }, /* MU_COLOR_BASE */
    { 35,  35,  35,  255 }, /* MU_COLOR_BASEHOVER */
    { 40,  40,  40,  255 }, /* MU_COLOR_BASEFOCUS */
    { 43,  43,  43,  255 }, /* MU_COLOR_SCROLLBASE */
    { 30,  30,  30,  255 }  /* MU_COLOR_SCROLLTHUMB */
  }
};

static void draw_frame(Context* ctx, Rect rect, int colorid) {
  draw_rect(ctx, rect, ctx->style->colors[colorid]);
  if (colorid == MU_COLOR_SCROLLBASE ||
    colorid == MU_COLOR_SCROLLTHUMB ||
    colorid == MU_COLOR_TITLEBG) {
    return;
  }
  /* draw border */
  if (ctx->style->colors[MU_COLOR_BORDER].a) {
    draw_box(ctx, rect.expand(1), ctx->style->colors[MU_COLOR_BORDER]);
  }
}

void init(Context* ctx) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->draw_frame = draw_frame;
  ctx->_style = default_style;
  ctx->style = &ctx->_style;
}


void begin(Context* ctx) {
  expect(ctx->text_width && ctx->text_height);
  ctx->command_list.idx = 0;
  ctx->root_list.idx = 0;
  ctx->scroll_target = nullptr;
  ctx->hover_root = ctx->next_hover_root;
  ctx->next_hover_root = nullptr;
  ctx->mouse_delta.x = ctx->mouse_pos.x - ctx->last_mouse_pos.x;
  ctx->mouse_delta.y = ctx->mouse_pos.y - ctx->last_mouse_pos.y;
  ctx->frame += 1;
}

void end(Context* ctx) {
  int32_t i = 0;
  /* check stacks */
  expect(ctx->container_stack.idx == 0);
  expect(ctx->clip_stack.idx == 0);
  expect(ctx->id_stack.idx == 0);
  expect(ctx->layout_stack.idx == 0);

  /* handle scroll input */
  if (ctx->scroll_target) {
    ctx->scroll_target->scroll.x += ctx->scroll_delta.x;
    ctx->scroll_target->scroll.y += ctx->scroll_delta.y;
  }

  /* unset focus if focus id was not touched this frame */
  if (!ctx->updated_focus) { ctx->focus = 0; }
  ctx->updated_focus = 0;

  /* bring hover root to front if mouse was pressed */
  if (ctx->mouse_pressed && ctx->next_hover_root &&
    ctx->next_hover_root->zindex < ctx->last_zindex &&
    ctx->next_hover_root->zindex >= 0
    ) {
    bring_to_front(ctx, ctx->next_hover_root);
  }

  /* reset input state */
  ctx->key_pressed = 0;
  ctx->input_text[0] = '\0';
  ctx->mouse_pressed = 0;
  ctx->scroll_delta = Vec2(0, 0);
  ctx->last_mouse_pos = ctx->mouse_pos;

  /* sort root containers by zindex */
  int32_t n = ctx->root_list.idx;
  //qsort(ctx->root_list.items, n, sizeof(Container*), compare_zindex);
  std::sort(std::begin(ctx->root_list.items), std::begin(ctx->root_list.items) + n, [](const auto& a, const auto& b) { return a->zindex < b->zindex; });

  /* set root container jump commands */
  for (i = 0; i < n; i++) {
    Container* cnt = ctx->root_list.items[i];
    /* if this is the first container then make the first command jump to it.
    ** otherwise set the previous container's tail to jump to this one */
    if (i == 0) {
      Command* cmd = (Command*)ctx->command_list.items.data();
      cmd->jump.dst = (char*)cnt->head + sizeof(mu_JumpCommand);
    } else {
      Container* prev = ctx->root_list.items[i - 1];
      prev->tail->jump.dst = (char*)cnt->head + sizeof(mu_JumpCommand);
    }
    /* make the last container's tail jump to the end of command list */
    if (i == n - 1) {
      cnt->tail->jump.dst = ctx->command_list.items.data() + ctx->command_list.idx;
    }
  }
}

void set_focus(Context* ctx, muId id) {
  ctx->focus = id;
  ctx->updated_focus = 1;
}

/* 32bit fnv-1a hash */
static constexpr muId HASH_INITIAL = 2166136261;

static void hash(muId* hash, const void* data, size_t size) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
  while (size--) {
    *hash = (*hash ^ *p++) * 16777619;
  }
}

muId get_id(Context* ctx, const void* data, int32_t size) {
  const auto idx = ctx->id_stack.idx;
  muId res = (idx > 0) ? ctx->id_stack.items[idx - 1] : HASH_INITIAL;
  hash(&res, data, size);
  ctx->last_id = res;
  return res;
}

void push_id(Context* ctx, const void* data, int32_t size) {
  ctx->id_stack.push(get_id(ctx, data, size));
}

void pop_id(Context* ctx) {
  ctx->id_stack.pop();
}

void push_clip_rect(Context* ctx, Rect rect) {
  Rect last = get_clip_rect(ctx);
  ctx->clip_stack.push(rect.insert(last));
}

void pop_clip_rect(Context* ctx) {
  ctx->clip_stack.pop();
}

Rect get_clip_rect(Context* ctx) {
  assert(ctx->clip_stack.idx > 0);
  return ctx->clip_stack.items[ctx->clip_stack.idx - 1];
}

int32_t check_clip(Context* ctx, Rect r) {
  Rect cr = get_clip_rect(ctx);
  if (r.x > cr.x + cr.w || r.x + r.w < cr.x ||
    r.y > cr.y + cr.h || r.y + r.h < cr.y) {
    return MU_CLIP_ALL;
  }
  if (r.x >= cr.x && r.x + r.w <= cr.x + cr.w &&
    r.y >= cr.y && r.y + r.h <= cr.y + cr.h) {
    return 0;
  }
  return MU_CLIP_PART;
}

static void push_layout(Context* ctx, Rect body, Vec2 scroll) {
  Layout layout;
  int width = 0;
  memset(&layout, 0, sizeof(layout));
  layout.body = Rect(body.x - scroll.x, body.y - scroll.y, body.w, body.h);
  layout.max = Vec2(-0x1000000, -0x1000000);
  ctx->layout_stack.push(layout);
  layout_row(ctx, 1, &width, 0);
}

static Layout* get_layout(Context* ctx) {
  return &ctx->layout_stack.items[ctx->layout_stack.idx - 1];
}

static void pop_container(Context* ctx) {
  Container* cnt = get_current_container(ctx);
  Layout* layout = get_layout(ctx);
  cnt->content_size.x = layout->max.x - layout->body.x;
  cnt->content_size.y = layout->max.y - layout->body.y;
  /* pop container, layout and id */
  ctx->container_stack.pop();
  ctx->layout_stack.pop();
  pop_id(ctx);
}

Container* get_current_container(Context* ctx) {
  assert(ctx->container_stack.idx > 0);
  return ctx->container_stack.items[ctx->container_stack.idx - 1];
}

static Container* get_container(Context* ctx, muId id, int opt) {
  Container* cnt;
  /* try to get existing container from pool */
  int idx = pool_get(ctx, ctx->container_pool, MU_CONTAINERPOOL_SIZE, id);
  if (idx >= 0) {
    if (ctx->containers[idx].open || ~opt & MU_OPT_CLOSED) {
      pool_update(ctx, ctx->container_pool, idx);
    }
    return &ctx->containers[idx];
  }
  if (opt & MU_OPT_CLOSED) { return NULL; }
  /* container not found in pool: init new container */
  idx = pool_init(ctx, ctx->container_pool, MU_CONTAINERPOOL_SIZE, id);
  cnt = &ctx->containers[idx];
  memset(cnt, 0, sizeof(*cnt));
  cnt->open = 1;
  bring_to_front(ctx, cnt);
  return cnt;
}

Container* get_container(Context* ctx, const char* name) {
  muId id = get_id(ctx, name, strlen(name));
  return get_container(ctx, id, 0);
}

void bring_to_front(Context* ctx, Container* cnt) {
  cnt->zindex = ++ctx->last_zindex;
}

/*============================================================================
** pool
**============================================================================*/

int32_t pool_init(Context* ctx, PoolItem* items, int32_t len, muId id) {
  int32_t i, n = -1, f = ctx->frame;
  for (i = 0; i < len; i++) {
    if (items[i].last_update < f) {
      f = items[i].last_update;
      n = i;
    }
  }
  assert(n > -1);
  items[n].id = id;
  pool_update(ctx, items, n);
  return n;
}

int32_t pool_get(Context* ctx, PoolItem* items, int32_t len, muId id) {
  int32_t i;
  unused(ctx);
  for (i = 0; i < len; i++) {
    if (items[i].id == id) { return i; }
  }
  return -1;
}

void pool_update(Context* ctx, PoolItem* items, int32_t idx) {
  items[idx].last_update = ctx->frame;
}

/*============================================================================
** input handlers
**============================================================================*/

void input_mousemove(Context* ctx, int32_t x, int32_t y) {
  ctx->mouse_pos = Vec2(x, y);
}

void input_mousedown(Context* ctx, int32_t x, int32_t y, int32_t btn) {
  input_mousemove(ctx, x, y);
  ctx->mouse_down |= btn;
  ctx->mouse_pressed |= btn;
}

void input_mouseup(Context* ctx, int32_t x, int32_t y, int32_t btn) {
  input_mousemove(ctx, x, y);
  ctx->mouse_down &= ~btn;
}

void input_scroll(Context* ctx, int32_t x, int32_t y) {
  ctx->scroll_delta.x += x;
  ctx->scroll_delta.y += y;
}

void input_keydown(Context* ctx, int32_t key) {
  ctx->key_pressed |= key;
  ctx->key_down |= key;
}

void input_keyup(Context* ctx, int32_t key) {
  ctx->key_down &= ~key;
}

void input_text(Context* ctx, const char* text) {
  int len = strlen(ctx->input_text);
  int size = strlen(text) + 1;
  assert(len + size <= (int)sizeof(ctx->input_text));
  memcpy(ctx->input_text + len, text, size);
}

/*============================================================================
** commandlist
**============================================================================*/

Command* push_command(Context* ctx, int32_t type, int32_t size) {
  Command* cmd = (Command*)(ctx->command_list.items.data() + ctx->command_list.idx);
  assert(ctx->command_list.idx + size < MU_COMMANDLIST_SIZE);
  cmd->base.type = type;
  cmd->base.size = size;
  ctx->command_list.idx += size;
  return cmd;
}

int32_t next_command(Context* ctx, Command** cmd) {
  if (*cmd) {
    *cmd = (Command*)(((char*)*cmd) + (*cmd)->base.size);
  } else {
    *cmd = (Command*)ctx->command_list.items.data();
  }
  while ((char*)*cmd != ctx->command_list.items.data() + ctx->command_list.idx) {
    if ((*cmd)->type != MU_COMMAND_JUMP) { return 1; }
    *cmd = reinterpret_cast<Command*>((*cmd)->jump.dst);
  }
  return 0;
}

static Command* push_jump(Context* ctx, Command* dst) {
  Command* cmd;
  cmd = push_command(ctx, MU_COMMAND_JUMP, sizeof(mu_JumpCommand));
  cmd->jump.dst = dst;
  return cmd;
}

void set_clip(Context* ctx, Rect rect) {
  Command* cmd;
  cmd = push_command(ctx, MU_COMMAND_CLIP, sizeof(mu_ClipCommand));
  cmd->clip.rect = rect;
}

void draw_rect(Context* ctx, Rect rect, Color color) {
  Command* cmd;
  rect = rect.insert(get_clip_rect(ctx));
  if (rect.w > 0 && rect.h > 0) {
    cmd = push_command(ctx, MU_COMMAND_RECT, sizeof(mu_RectCommand));
    cmd->rect.rect = rect;
    cmd->rect.color = color;
  }
}

void draw_box(Context* ctx, Rect rect, Color color) {
  draw_rect(ctx, Rect(rect.x + 1, rect.y, rect.w - 2, 1), color);
  draw_rect(ctx, Rect(rect.x + 1, rect.y + rect.h - 1, rect.w - 2, 1), color);
  draw_rect(ctx, Rect(rect.x, rect.y, 1, rect.h), color);
  draw_rect(ctx, Rect(rect.x + rect.w - 1, rect.y, 1, rect.h), color);
}

void draw_text(Context* ctx, mu_Font font, const char* str, int32_t len, Vec2 pos, Color color)
{
  Command* cmd;
  Rect rect = Rect(
    pos.x, pos.y, ctx->text_width(font, str, len), ctx->text_height(font));
  int clipped = check_clip(ctx, rect);
  if (clipped == MU_CLIP_ALL) { return; }
  if (clipped == MU_CLIP_PART) { set_clip(ctx, get_clip_rect(ctx)); }
  /* add command */
  if (len < 0) { len = strlen(str); }
  cmd = push_command(ctx, MU_COMMAND_TEXT, sizeof(mu_TextCommand) + len);
  memcpy(cmd->text.str, str, len);
  cmd->text.str[len] = '\0';
  cmd->text.pos = pos;
  cmd->text.color = color;
  cmd->text.font = font;
  /* reset clipping if it was set */
  if (clipped) { set_clip(ctx, unclipped_rect); }
}

void draw_icon(Context* ctx, int32_t id, Rect rect, Color color) {
  Command* cmd;
  /* do clip command if the rect isn't fully contained within the cliprect */
  int clipped = check_clip(ctx, rect);
  if (clipped == MU_CLIP_ALL) { return; }
  if (clipped == MU_CLIP_PART) { set_clip(ctx, get_clip_rect(ctx)); }
  /* do icon command */
  cmd = push_command(ctx, MU_COMMAND_ICON, sizeof(mu_IconCommand));
  cmd->icon.id = id;
  cmd->icon.rect = rect;
  cmd->icon.color = color;
  /* reset clipping if it was set */
  if (clipped) { set_clip(ctx, unclipped_rect); }
}

/*============================================================================
** layout
**============================================================================*/

enum { RELATIVE = 1, ABSOLUTE = 2 };

void layout_begin_column(Context* ctx) {
  push_layout(ctx, layout_next(ctx), Vec2(0, 0));
}

void layout_end_column(Context* ctx) {
  Layout* a, * b;
  b = get_layout(ctx);
  ctx->layout_stack.pop();
  /* inherit position/next_row/max from child layout if they are greater */
  a = get_layout(ctx);
  a->position.x = std::max(a->position.x, b->position.x + b->body.x - a->body.x);
  a->next_row = std::max(a->next_row, b->next_row + b->body.y - a->body.y);
  a->max.x = std::max(a->max.x, b->max.x);
  a->max.y = std::max(a->max.y, b->max.y);
}

void layout_row(Context* ctx, int items, const int* widths, int height) {
  Layout* layout = get_layout(ctx);
  if (widths) {
    assert(items <= MU_MAX_WIDTHS);
    memcpy(layout->widths, widths, items * sizeof(widths[0]));
  }
  layout->items = items;
  layout->position = Vec2(layout->indent, layout->next_row);
  layout->size.y = height;
  layout->item_index = 0;
}

void layout_width(Context* ctx, int32_t width) {
  get_layout(ctx)->size.x = width;
}

void layout_height(Context* ctx, int32_t height) {
  get_layout(ctx)->size.y = height;
}

void layout_set_next(Context* ctx, Rect r, int32_t relative) {
  Layout* layout = get_layout(ctx);
  layout->next = r;
  layout->next_type = relative ? RELATIVE : ABSOLUTE;
}

Rect layout_next(Context* ctx) {
  Layout* layout = get_layout(ctx);
  Style* style = ctx->style;
  Rect res;

  if (layout->next_type) {
    /* handle rect set by `mu_layout_set_next` */
    int32_t type = layout->next_type;
    layout->next_type = 0;
    res = layout->next;
    if (type == ABSOLUTE) { return (ctx->last_rect = res); }
  } else {
    /* handle next row */
    if (layout->item_index == layout->items) {
      layout_row(ctx, layout->items, nullptr, layout->size.y);
    }

    /* position */
    res.x = layout->position.x;
    res.y = layout->position.y;

    /* size */
    res.w = layout->items > 0 ? layout->widths[layout->item_index] : layout->size.x;
    res.h = layout->size.y;
    if (res.w == 0) { res.w = style->size.x + style->padding * 2; }
    if (res.h == 0) { res.h = style->size.y + style->padding * 2; }
    if (res.w < 0) { res.w += layout->body.w - res.x + 1; }
    if (res.h < 0) { res.h += layout->body.h - res.y + 1; }

    layout->item_index++;
  }

  /* update position */
  layout->position.x += res.w + style->spacing;
  layout->next_row = mu_max(layout->next_row, res.y + res.h + style->spacing);

  /* apply body offset */
  res.x += layout->body.x;
  res.y += layout->body.y;

  /* update max position */
  layout->max.x = mu_max(layout->max.x, res.x + res.w);
  layout->max.y = mu_max(layout->max.y, res.y + res.h);

  return (ctx->last_rect = res);
}

/*============================================================================
** controls
**============================================================================*/

static int in_hover_root(Context* ctx) {
  int i = ctx->container_stack.idx;
  while (i--) {
    if (ctx->container_stack.items[i] == ctx->hover_root) { return 1; }
    /* only root containers have their `head` field set; stop searching if we've
    ** reached the current root container */
    if (ctx->container_stack.items[i]->head) { break; }
  }
  return 0;
}

void draw_control_frame(Context* ctx, muId id, Rect rect, int colorid, int opt)
{
  if (opt & MU_OPT_NOFRAME) { return; }
  colorid += (ctx->focus == id) ? 2 : (ctx->hover == id) ? 1 : 0;
  ctx->draw_frame(ctx, rect, colorid);
}

void draw_control_text(Context* ctx, const char* str, Rect rect, int colorid, int opt)
{
  Vec2 pos;
  mu_Font font = ctx->style->font;
  int tw = ctx->text_width(font, str, -1);
  push_clip_rect(ctx, rect);
  pos.y = rect.y + (rect.h - ctx->text_height(font)) / 2;
  if (opt & MU_OPT_ALIGNCENTER) {
    pos.x = rect.x + (rect.w - tw) / 2;
  } else if (opt & MU_OPT_ALIGNRIGHT) {
    pos.x = rect.x + rect.w - tw - ctx->style->padding;
  } else {
    pos.x = rect.x + ctx->style->padding;
  }
  draw_text(ctx, font, str, -1, pos, ctx->style->colors[colorid]);
  pop_clip_rect(ctx);
}

int mouse_over(Context* ctx, Rect rect) {
  return rect.overlaps(ctx->mouse_pos) &&
    get_clip_rect(ctx).overlaps(ctx->mouse_pos) &&
    in_hover_root(ctx);
}

void update_control(Context* ctx, muId id, Rect rect, int opt) {
  int mouseover = mouse_over(ctx, rect);

  if (ctx->focus == id) { ctx->updated_focus = 1; }
  if (opt & MU_OPT_NOINTERACT) { return; }
  if (mouseover && !ctx->mouse_down) { ctx->hover = id; }

  if (ctx->focus == id) {
    if (ctx->mouse_pressed && !mouseover) { set_focus(ctx, 0); }
    if (!ctx->mouse_down && ~opt & MU_OPT_HOLDFOCUS) { set_focus(ctx, 0); }
  }

  if (ctx->hover == id) {
    if (ctx->mouse_pressed) {
      set_focus(ctx, id);
    } else if (!mouseover) {
      ctx->hover = 0;
    }
  }
}

void text(Context* ctx, const char* text) {
  const char* start, * end, * p = text;
  int width = -1;
  mu_Font font = ctx->style->font;
  Color color = ctx->style->colors[MU_COLOR_TEXT];
  layout_begin_column(ctx);
  layout_row(ctx, 1, &width, ctx->text_height(font));
  do {
    Rect r = layout_next(ctx);
    int w = 0;
    start = end = p;
    do {
      const char* word = p;
      while (*p && *p != ' ' && *p != '\n') { p++; }
      w += ctx->text_width(font, word, p - word);
      if (w > r.w && end != start) { break; }
      w += ctx->text_width(font, p, 1);
      end = p++;
    } while (*end && *end != '\n');
    draw_text(ctx, font, start, end - start, Vec2(r.x, r.y), color);
    p = end + 1;
  } while (*end);
  layout_end_column(ctx);
}

void label(Context* ctx, const char* text) {
  draw_control_text(ctx, text, layout_next(ctx), MU_COLOR_TEXT, 0);
}

int button(Context* ctx, const char* label, int icon, int opt) {
  int res = 0;
  muId id = label ? get_id(ctx, label, strlen(label))
    : get_id(ctx, &icon, sizeof(icon));
  Rect r = layout_next(ctx);
  update_control(ctx, id, r, opt);
  /* handle click */
  if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id) {
    res |= MU_RES_SUBMIT;
  }
  /* draw */
  draw_control_frame(ctx, id, r, MU_COLOR_BUTTON, opt);
  if (label) { draw_control_text(ctx, label, r, MU_COLOR_TEXT, opt); }
  if (icon) { draw_icon(ctx, icon, r, ctx->style->colors[MU_COLOR_TEXT]); }
  return res;
}

int checkbox(Context* ctx, const char* label, int* state) {
  int res = 0;
  muId id = get_id(ctx, &state, sizeof(state));
  Rect r = layout_next(ctx);
  Rect box = Rect(r.x, r.y, r.h, r.h);
  update_control(ctx, id, r, 0);
  /* handle click */
  if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id) {
    res |= MU_RES_CHANGE;
    *state = !*state;
  }
  /* draw */
  draw_control_frame(ctx, id, box, MU_COLOR_BASE, 0);
  if (*state) {
    draw_icon(ctx, MU_ICON_CHECK, box, ctx->style->colors[MU_COLOR_TEXT]);
  }
  r = Rect(r.x + box.w, r.y, r.w - box.w, r.h);
  draw_control_text(ctx, label, r, MU_COLOR_TEXT, 0);
  return res;
}

int textbox_raw(Context* ctx, char* buf, int bufsz, muId id, Rect r, int opt)
{
  int res = 0;
  update_control(ctx, id, r, opt | MU_OPT_HOLDFOCUS);

  if (ctx->focus == id) {
    /* handle text input */
    int len = strlen(buf);
    int n = mu_min(bufsz - len - 1, (int)strlen(ctx->input_text));
    if (n > 0) {
      memcpy(buf + len, ctx->input_text, n);
      len += n;
      buf[len] = '\0';
      res |= MU_RES_CHANGE;
    }
    /* handle backspace */
    if (ctx->key_pressed & MU_KEY_BACKSPACE && len > 0) {
      /* skip utf-8 continuation bytes */
      while ((buf[--len] & 0xc0) == 0x80 && len > 0);
      buf[len] = '\0';
      res |= MU_RES_CHANGE;
    }
    /* handle return */
    if (ctx->key_pressed & MU_KEY_RETURN) {
      set_focus(ctx, 0);
      res |= MU_RES_SUBMIT;
    }
  }

  /* draw */
  draw_control_frame(ctx, id, r, MU_COLOR_BASE, opt);
  if (ctx->focus == id) {
    Color color = ctx->style->colors[MU_COLOR_TEXT];
    mu_Font font = ctx->style->font;
    int textw = ctx->text_width(font, buf, -1);
    int texth = ctx->text_height(font);
    int ofx = r.w - ctx->style->padding - textw - 1;
    int textx = r.x + mu_min(ofx, ctx->style->padding);
    int texty = r.y + (r.h - texth) / 2;
    push_clip_rect(ctx, r);
    draw_text(ctx, font, buf, -1, Vec2(textx, texty), color);
    draw_rect(ctx, Rect(textx + textw, texty, 1, texth), color);
    pop_clip_rect(ctx);
  } else {
    draw_control_text(ctx, buf, r, MU_COLOR_TEXT, opt);
  }

  return res;
}

static int number_textbox(Context* ctx, muReal* value, Rect r, muId id) {
  if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->key_down & MU_KEY_SHIFT &&
    ctx->hover == id
    ) {
    ctx->number_edit = id;
    snprintf(ctx->number_edit_buf, sizeof(ctx->number_edit_buf), MU_REAL_FMT, *value);
  }
  if (ctx->number_edit == id) {
    int res = textbox_raw(
      ctx, ctx->number_edit_buf, sizeof(ctx->number_edit_buf), id, r, 0);
    if (res & MU_RES_SUBMIT || ctx->focus != id) {
      *value = strtod(ctx->number_edit_buf, NULL);
      ctx->number_edit = 0;
    }
    else {
      return 1;
    }
  }
  return 0;
}

int textbox(Context* ctx, char* buf, int bufsz, int opt) {
  muId id = get_id(ctx, &buf, sizeof(buf));
  Rect r = layout_next(ctx);
  return textbox_raw(ctx, buf, bufsz, id, r, opt);
}

int slider(Context* ctx, muReal* value, muReal low, muReal high,
  muReal step, const char* fmt, int opt)
{
  char buf[MU_MAX_FMT + 1];
  Rect thumb;
  int x, w, res = 0;
  muReal last = *value, v = last;
  muId id = get_id(ctx, &value, sizeof(value));
  Rect base = layout_next(ctx);

  /* handle text input mode */
  if (number_textbox(ctx, &v, base, id)) { return res; }

  /* handle normal mode */
  update_control(ctx, id, base, opt);

  /* handle input */
  if (ctx->focus == id &&
    (ctx->mouse_down | ctx->mouse_pressed) == MU_MOUSE_LEFT)
  {
    v = low + (ctx->mouse_pos.x - base.x) * (high - low) / base.w;
    if (step) { v = ((long long)((v + step / 2) / step)) * step; }
  }
  /* clamp and store value, update res */
  *value = v = mu_clamp(v, low, high);
  if (last != v) { res |= MU_RES_CHANGE; }

  /* draw base */
  draw_control_frame(ctx, id, base, MU_COLOR_BASE, opt);
  /* draw thumb */
  w = ctx->style->thumb_size;
  x = (v - low) * (base.w - w) / (high - low);
  thumb = Rect(base.x + x, base.y, w, base.h);
  draw_control_frame(ctx, id, thumb, MU_COLOR_BUTTON, opt);
  /* draw text  */
  snprintf(buf, sizeof(buf), fmt, v);
  draw_control_text(ctx, buf, base, MU_COLOR_TEXT, opt);

  return res;
}

int number_ex(Context* ctx, muReal* value, muReal step,
  const char* fmt, int opt)
{
  char buf[MU_MAX_FMT + 1];
  int res = 0;
  muId id = get_id(ctx, &value, sizeof(value));
  Rect base = layout_next(ctx);
  muReal last = *value;

  /* handle text input mode */
  if (number_textbox(ctx, value, base, id)) { return res; }

  /* handle normal mode */
  update_control(ctx, id, base, opt);

  /* handle input */
  if (ctx->focus == id && ctx->mouse_down == MU_MOUSE_LEFT) {
    *value += ctx->mouse_delta.x * step;
  }
  /* set flag if value changed */
  if (*value != last) { res |= MU_RES_CHANGE; }

  /* draw base */
  draw_control_frame(ctx, id, base, MU_COLOR_BASE, opt);
  /* draw text  */
  snprintf(buf, sizeof(buf), fmt, *value);
  draw_control_text(ctx, buf, base, MU_COLOR_TEXT, opt);

  return res;
}

static int header_local(Context* ctx, const char* label, int istreenode, int opt) {
  Rect r;
  int active, expanded;
  muId id = get_id(ctx, label, strlen(label));
  int idx = pool_get(ctx, ctx->treenode_pool, MU_TREENODEPOOL_SIZE, id);
  int width = -1;
  layout_row(ctx, 1, &width, 0);

  active = (idx >= 0);
  expanded = (opt & MU_OPT_EXPANDED) ? !active : active;
  r = layout_next(ctx);
  update_control(ctx, id, r, 0);

  /* handle click */
  active ^= (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id);

  /* update pool ref */
  if (idx >= 0) {
    if (active) { pool_update(ctx, ctx->treenode_pool, idx); }
    else { memset(&ctx->treenode_pool[idx], 0, sizeof(PoolItem)); }
  } else if (active) {
    pool_init(ctx, ctx->treenode_pool, MU_TREENODEPOOL_SIZE, id);
  }

  /* draw */
  if (istreenode) {
    if (ctx->hover == id) { ctx->draw_frame(ctx, r, MU_COLOR_BUTTONHOVER); }
  }
  else {
    draw_control_frame(ctx, id, r, MU_COLOR_BUTTON, 0);
  }
  draw_icon(
    ctx, expanded ? MU_ICON_EXPANDED : MU_ICON_COLLAPSED,
    Rect(r.x, r.y, r.h, r.h), ctx->style->colors[MU_COLOR_TEXT]);
  r.x += r.h - ctx->style->padding;
  r.w -= r.h - ctx->style->padding;
  draw_control_text(ctx, label, r, MU_COLOR_TEXT, 0);

  return expanded ? MU_RES_ACTIVE : 0;
}

int header(Context* ctx, const char* label, int opt) {
  return header_local(ctx, label, 0, opt);
}

int begin_treenode(Context* ctx, const char* label, int opt) {
  int res = header_local(ctx, label, 1, opt);
  if (res & MU_RES_ACTIVE) {
    get_layout(ctx)->indent += ctx->style->indent;
    ctx->id_stack.push(ctx->last_id);
  }
  return res;
}

void end_treenode(Context* ctx) {
  get_layout(ctx)->indent -= ctx->style->indent;
  pop_id(ctx);
}

#define scrollbar(ctx, cnt, b, cs, x, y, w, h)                              \
  do {                                                                      \
    /* only add scrollbar if content size is larger than body */            \
    int maxscroll = cs.y - b->h;                                            \
                                                                            \
    if (maxscroll > 0 && b->h > 0) {                                        \
      Rect base, thumb;                                                  \
      muId id = get_id(ctx, "!scrollbar" #y, 11);                       \
                                                                            \
      /* get sizing / positioning */                                        \
      base = *b;                                                            \
      base.x = b->x + b->w;                                                 \
      base.w = ctx->style->scrollbar_size;                                  \
                                                                            \
      /* handle input */                                                    \
      update_control(ctx, id, base, 0);                                  \
      if (ctx->focus == id && ctx->mouse_down == MU_MOUSE_LEFT) {           \
        cnt->scroll.y += ctx->mouse_delta.y * cs.y / base.h;                \
      }                                                                     \
      /* clamp scroll to limits */                                          \
      cnt->scroll.y = mu_clamp(cnt->scroll.y, 0, maxscroll);                \
                                                                            \
      /* draw base and thumb */                                             \
      ctx->draw_frame(ctx, base, MU_COLOR_SCROLLBASE);                      \
      thumb = base;                                                         \
      thumb.h = std::max(ctx->style->thumb_size, base.h * b->h / cs.y);       \
      thumb.y += cnt->scroll.y * (base.h - thumb.h) / maxscroll;            \
      ctx->draw_frame(ctx, thumb, MU_COLOR_SCROLLTHUMB);                    \
                                                                            \
      /* set this as the scroll_target (will get scrolled on mousewheel) */ \
      /* if the mouse is over it */                                         \
      if (mouse_over(ctx, *b)) { ctx->scroll_target = cnt; }             \
    } else {                                                                \
      cnt->scroll.y = 0;                                                    \
    }                                                                       \
  } while (0)


static void scrollbars(Context* ctx, Container* cnt, Rect* body) {
  int sz = ctx->style->scrollbar_size;
  Vec2 cs = cnt->content_size;
  cs.x += ctx->style->padding * 2;
  cs.y += ctx->style->padding * 2;
  push_clip_rect(ctx, *body);
  /* resize body to make room for scrollbars */
  if (cs.y > cnt->body.h) { body->w -= sz; }
  if (cs.x > cnt->body.w) { body->h -= sz; }
  /* to create a horizontal or vertical scrollbar almost-identical code is
  ** used; only the references to `x|y` `w|h` need to be switched */
  scrollbar(ctx, cnt, body, cs, x, y, w, h);
  scrollbar(ctx, cnt, body, cs, y, x, h, w);
  pop_clip_rect(ctx);
}


static void push_container_body(
  Context* ctx, Container* cnt, Rect body, int opt
) {
  if (~opt & MU_OPT_NOSCROLL) { scrollbars(ctx, cnt, &body); }
  push_layout(ctx, body.expand(-ctx->style->padding), cnt->scroll);
  cnt->body = body;
}

static void begin_root_container(Context* ctx, Container* cnt) {
  ctx->container_stack.push(cnt);
  /* push container to roots list and push head command */
  ctx->root_list.push(cnt);
  cnt->head = push_jump(ctx, NULL);
  /* set as hover root if the mouse is overlapping this container and it has a
  ** higher zindex than the current hover root */
  if (cnt->rect.overlaps(ctx->mouse_pos) &&
    (!ctx->next_hover_root || cnt->zindex > ctx->next_hover_root->zindex)
    ) {
    ctx->next_hover_root = cnt;
  }
  /* clipping is reset here in case a root-container is made within
  ** another root-containers's begin/end block; this prevents the inner
  ** root-container being clipped to the outer */
  ctx->clip_stack.push(unclipped_rect);
}

static void end_root_container(Context* ctx) {
  /* push tail 'goto' jump command and set head 'skip' command. the final steps
  ** on initing these are done in mu_end() */
  Container* cnt = get_current_container(ctx);
  cnt->tail = push_jump(ctx, NULL);
  cnt->head->jump.dst = ctx->command_list.items.data() + ctx->command_list.idx;
  /* pop base clip rect and container */
  pop_clip_rect(ctx);
  pop_container(ctx);
}

int begin_window(Context* ctx, const char* title, Rect rect, int opt) {
  Rect body;
  muId id = get_id(ctx, title, strlen(title));
  Container* cnt = get_container(ctx, id, opt);
  if (!cnt || !cnt->open) { return 0; }
  ctx->id_stack.push(id);

  if (cnt->rect.w == 0) { cnt->rect = rect; }
  begin_root_container(ctx, cnt);
  rect = body = cnt->rect;

  /* draw frame */
  if (~opt & MU_OPT_NOFRAME) {
    ctx->draw_frame(ctx, rect, MU_COLOR_WINDOWBG);
  }

  /* do title bar */
  if (~opt & MU_OPT_NOTITLE) {
    Rect tr = rect;
    tr.h = ctx->style->title_height;
    ctx->draw_frame(ctx, tr, MU_COLOR_TITLEBG);

    /* do title text */
    if (~opt & MU_OPT_NOTITLE) {
      muId id = get_id(ctx, "!title", 6);
      update_control(ctx, id, tr, opt);
      draw_control_text(ctx, title, tr, MU_COLOR_TITLETEXT, opt);
      if (id == ctx->focus && ctx->mouse_down == MU_MOUSE_LEFT) {
        cnt->rect.x += ctx->mouse_delta.x;
        cnt->rect.y += ctx->mouse_delta.y;
      }
      body.y += tr.h;
      body.h -= tr.h;
    }

    /* do `close` button */
    if (~opt & MU_OPT_NOCLOSE) {
      muId id = get_id(ctx, "!close", 6);
      Rect r = Rect(tr.x + tr.w - tr.h, tr.y, tr.h, tr.h);
      tr.w -= r.w;
      draw_icon(ctx, MU_ICON_CLOSE, r, ctx->style->colors[MU_COLOR_TITLETEXT]);
      update_control(ctx, id, r, opt);
      if (ctx->mouse_pressed == MU_MOUSE_LEFT && id == ctx->focus) {
        cnt->open = 0;
      }
    }
  }

  push_container_body(ctx, cnt, body, opt);

  /* do `resize` handle */
  if (~opt & MU_OPT_NORESIZE) {
    int sz = ctx->style->title_height;
    muId id = get_id(ctx, "!resize", 7);
    Rect r = Rect(rect.x + rect.w - sz, rect.y + rect.h - sz, sz, sz);
    update_control(ctx, id, r, opt);
    if (id == ctx->focus && ctx->mouse_down == MU_MOUSE_LEFT) {
      cnt->rect.w = mu_max(96, cnt->rect.w + ctx->mouse_delta.x);
      cnt->rect.h = mu_max(64, cnt->rect.h + ctx->mouse_delta.y);
    }
  }

  /* resize to content size */
  if (opt & MU_OPT_AUTOSIZE) {
    Rect r = get_layout(ctx)->body;
    cnt->rect.w = cnt->content_size.x + (cnt->rect.w - r.w);
    cnt->rect.h = cnt->content_size.y + (cnt->rect.h - r.h);
  }

  /* close if this is a popup window and elsewhere was clicked */
  if (opt & MU_OPT_POPUP && ctx->mouse_pressed && ctx->hover_root != cnt) {
    cnt->open = 0;
  }

  push_clip_rect(ctx, cnt->body);
  return MU_RES_ACTIVE;
}

void end_window(Context* ctx) {
  pop_clip_rect(ctx);
  end_root_container(ctx);
}

void open_popup(Context* ctx, const char* name) {
  Container* cnt = get_container(ctx, name);
  /* set as hover root so popup isn't closed in begin_window_ex()  */
  ctx->hover_root = ctx->next_hover_root = cnt;
  /* position at mouse cursor, open and bring-to-front */
  cnt->rect = Rect(ctx->mouse_pos.x, ctx->mouse_pos.y, 1, 1);
  cnt->open = 1;
  bring_to_front(ctx, cnt);
}

int begin_popup(Context* ctx, const char* name) {
  int opt = MU_OPT_POPUP | MU_OPT_AUTOSIZE | MU_OPT_NORESIZE |
    MU_OPT_NOSCROLL | MU_OPT_NOTITLE | MU_OPT_CLOSED;
  return begin_window(ctx, name, Rect(0, 0, 0, 0), opt);
}

void end_popup(Context* ctx) {
  end_window(ctx);
}

void begin_panel(Context* ctx, const char* name, int opt) {
  Container* cnt;
  push_id(ctx, name, strlen(name));
  cnt = get_container(ctx, ctx->last_id, opt);
  cnt->rect = layout_next(ctx);
  if (~opt & MU_OPT_NOFRAME) {
    ctx->draw_frame(ctx, cnt->rect, MU_COLOR_PANELBG);
  }
  ctx->container_stack.push(cnt);
  push_container_body(ctx, cnt, cnt->rect, opt);
  push_clip_rect(ctx, cnt->body);
}

void end_panel(Context* ctx) {
  pop_clip_rect(ctx);
  pop_container(ctx);
}

}