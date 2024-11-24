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
  //mu_draw_rect(ctx, rect, ctx->style->colors[colorid]);
  //if (colorid == MU_COLOR_SCROLLBASE ||
  //  colorid == MU_COLOR_SCROLLTHUMB ||
  //  colorid == MU_COLOR_TITLEBG) {
  //  return;
  //}
  ///* draw border */
  //if (ctx->style->colors[MU_COLOR_BORDER].a) {
  //  mu_draw_box(ctx, rect.expand(1), ctx->style->colors[MU_COLOR_BORDER]);
  //}
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
#if 0
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
      mu_Command* cmd = (mu_Command*)ctx->command_list.items;
      cmd->jump.dst = (char*)cnt->head + sizeof(mu_JumpCommand);
    } else {
      Container* prev = ctx->root_list.items[i - 1];
      prev->tail->jump.dst = (char*)cnt->head + sizeof(mu_JumpCommand);
    }
    /* make the last container's tail jump to the end of command list */
    if (i == n - 1) {
      cnt->tail->jump.dst = ctx->command_list.items + ctx->command_list.idx;
    }
  }
#endif
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

muId get_id(Context* ctx, const void* data, int size) {
  const auto idx = ctx->id_stack.idx;
  muId res = (idx > 0) ? ctx->id_stack.items[idx - 1] : HASH_INITIAL;
  hash(&res, data, size);
  ctx->last_id = res;
  return res;
}



}