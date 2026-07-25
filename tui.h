#ifndef TUI
#define TUI
#include <cstdint>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t codepoint;
  uint8_t fg;
  uint8_t bg;
  uint8_t flags;
} Cell;

typedef struct {
  Cell *front_buf, *back_buf;
  size_t num_of_rows, num_of_cols, cursor_row, cursor_col;
} TerminalWindow;

enum {
  CELL_BOLD = 1 << 0,
  CELL_UNDERLINE = 1 << 1,
  CELL_ITALIC = 1 << 2,
  CELL_REVERSE = 1 << 3,
  CELL_BLINK = 1 << 4,
  CELL_DIM = 1 << 5,
};

TerminalWindow createTermWindow(const char *title, int width, int height);
void move_cursor(int row, int col, TerminalWindow *term);
void set_fg_color(uint8_t clr);
void set_bg_color(uint8_t clr);
void write_char(uint8_t c, TerminalWindow *term);
void write_str(const char *c, TerminalWindow *term);
void clear(TerminalWindow *term);
void display(TerminalWindow term);
#endif
