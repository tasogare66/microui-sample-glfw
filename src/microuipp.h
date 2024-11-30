#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
namespace mui{
#define MU_COMMANDLIST_SIZE     (256 * 1024)
#define MU_ROOTLIST_SIZE        32
#define MU_CONTAINERSTACK_SIZE  32
#define MU_CLIPSTACK_SIZE       32
#define MU_IDSTACK_SIZE         32
#define MU_LAYOUTSTACK_SIZE     16
#define MU_CONTAINERPOOL_SIZE   48
#define MU_TREENODEPOOL_SIZE    48
#define MU_MAX_WIDTHS           16
#define MU_REAL                 float
#define MU_REAL_FMT             "%.3g"
#define MU_SLIDER_FMT           "%.2f"
#define MU_MAX_FMT              127

//#define mu_stack(T, n)          struct { int idx; T items[n]; }
//#define mu_min(a, b)            ((a) < (b) ? (a) : (b))
//#define mu_max(a, b)            ((a) > (b) ? (a) : (b))
//#define mu_clamp(x, a, b)       mu_min(b, mu_max(a, x))

  template<class T, size_t _Size>
  struct Stack {
    int32_t idx = 0;
    std::array<T, _Size> items = {};
    void push(const T& val) {
      assert(idx < _Size);
      items[idx] = val;
      ++idx;
    }
    void pop() {
      assert(idx > 0);
      --idx;
    }
  };

  enum {
    MU_CLIP_PART = 1,
    MU_CLIP_ALL
  };

  enum {
    MU_COMMAND_JUMP = 1,
    MU_COMMAND_CLIP,
    MU_COMMAND_RECT,
    MU_COMMAND_TEXT,
    MU_COMMAND_ICON,
    MU_COMMAND_MAX
  };

  enum {
    MU_COLOR_TEXT,
    MU_COLOR_BORDER,
    MU_COLOR_WINDOWBG,
    MU_COLOR_TITLEBG,
    MU_COLOR_TITLETEXT,
    MU_COLOR_PANELBG,
    MU_COLOR_BUTTON,
    MU_COLOR_BUTTONHOVER,
    MU_COLOR_BUTTONFOCUS,
    MU_COLOR_BASE,
    MU_COLOR_BASEHOVER,
    MU_COLOR_BASEFOCUS,
    MU_COLOR_SCROLLBASE,
    MU_COLOR_SCROLLTHUMB,
    MU_COLOR_MAX
  };

  enum {
    MU_ICON_CLOSE = 1,
    MU_ICON_CHECK,
    MU_ICON_COLLAPSED,
    MU_ICON_EXPANDED,
    MU_ICON_MAX
  };

  enum {
    MU_RES_ACTIVE = (1 << 0),
    MU_RES_SUBMIT = (1 << 1),
    MU_RES_CHANGE = (1 << 2)
  };

  enum {
    MU_OPT_ALIGNCENTER = (1 << 0),
    MU_OPT_ALIGNRIGHT = (1 << 1),
    MU_OPT_NOINTERACT = (1 << 2),
    MU_OPT_NOFRAME = (1 << 3),
    MU_OPT_NORESIZE = (1 << 4),
    MU_OPT_NOSCROLL = (1 << 5),
    MU_OPT_NOCLOSE = (1 << 6),
    MU_OPT_NOTITLE = (1 << 7),
    MU_OPT_HOLDFOCUS = (1 << 8),
    MU_OPT_AUTOSIZE = (1 << 9),
    MU_OPT_POPUP = (1 << 10),
    MU_OPT_CLOSED = (1 << 11),
    MU_OPT_EXPANDED = (1 << 12)
  };

  enum {
    MU_MOUSE_LEFT = (1 << 0),
    MU_MOUSE_RIGHT = (1 << 1),
    MU_MOUSE_MIDDLE = (1 << 2)
  };

  enum {
    MU_KEY_SHIFT = (1 << 0),
    MU_KEY_CTRL = (1 << 1),
    MU_KEY_ALT = (1 << 2),
    MU_KEY_BACKSPACE = (1 << 3),
    MU_KEY_RETURN = (1 << 4)
  };

  using muId = uint32_t;
  using muReal = float;
  using muFont = void*;

  struct Vec2 {
    constexpr Vec2(int32_t ix, int32_t iy) : x(ix), y(iy) {}
    constexpr Vec2() : Vec2(0, 0) {}
    int32_t x, y;
  };
  struct Rect {
    constexpr Rect(int32_t ix, int32_t iy, int32_t iw, int32_t ih) : x(ix), y(iy), w(iw), h(ih) {}
    constexpr Rect() : Rect(0, 0, 0, 0) {}
    Rect expand(int32_t n) const { return { x - n, y - n, w + n * 2, h + n * 2 }; }
    Rect insert(Rect r2) const { //重複部分
      int32_t x1 = std::max(x, r2.x);
      int32_t y1 = std::max(y, r2.y);
      int32_t x2 = std::min(x + w, r2.x + r2.w);
      int32_t y2 = std::min(y + h, r2.y + r2.h);
      return Rect(x1, y1, std::max(x2 - x1, 0), std::max(y2 - y1, 0));
    }
    bool overlaps(Vec2 p) const { return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h; }
    int32_t x, y, w, h;
  };
  struct Color {
    constexpr Color(uint8_t ir, uint8_t ig, uint8_t ib, uint8_t ia) : r(ir), g(ig), b(ib), a(ia) {}
    constexpr Color() : Color(0, 0, 0, 0) {}
    uint8_t r, g, b, a;
  };
  struct PoolItem { muId id; int last_update; };

  struct BaseCommand { int type, size; };
  struct JumpCommand { BaseCommand base; void* dst; };
  struct ClipCommand { BaseCommand base; Rect rect; };
  struct RectCommand { BaseCommand base; Rect rect; Color color; };
  struct TextCommand { BaseCommand base; muFont font; Vec2 pos; Color color; char str[1]; };
  struct IconCommand { BaseCommand base; Rect rect; int id; Color color; };

  union Command {
    int type;
    BaseCommand base;
    JumpCommand jump;
    ClipCommand clip;
    RectCommand rect;
    TextCommand text;
    IconCommand icon;
  };

  struct Layout {
    Rect body;
    Rect next;
    Vec2 position;
    Vec2 size;
    Vec2 max;
    int32_t widths[MU_MAX_WIDTHS];
    int32_t items;
    int32_t item_index;
    int32_t next_row;
    int32_t next_type;
    int32_t indent;
  };

  struct Container {
    Command* head, * tail;
    Rect rect;
    Rect body;
    Vec2 content_size;
    Vec2 scroll;
    int32_t zindex;
    int32_t open;
  };

  struct Style {
    muFont font;
    Vec2 size;
    int padding;
    int spacing;
    int indent;
    int title_height;
    int scrollbar_size;
    int thumb_size;
    Color colors[MU_COLOR_MAX];
  };

  struct Context {
    /* callbacks */
    int (*text_width)(muFont font, const char* str, int len);
    int (*text_height)(muFont font);
    void (*draw_frame)(Context* ctx, Rect rect, int colorid);
    /* core state */
    Style _style;
    Style* style;
    muId hover;
    muId focus;
    muId last_id;
    Rect last_rect;
    int last_zindex;
    int updated_focus;
    int frame;
    Container* hover_root;
    Container* next_hover_root;
    Container* scroll_target;
    char number_edit_buf[MU_MAX_FMT];
    muId number_edit;
    /* stacks */
    Stack<char, MU_COMMANDLIST_SIZE> command_list;
    Stack<Container*, MU_ROOTLIST_SIZE> root_list;
    Stack<Container*, MU_CONTAINERSTACK_SIZE> container_stack;
    Stack<Rect, MU_CLIPSTACK_SIZE> clip_stack;
    Stack<muId, MU_IDSTACK_SIZE> id_stack;
    Stack<Layout, MU_LAYOUTSTACK_SIZE> layout_stack;
    /* retained state pools */
    PoolItem container_pool[MU_CONTAINERPOOL_SIZE];
    Container containers[MU_CONTAINERPOOL_SIZE];
    PoolItem treenode_pool[MU_TREENODEPOOL_SIZE];
    /* input state */
    Vec2 mouse_pos;
    Vec2 last_mouse_pos;
    Vec2 mouse_delta;
    Vec2 scroll_delta;
    int mouse_down;
    int mouse_pressed;
    int key_down;
    int key_pressed;
    char input_text[32];
  };

  void init(Context* ctx);
  void begin(Context* ctx);
  void end(Context* ctx);
  void set_focus(Context* ctx, muId id);
  muId get_id(Context* ctx, const void* data, size_t size);
  void push_id(Context* ctx, const void* data, size_t size);
  void pop_id(Context* ctx);
  void push_clip_rect(Context* ctx, Rect rect);
  void pop_clip_rect(Context* ctx);
  Rect get_clip_rect(Context* ctx);
  int32_t check_clip(Context* ctx, Rect r);
  Container* get_current_container(Context* ctx);
  Container* get_container(Context* ctx, const char* name);
  void bring_to_front(Context* ctx, Container* cnt);

  int32_t pool_init(Context* ctx, PoolItem* items, int32_t len, muId id);
  int32_t pool_get(Context* ctx, PoolItem* items, int32_t len, muId id);
  void pool_update(Context* ctx, PoolItem* items, int32_t idx);

  void input_mousemove(Context* ctx, int32_t x, int32_t y);
  void input_mousedown(Context* ctx, int32_t x, int32_t y, int32_t btn);
  void input_mouseup(Context* ctx, int32_t x, int32_t y, int32_t btn);
  void input_scroll(Context* ctx, int32_t x, int32_t y);
  void input_keydown(Context* ctx, int32_t key);
  void input_keyup(Context* ctx, int32_t key);
  void input_text(Context* ctx, const char* text);

  Command* push_command(Context* ctx, int32_t type, int32_t size);
  int32_t next_command(Context* ctx, Command** cmd);
  void set_clip(Context* ctx, Rect rect);
  void draw_rect(Context* ctx, Rect rect, Color color);
  void draw_box(Context* ctx, Rect rect, Color color);
  void draw_text(Context* ctx, muFont font, const char* str, int len, Vec2 pos, Color color);
  void draw_icon(Context* ctx, int32_t id, Rect rect, Color color);

  void layout_row(Context* ctx, int items, const int* widths, int height);
  void layout_width(Context* ctx, int32_t width);
  void layout_height(Context* ctx, int32_t height);
  void layout_begin_column(Context* ctx);
  void layout_end_column(Context* ctx);
  void layout_set_next(Context* ctx, Rect r, int32_t relative);
  Rect layout_next(Context* ctx);

  void draw_control_frame(Context* ctx, muId id, Rect rect, int colorid, int opt);
  void draw_control_text(Context* ctx, const char* str, Rect rect, int colorid, int opt);
  int mouse_over(Context* ctx, Rect rect);
  void update_control(Context* ctx, muId id, Rect rect, int opt);

  void text(Context* ctx, const char* text);
  void label(Context* ctx, const char* text);
  int button(Context* ctx, const char* label, int icon = 0, int opt = MU_OPT_ALIGNCENTER);
  int checkbox(Context* ctx, const char* label, int* state);
  int textbox_raw(Context* ctx, char* buf, int bufsz, muId id, Rect r, int opt);
  int textbox(Context* ctx, char* buf, int bufsz, int opt = 0);
  int slider(Context* ctx, muReal* value, muReal low, muReal high, muReal step = 0, const char* fmt = MU_SLIDER_FMT, int opt = MU_OPT_ALIGNCENTER);
  int number(Context* ctx, muReal* value, muReal step, const char* fmt = MU_SLIDER_FMT, int opt = MU_OPT_ALIGNCENTER);
  int header(Context* ctx, const char* label, int opt = 0);
  int begin_treenode(Context* ctx, const char* label, int opt = 0);
  void end_treenode(Context* ctx);
  int begin_window(Context* ctx, const char* title, Rect rect, int opt = 0);
  void end_window(Context* ctx);
  void open_popup(Context* ctx, const char* name);
  int begin_popup(Context* ctx, const char* name);
  void end_popup(Context* ctx);
  void begin_panel(Context* ctx, const char* name, int opt = 0);
  void end_panel(Context* ctx);
}