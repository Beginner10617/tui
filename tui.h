#ifndef TUI
#define TUI
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#ifdef TUI_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"

typedef struct {
  uint32_t codepoint;
  uint8_t fg;
  uint8_t bg;
  uint8_t style_flags;
} Cell;

typedef struct {
  Cell *front_buf, *back_buf;
  size_t num_of_rows, num_of_cols, cursor_row, cursor_col;
  uint8_t cursor_color_fg, cursor_color_bg;
  uint8_t cursor_style_flags;
  bool first_render;
} TerminalWindow;

enum {
  BOLD = 1 << 0,
  DIM = 1 << 1,
  ITALIC = 1 << 2,
  UNDERLINE = 1 << 3,
  BLINK = 1 << 4,
  INVERSE = 1 << 5,
  HIDDEN = 1 << 6,
  STRIKE = 1 << 7,
};

enum {
  BLACK,
  MAROON,
  GREEN,
  OLIVE,
  NAVY,
  PURPLE,
  TEAL,
  SILVER,
  GREY,
  RED,
  LIME,
  YELLOW,
  BLUE,
  FUCHSIA,
  AQUA,
  WHITE,
};

TerminalWindow createTermWindow(size_t width, size_t height);

void move_cursor(size_t row, size_t col, TerminalWindow *term);
void set_color_fg(uint8_t clr, TerminalWindow *term);
void set_color_bg(uint8_t clr, TerminalWindow *term);
void write_char(uint32_t c, TerminalWindow *term);
void write_str(const char *str, TerminalWindow *term);
void fill_clr(uint8_t clr, TerminalWindow *term);

void display(TerminalWindow *term);
void sleep_ms(long ms);

typedef struct {
  struct timespec last_frame_time;
  long frame_duration_ns; // in n-sec
} FrameLimiter;

void frame_limiter_init(unsigned int frame_rate, FrameLimiter *frame_limiter);
void frame_limiter_wait(FrameLimiter *frame_limiter);

typedef struct Rect Rect;
struct Rect {
  size_t start_row, start_col, end_row, end_col;
};
Rect create_rect(size_t start_row, size_t start_col, size_t end_row,
                 size_t end_col);
void draw_borders(Rect rect, TerminalWindow *term);

typedef struct Image Image;

Image *load_image(const char *path);
void destroy_image(Image **img);

struct Image {
  uint8_t *pixels;
  size_t width, height;
};

typedef enum {
  IMG_NEAREST,
  IMG_BILINEAR,
  IMG_BICUBIC,
  IMG_LANCZOS
} ImageFilter;

void drawImage(TerminalWindow *win, const Image *img, Rect dst,
               ImageFilter filter, bool keep_aspect);

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
#define TUI_IMPLEMENTATION
#ifdef TUI_IMPLEMENTATION
TerminalWindow createTermWindow(size_t width, size_t height) {
  TerminalWindow output;
  output.num_of_rows = height;
  output.num_of_cols = width;
  output.cursor_row = 0;
  output.cursor_col = 0;
  output.cursor_color_fg = WHITE;
  output.cursor_color_bg = BLACK;
  output.cursor_style_flags = 0;
  output.first_render = true;
  output.front_buf = malloc(sizeof(Cell) * width * height);
  output.back_buf = malloc(sizeof(Cell) * width * height);
  if (!output.front_buf || !output.back_buf) {
#ifdef DEBUG
    printf("error[createTermWindow]: unable to allocate space for front and "
           "back buffer\n");
#endif
    exit(EXIT_FAILURE);
  }
  for (size_t i = 0; i < height * width; i++) {
    // front
    output.front_buf[i].codepoint = ' ';
    output.front_buf[i].fg = WHITE;      // white
    output.front_buf[i].bg = BLACK;      // black
    output.front_buf[i].style_flags = 0; // no flags

    // back
    output.back_buf[i].codepoint = ' ';
    output.back_buf[i].fg = WHITE;      // white
    output.back_buf[i].bg = BLACK;      // black
    output.back_buf[i].style_flags = 0; // no flags
  }
  return output;
}

void move_cursor(size_t row, size_t col, TerminalWindow *term) {
  if (row < term->num_of_rows && col < term->num_of_cols) {
    term->cursor_row = row;
    term->cursor_col = col;
  } else {
#ifdef DEBUG
    printf("warning[move_cursor]: position specified is out of the window\n");
#endif
  }
}

void set_color_fg(uint8_t clr, TerminalWindow *term) {
  term->cursor_color_fg = clr;
}

void set_color_bg(uint8_t clr, TerminalWindow *term) {
  term->cursor_color_bg = clr;
}

void write_char(uint32_t c, TerminalWindow *term) {
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  term->back_buf[index].codepoint = c;
  term->back_buf[index].fg = term->cursor_color_fg;
  term->back_buf[index].bg = term->cursor_color_bg;
  term->back_buf[index].style_flags = term->cursor_style_flags;
}

void write_str(const char *str, TerminalWindow *term) {
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  // assuimng null terminated
  const char *c = str;
  while (*c) {
    if (index >= term->num_of_cols * term->num_of_rows) {
#ifdef DEBUG
      printf("warning[write_str]: string length exceeded buffer size\n");
#endif
      return;
    }
    term->back_buf[index].codepoint = *c;
    term->back_buf[index].fg = term->cursor_color_fg;
    term->back_buf[index].bg = term->cursor_color_bg;
    term->back_buf[index].style_flags = term->cursor_style_flags;
    term->cursor_col++;
    if (term->cursor_col == term->num_of_cols) {
      term->cursor_col = 0;
      term->cursor_row++;
    }
    index++;
    c++;
  }
}

void fill_clr(uint8_t clr, TerminalWindow *term) {
  for (size_t i = 0; i < term->num_of_cols * term->num_of_rows; i++) {
    term->back_buf[i].codepoint = ' ';
    term->back_buf[i].bg = clr;
  }
  term->cursor_color_bg = clr;
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

void display(TerminalWindow *term) {
  if (term->first_render) {
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
    printf("\x1b[?25l"); // Hide cursor
  }
  printf("\x1b[H"); // Move cursor to (0,0)
  char tmp_c[5];
  for (size_t i = 0; i < term->num_of_cols * term->num_of_rows; i++) {
    if (i != 0 && i % term->num_of_cols == 0)
      putchar('\n');
    Cell cell = term->back_buf[i];
    utf8_encode(cell.codepoint, tmp_c);
    // styling
    for (size_t i = 0; i < 8; i++) {
      if (cell.style_flags & (1 << i))
        printf("\x1b[%zum", i + (i < 5 ? 1 : 2));
    }
    printf("\x1b[38;5;%um\x1b[48;5;%um%s\x1b[0m", cell.fg, cell.bg, tmp_c);
  }
  fflush(stdout);

  Cell *tmp = term->front_buf;
  term->front_buf = term->back_buf;
  term->back_buf = tmp;
  printf("\x1b[H"); // Move cursor to (0,0)
}

void sleep_ms(long ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000L;

  nanosleep(&ts, NULL);
}

void frame_limiter_init(unsigned int frame_rate, FrameLimiter *frame_limiter) {
  frame_limiter->frame_duration_ns = 1000000000L / frame_rate;
  clock_gettime(CLOCK_MONOTONIC, &frame_limiter->last_frame_time);
}

static struct timespec timespec_add_ns(struct timespec t, long ns) {
  // helper function
  t.tv_sec += ns / 1000000000L;
  t.tv_nsec += ns % 1000000000L;
  if (t.tv_nsec >= 1000000000L) {
    t.tv_sec++;
    t.tv_nsec -= 1000000000L;
  }
  return t;
}
static struct timespec timespec_sub(struct timespec a, struct timespec b) {
  struct timespec r;
  r.tv_sec = a.tv_sec - b.tv_sec;
  r.tv_nsec = a.tv_nsec - b.tv_nsec;
  if (r.tv_nsec < 0) {
    r.tv_nsec += 1000000000L;
    r.tv_sec--;
  }
  return r;
}
static int timespec_cmp(struct timespec a, struct timespec b) {
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
void frame_limiter_wait(FrameLimiter *frame_limiter) {
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

    struct timespec remaining = timespec_sub(target, now);
    // To handle interrupts if any
    while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR)
      ;
  }
  frame_limiter->last_frame_time = target;
}

Rect create_rect(size_t start_row, size_t start_col, size_t end_row,
                 size_t end_col) {
  Rect out;
  out.start_row = start_row;
  out.start_col = start_col;
  out.end_row = end_row;
  out.end_col = end_col;
  return out;
}

void draw_borders(Rect rect, TerminalWindow *term) {
  for (size_t col = rect.start_col;
       col <= rect.end_col && col < term->num_of_cols; col++) {
    if (rect.start_row < term->num_of_rows) {
      move_cursor(rect.start_row, col, term);
      if (col > rect.start_col && col < rect.end_col)
        write_char(U'─', term);
      else if (col == rect.start_col)
        write_char(U'┌', term);
      else if (col == rect.end_col)
        write_char(U'┐', term);
    }
    if (rect.end_row < term->num_of_rows) {
      move_cursor(rect.end_row, col, term);
      if (col > rect.start_col && col < rect.end_col)
        write_char(U'─', term);
      else if (col == rect.start_col)
        write_char(U'└', term);
      else if (col == rect.end_col)
        write_char(U'┘', term);
    }
  }
  for (size_t row = rect.start_row + 1;
       row < rect.end_row && row < term->num_of_rows; row++) {
    if (rect.start_col < term->num_of_cols) {
      move_cursor(row, rect.start_col, term);
      write_char(U'│', term);
    }
    if (rect.end_row < term->num_of_rows) {
      move_cursor(row, rect.end_col, term);
      write_char(U'│', term);
    }
  }
}

Image *load_image(const char *path) {
  int x, y;
  unsigned char *data = stbi_load(path, &x, &y, NULL, 3);
  if (data == NULL) {
#ifdef DEBUG
    printf("error[load_image]: Unable to open file %s\n", path);
#endif
    return NULL;
  }
  Image *output = malloc(sizeof(Image));
  output->height = y;
  output->width = x;
  output->pixels = data;
  return output;
}

void destroy_image(Image **img) {
  if (img == NULL || *img == NULL)
    return;
  stbi_image_free((*img)->pixels);
  free(*img);
  *img = NULL;
}

// helper functions
Image *apply_nearest_neighbour(Image *img, int width, int height) {
  if (img == NULL) {
#ifdef DEBUG
    printf("warning[apply_nearest_neighbour]: NULL passed to "
           "apply_nearest_neighbour()\n");
#endif
    return NULL;
  }
  Image *output = malloc(sizeof(Image));
  output->width = width;
  output->height = height;
  output->pixels = malloc(sizeof(uint8_t) * width * height * 3);
  if (output->pixels == NULL) {
#ifdef DEBUG
    printf("error[apply_nearest_neighbour]: Unable to allocate memory for "
           "transformed image pixels\n");
#endif
    return NULL;
  }
  double scale_x = (double)img->width / (double)width;
  double scale_y = (double)img->height / (double)height;
  for (size_t x = 0; x < width; x++) {
    for (size_t y = 0; y < height; y++) {
      size_t nearest_x = round(scale_x * x);
      size_t nearest_y = round(scale_y * y);
      output->pixels[x + y * width + 0] =
          img->pixels[nearest_x + nearest_y * img->width + 0];
      output->pixels[x + y * width + 1] =
          img->pixels[nearest_x + nearest_y * img->width + 1];
      output->pixels[x + y * width + 2] =
          img->pixels[nearest_x + nearest_y * img->width + 2];
    }
  }
  return output;
}

uint8_t rgb_to_ansi(uint8_t r, uint8_t g, uint8_t b) {
  return 16 + 36 * (r * 5) / 255 + 6 * (g * 5) / 255 + (b * 5) / 255;
}

drawImage(TerminalWindow *win, const Image *img, Rect dst, ImageFilter filter,
          bool keep_aspect) {
  // 1. calculate width and height if keep_aspect is true (fit in dst)
  // 2. call appropriate filter function, and store the transformed image in tmp
  // 3. store the current state of the cursor
  // 4. convert pixel data to ansi code and write using the cursor
  // 5. restore the cursor to original state
  // 6. destroy the tmp image data
}

#endif
#endif
