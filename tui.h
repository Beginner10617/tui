#ifndef TUI
#define TUI
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

typedef struct {
  uint32_t codepoint;
  uint8_t fg;
  uint8_t bg;
  uint8_t flags;
} Cell;

typedef struct {
  Cell *front_buf, *back_buf;
  size_t num_of_rows, num_of_cols, cursor_row, cursor_col;
  bool first_render;
} TerminalWindow;

enum {
  CELL_BOLD = 1 << 0,
  CELL_UNDERLINE = 1 << 1,
  CELL_ITALIC = 1 << 2,
  CELL_STRIKE = 1 << 3,
  CELL_BLINK = 1 << 4,
  CELL_DIM = 1 << 5,
};

TerminalWindow createTermWindow(size_t width, size_t height);
void move_cursor(size_t row, size_t col, TerminalWindow *term);
void set_fg_color(uint8_t clr, TerminalWindow *term);
void set_bg_color(uint8_t clr, TerminalWindow *term);
void write_char(uint32_t c, TerminalWindow *term);
void write_str(const char *str, TerminalWindow *term);
void fill_clr(uint8_t clr, TerminalWindow *term);
void display(TerminalWindow *term);
void sleep_ms(long ms);

typedef struct{
  struct timespec last_frame_time;
  long frame_duration_ns; // in n-sec
} FrameLimiter;

void frame_limiter_init(unsigned int frame_rate, FrameLimiter *frame_limiter);
void frame_limiter_wait(FrameLimiter *frame_limiter);

typedef struct Rect Rect;
struct Rect {
  size_t starte_row, start_col, end_row, end_col;
  Rect *children;
};
Rect draw_rect(size_t start_row, size_t start_col, size_t end_row, size_t end_col);
void split_vert(Rect *parent, Rect *left, Rect *right, size_t cut);
void split_horz(Rect *parent, Rect *top, Rect *bottom, size_t cut);
/*
Horizontal : ─
Vertical   : │

Corners:
┌ ┐
└ ┘

T-junctions:
├ ┤
┬ ┴

Cross:
┼
*/

// Keystates like sdl
enum {
  TUIK_COUNT = 1,
};
typedef struct {
    bool down[TUIK_COUNT];
    bool pressed[TUIK_COUNT];
    bool released[TUIK_COUNT];

    uint32_t text[32];
    size_t text_len;
} InputState;

void tui_poll_events(InputState *input);
#ifdef TUI_IMPLEMENTATION
TerminalWindow createTermWindow(size_t width, size_t height){
  TerminalWindow output;
  output.num_of_rows = height;
  output.num_of_cols = width;
  output.cursor_row = 0;
  output.cursor_col = 0;
  output.first_render = true;
  output.front_buf = malloc( sizeof(Cell) * width * height );
  output.back_buf = malloc( sizeof(Cell) * width * height );
  if(!output.front_buf || !output.back_buf){
    #ifdef DEBUG
    printf("error[createTermWindow]: unable to allocate space for front and back buffer\n");
    #endif
    exit(EXIT_FAILURE);
  }
  for(size_t i = 0; i < height * width; i++){
    // front
    output.front_buf[i].codepoint = ' ';
    output.front_buf[i].fg = 7; // white
    output.front_buf[i].bg = 0; // black
    output.front_buf[i].flags = 0; // no flags
			    
    // back			    
    output.back_buf[i].codepoint = ' ';
    output.back_buf[i].fg = 7; // white
    output.back_buf[i].bg = 0; // black
    output.back_buf[i].flags = 0; // no flags
  }
  return output;
}

void move_cursor(size_t row, size_t col, TerminalWindow *term){
  if (row < term->num_of_rows && col < term->num_of_cols){
    term->cursor_row = row;
    term->cursor_col = col;
  }
  else {
    #ifdef DEBUG
      printf("warning[move_cursor]: position specified is out of the window\n");
    #endif
  }
}

void set_fg_color(uint8_t clr, TerminalWindow *term){
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  term->back_buf[index].fg = clr;
}

void set_bg_color(uint8_t clr, TerminalWindow *term){
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  term->back_buf[index].bg = clr;
}

void write_char(uint32_t c, TerminalWindow *term){
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  term->back_buf[index].codepoint = c;
}

void write_str(const char *str, TerminalWindow *term){
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  // assuimng null terminated
  const char *c = str;
  while(*c){
    if(index >= term->num_of_cols * term->num_of_rows){
    #ifdef DEBUG
      printf("warning[write_str]: string length exceeded buffer size\n");
    #endif
      return;
    }
    term->back_buf[index].codepoint = *c;
    term->cursor_col++;
    if(term->cursor_col == term->num_of_cols){
      term->cursor_col = 0;
      term->cursor_row++;
    }
    index++; c++;
  }
}

void fill_clr(uint8_t clr, TerminalWindow *term){
  for(size_t i = 0; i < term->num_of_cols * term->num_of_rows; i++){
    term->back_buf[i].codepoint = ' ';
    term->back_buf[i].bg = clr;
  }
}

// for utf-8 charset, translates it into null terminated, and returns length
static int utf8_encode(uint32_t cp, char out[5]) {
  if (cp <= 0x7F) {
      out[0] = cp;
      out[1] = 0;
      return 1;
  } else if (cp <= 0x7FF) {
      out[0] = 0xC0 | (cp >> 6);
      out[1] = 0x80 | (cp & 0x3F);
      out[2] = 0;
      return 2;
  } else if (cp <= 0xFFFF) {
      out[0] = 0xE0 | (cp >> 12);
      out[1] = 0x80 | ((cp >> 6) & 0x3F);
      out[2] = 0x80 | (cp & 0x3F);
      out[3] = 0;
      return 3;
  } else {
      out[0] = 0xF0 | (cp >> 18);
      out[1] = 0x80 | ((cp >> 12) & 0x3F);
      out[2] = 0x80 | ((cp >> 6) & 0x3F);
      out[3] = 0x80 | (cp & 0x3F);
      out[4] = 0;
      return 4;
  }
}

void display(TerminalWindow *term){
  if(term->first_render){
    term->first_render = false;
    
    struct winsize ws;
    ws.ws_row = term->num_of_rows;
    ws.ws_col = term->num_of_cols;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;
    ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws);

    if (ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws) == -1) {
      perror("ioctl");
    }
    printf("\x1b[?25l");   // Hide cursor
  }
  printf("\x1b[H");      // Move cursor to (0,0)
  char tmp_c[5];
  for(size_t i = 0; i < term->num_of_cols * term->num_of_rows; i++){
    if (i != 0 && i % term->num_of_cols == 0) putchar('\n');
    Cell cell = term->back_buf[i];
    utf8_encode(cell.codepoint, tmp_c);
    printf("\x1b[38;5;%um\x1b[48;5;%um%s\x1b[0m", 
	cell.fg, cell.bg, tmp_c);
  }
  fflush(stdout);

  Cell *tmp = term->front_buf;
  term->front_buf = term->back_buf;
  term->back_buf = tmp;
  printf("\x1b[H");      // Move cursor to (0,0)
}

void sleep_ms(long ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;

  nanosleep(&ts, NULL);
}

void frame_limiter_init(unsigned int frame_rate, FrameLimiter *frame_limiter){
  frame_limiter->frame_duration_ns = 1000000000L / frame_rate;
  clock_gettime(CLOCK_MONOTONIC, &frame_limiter->last_frame_time);
}

static struct timespec timespec_add_ns(struct timespec t, long ns){
  // helper function
  t.tv_sec += ns / 1000000000L;
  t.tv_nsec += ns % 1000000000L;
  if (t.tv_nsec >= 1000000000L) {
    t.tv_sec++;
    t.tv_nsec -= 1000000000L;
  }
  return t;
}
static struct timespec timespec_sub(struct timespec a, struct timespec b){
  struct timespec r;
  r.tv_sec  = a.tv_sec  - b.tv_sec;
  r.tv_nsec = a.tv_nsec - b.tv_nsec;
  if (r.tv_nsec < 0) {
      r.tv_nsec += 1000000000L;
      r.tv_sec--;
  }
  return r;
}
static int timespec_cmp(struct timespec a, struct timespec b){
  if (a.tv_sec < b.tv_sec)
      return -1;
  if (a.tv_sec > b.tv_sec)
      return 1;
  if (a.tv_nsec < b.tv_nsec)
      return -1;
  if (a.tv_nsec > b.tv_nsec)
      return 1;
  return 0;
}
void frame_limiter_wait(FrameLimiter *frame_limiter){
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  struct timespec target = timespec_add_ns(frame_limiter->last_frame_time,
                           frame_limiter->frame_duration_ns);

  if (timespec_cmp(now, target) >= 0) {
      frame_limiter->last_frame_time = now;
      return;
  }

  for (;;) {
      clock_gettime(CLOCK_MONOTONIC, &now);

      if (timespec_cmp(now, target) >= 0)
          break;

      struct timespec remaining =
          timespec_sub(target, now);
      // To handle interrupts if any
      while (nanosleep(&remaining, &remaining) == -1 &&
             errno == EINTR);
  }
  frame_limiter->last_frame_time = target;
}

#endif
#endif
