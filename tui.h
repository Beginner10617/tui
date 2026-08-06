#ifndef TUI
#define TUI
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#ifdef TUI_IMPLEMENTATION
#define _USE_MATH_DEFINES
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"
#include <math.h>

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

static struct termios og_config;

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
void show_cursor();
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

void draw_image(Image *img, TerminalWindow *term, Rect dst, ImageFilter filter,
                bool keep_aspect);

// Keystates like sdl
enum {
  TUIK_SPACE,
  TUIK_ENTER,
  TUIK_TAB,
  TUIK_BACK,
  TUIK_ESCAPE,
  TUIK_CONTROL,

  TUIK_UP,
  TUIK_DOWN,
  TUIK_LEFT,
  TUIK_RIGHT,

  TUIK_HOME,
  TUIK_END,
  TUIK_INSERT,
  TUIK_DEL,
  TUIK_PG_UP,
  TUIK_PG_DN,

  TUIK_F1,
  TUIK_F2,
  TUIK_F3,
  TUIK_F4,
  TUIK_F5,
  TUIK_F6,
  TUIK_F7,
  TUIK_F8,
  TUIK_F9,
  TUIK_F10,
  TUIK_F11,
  TUIK_F12,

  TUIK_CHAR,

  TUIK_COUNT,
};
typedef struct {
  bool pressed[TUIK_COUNT];
  char c_data;
} InputState;

void disable_raw_mode();
void enable_raw_mode();
void tui_poll_events(InputState *input);

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
    dprintf(STDOUT_FILENO, "\x1b[?25l"); // Hide cursor
  }
  dprintf(STDOUT_FILENO, "\x1b[H"); // Move cursor to (0,0)
  char tmp_c[5];
  for (size_t i = 0; i < term->num_of_cols * term->num_of_rows; i++) {
    if (i != 0 && i % term->num_of_cols == 0)
      dprintf(STDOUT_FILENO, "\r\n");
    Cell cell = term->back_buf[i];
    utf8_encode(cell.codepoint, tmp_c);
    // styling
    for (size_t i = 0; i < 8; i++) {
      if (cell.style_flags & (1 << i))
        dprintf(STDOUT_FILENO, "\x1b[%zum", i + (i < 5 ? 1 : 2));
    }
    dprintf(STDOUT_FILENO, "\x1b[38;5;%um\x1b[48;5;%um%s\x1b[0m", cell.fg, cell.bg, tmp_c);
  }

  Cell *tmp = term->front_buf;
  term->front_buf = term->back_buf;
  term->back_buf = tmp;
  fflush(stdout);
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
    free(output);
    return NULL;
  }
  double scale_x = (double)img->width / (double)width;
  double scale_y = (double)img->height / (double)height;
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      size_t nearest_x = round(scale_x * x);
      size_t nearest_y = round(scale_y * y);
      nearest_x = nearest_x >= img->width ? img->width - 1 : nearest_x;
      nearest_y = nearest_y >= img->height ? img->height - 1 : nearest_y;
      output->pixels[(x + y * width) * 3 + 0] =
          img->pixels[(nearest_x + nearest_y * img->width) * 3 + 0];
      output->pixels[(x + y * width) * 3 + 1] =
          img->pixels[(nearest_x + nearest_y * img->width) * 3 + 1];
      output->pixels[(x + y * width) * 3 + 2] =
          img->pixels[(nearest_x + nearest_y * img->width) * 3 + 2];
    }
  }
  return output;
}

Image *apply_bilinear(Image *img, int width, int height) {
  if (img == NULL) {
#ifdef DEBUG
    printf("warning[apply_bilinear]: NULL passed to "
           "apply_bilinear()\n");
#endif
    return NULL;
  }
  Image *output = malloc(sizeof(Image));
  output->width = width;
  output->height = height;
  output->pixels = malloc(sizeof(uint8_t) * width * height * 3);
  if (output->pixels == NULL) {
#ifdef DEBUG
    printf("error[apply_bilinear]: Unable to allocate memory for "
           "transformed image pixels\n");
#endif
    free(output);
    return NULL;
  }
  double scale_x = (double)(img->width - 1) / (double)(width - 1);
  double scale_y = (double)(img->height - 1) / (double)(height - 1);
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      size_t x0 = floor(scale_x * x);
      size_t y0 = floor(scale_y * y);
      size_t x1 = x0 + 1;
      size_t y1 = y0 + 1;
      if (x1 >= img->width) x1 = x0;
      if (y1 >= img->height) y1 = y0;
      double tx = scale_x * x - (double) x0;
      double ty = scale_y * y - (double) y0;
      output->pixels[(x + y * width) * 3 + 0] = 
	      (1-tx) * (1-ty) * img->pixels[(x0 + y0 * img->width) * 3 + 0]
	    + ( tx ) * (1-ty) * img->pixels[(x1 + y0 * img->width) * 3 + 0]
	    + (1-tx) * ( ty ) * img->pixels[(x0 + y1 * img->width) * 3 + 0]
	    + ( tx ) * ( ty ) * img->pixels[(x1 + y1 * img->width) * 3 + 0];

      output->pixels[(x + y * width) * 3 + 1] = 
	      (1-tx) * (1-ty) * img->pixels[(x0 + y0 * img->width) * 3 + 1]
	    + ( tx ) * (1-ty) * img->pixels[(x1 + y0 * img->width) * 3 + 1]
	    + (1-tx) * ( ty ) * img->pixels[(x0 + y1 * img->width) * 3 + 1]
	    + ( tx ) * ( ty ) * img->pixels[(x1 + y1 * img->width) * 3 + 1];

      output->pixels[(x + y * width) * 3 + 2] = 
	      (1-tx) * (1-ty) * img->pixels[(x0 + y0 * img->width) * 3 + 2]
	    + ( tx ) * (1-ty) * img->pixels[(x1 + y0 * img->width) * 3 + 2]
	    + (1-tx) * ( ty ) * img->pixels[(x0 + y1 * img->width) * 3 + 2]
	    + ( tx ) * ( ty ) * img->pixels[(x1 + y1 * img->width) * 3 + 2];
    }
  }
  return output;
}

double catmull_rom(double x){
  double abs_x = fabs(x);
  double x_2 = x * x;
  double abs_x_3 = x_2 * abs_x;

  if (abs_x <= 1){ 
    return 1.5 * abs_x_3 - 2.5 * x_2 + 1;
  }
  else if (abs_x < 2 && 1 < abs_x){
    return -0.5 * abs_x_3 + 2.5 * x_2 - 4 * abs_x + 2;
  }
  return 0;
}

double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

Image *apply_bicubic(Image *img, int width, int height){
  if (img == NULL) {
#ifdef DEBUG
    printf("warning[apply_bicubic]: NULL passed to "
           "apply_bicubic()\n");
#endif
    return NULL;
  }
  Image *output = malloc(sizeof(Image));
  output->width = width;
  output->height = height;
  output->pixels = malloc(sizeof(uint8_t) * width * height * 3);
  if (output->pixels == NULL) {
#ifdef DEBUG
    printf("error[apply_bicubic]: Unable to allocate memory for "
           "transformed image pixels\n");
#endif
    free(output);
    return NULL;
  }
  double scale_x = (double)(img->width - 1) / (double)(width - 1);
  double scale_y = (double)(img->height - 1) / (double)(height - 1);
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      size_t _x[4], _y[4];
      _x[1] = floor(scale_x * x);
      _y[1] = floor(scale_y * y);
      double u = scale_x * x - (double) _x[1];
      double v = scale_y * y - (double) _y[1];
      _x[2] = _x[1] + 1 < img->width ? _x[1] + 1 : _x[1];
      _y[2] = _y[1] + 1 < img->height ? _y[1] + 1 : _y[1];
      _x[0] = _x[1] > 0 ? _x[1] - 1 : _x[1];
      _y[0] = _y[1] > 0 ? _y[1] - 1 : _y[1];
      _x[3] = _x[1] + 2 < img->width ? _x[1] + 2 : _x[1];
      _y[3] = _y[1] + 2 < img->height ? _y[1] + 2 : _y[1];
      double R[4], G[4], B[4];
      for(size_t i = 0; i < 4; i++){
        R[i] = 0; G[i] = 0; B[i] = 0;
        for(int m = -1; m <= 2; m++){
          R[i] += catmull_rom(m - u) * img->pixels[(_x[m + 1] + _y[i] * img->width) * 3 + 0];
          G[i] += catmull_rom(m - u) * img->pixels[(_x[m + 1] + _y[i] * img->width) * 3 + 1];
          B[i] += catmull_rom(m - u) * img->pixels[(_x[m + 1] + _y[i] * img->width) * 3 + 2];
        }
      }
      double r = 0, g = 0, b = 0;
      for(int k = -1; k <= 2; k++){
        r += R[k + 1] * catmull_rom(k - v);
        g += G[k + 1] * catmull_rom(k - v);
        b += B[k + 1] * catmull_rom(k - v);
      }
      r = clamp(r, 0, 255.0);
      g = clamp(g, 0, 255.0);
      b = clamp(b, 0, 255.0);
      output->pixels[(x + y * width) * 3 + 0] = (uint8_t)lround(r);
      output->pixels[(x + y * width) * 3 + 1] = (uint8_t)lround(g);
      output->pixels[(x + y * width) * 3 + 2] = (uint8_t)lround(b);
    }
  }
  return output;
}

/*
 Lanczos kernel with fixed a = 3
 */
int lanczos_a = 3;

double _lanczos(double x) {
  if (x == 0.0)
    return 1.0;
  return lanczos_a * sin(M_PI * x) * sin(M_PI * x / lanczos_a) /
         (M_PI * M_PI * x * x);
}

Image *apply_lanczos(Image *img, int width, int height) {
  if (img == NULL) {
#ifdef DEBUG
    printf("warning[apply_lanczos]: NULL passed to "
           "apply_lanczos()\n");
#endif
    return NULL;
  }
  Image *output = malloc(sizeof(Image));
  output->width = width;
  output->height = height;
  output->pixels = malloc(sizeof(uint8_t) * width * height * 3);
  if (output->pixels == NULL) {
#ifdef DEBUG
    printf("error[apply_lanczos]: Unable to allocate memory for "
           "transformed image pixels\n");
#endif
    free(output);
    return NULL;
  }
  double scale_x = (double)(img->width - 1) / (double)(width - 1);
  double scale_y = (double)(img->height - 1) / (double)(height - 1);
  double s_x = (scale_x > 1.0 ? scale_x : 1.0);
  double s_y = (scale_y > 1.0 ? scale_y : 1.0);
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      int ix = floor(scale_x * x);
      int iy = floor(scale_y * y);
      double r = 0, g = 0, b = 0, weight_sum = 0;
      int rad_x = ceil(lanczos_a * s_x);
      int rad_y = ceil(lanczos_a * s_y);
      for (int i = -rad_x + 1; i <= rad_x; i++) {
        for (int j = -rad_y + 1; j <= rad_y; j++) {
          int src_x = ix + i;
          int src_y = iy + j;
          int look_x = clamp((double)src_x, 0, (double)img->width - 1);
          int look_y = clamp((double)src_y, 0, (double)img->height - 1);
          double wx = _lanczos(((double)src_x - scale_x * x) / s_x);
          double wy = _lanczos(((double)src_y - scale_y * y) / s_y);
          double weight = wx * wy;
          weight_sum += weight;
          r += weight * img->pixels[(look_x + look_y * img->width) * 3 + 0];
          g += weight * img->pixels[(look_x + look_y * img->width) * 3 + 1];
          b += weight * img->pixels[(look_x + look_y * img->width) * 3 + 2];
        }
      }
      r = r / weight_sum;
      g = g / weight_sum;
      b = b / weight_sum;
      r = clamp(r, 0, 255.0);
      g = clamp(g, 0, 255.0);
      b = clamp(b, 0, 255.0);
      output->pixels[(x + y * width) * 3 + 0] = (uint8_t)lround(r);
      output->pixels[(x + y * width) * 3 + 1] = (uint8_t)lround(g);
      output->pixels[(x + y * width) * 3 + 2] = (uint8_t)lround(b);
    }
  }
  return output;
}
uint8_t rgb_to_ansi(uint8_t r, uint8_t g, uint8_t b) {
  return 16 + 36 * (r * 5) / 255 + 6 * (g * 5) / 255 + (b * 5) / 255;
}

void draw_image(Image *img, TerminalWindow *term, Rect dst, ImageFilter filter,
                bool keep_aspect){
  size_t width = 1 + dst.end_col - dst.start_col,
         height = (1 + dst.end_row - dst.start_row), pixel_h = 2 * height;
  double scale_x = (double)img->width / (double)width;
  double scale_y = (double)img->height / (double)pixel_h;
  if (keep_aspect) {
    double scale = fmax(scale_x, scale_y);
    scale_x = scale;
    scale_y = scale;
  }
  size_t out_w = round((double)img->width / scale_x);
  size_t out_h = round((double)img->height / scale_y);

  Image *tmp = NULL;
  switch (filter) {
  case IMG_NEAREST:
    tmp = apply_nearest_neighbour(img, out_w, out_h);
    break;
  case IMG_BILINEAR:
    tmp = apply_bilinear(img, out_w, out_h);
    break;
  case IMG_BICUBIC:
    tmp = apply_bicubic(img, out_w, out_h);
    break;
  case IMG_LANCZOS:
    tmp = apply_lanczos(img, out_w, out_h);
    break;
  default:
#ifdef DEBUG
    printf("error[draw_image]: Invalid filter argument\n");
#endif
    return;
  }

  for (size_t row = dst.start_row;
       row < dst.start_row + out_h / 2 && row < term->num_of_rows; row++) {
    for (size_t col = dst.start_col;
         col < dst.start_col + out_w && col < term->num_of_cols; col++) {

      move_cursor(row, col, term);
      size_t x = col - dst.start_col, y = row - dst.start_row;
      size_t index1 = 3 * (x + 2 * y * out_w);
      size_t index2 = 3 * (x + (2 * y + 1) * out_w);

      uint8_t r1 = tmp->pixels[index1 + 0], r2 = tmp->pixels[index2 + 0];
      uint8_t g1 = tmp->pixels[index1 + 1], g2 = tmp->pixels[index2 + 1];
      uint8_t b1 = tmp->pixels[index1 + 2], b2 = tmp->pixels[index2 + 2];
      set_color_fg(rgb_to_ansi(r1, g1, b1), term);
      set_color_bg(rgb_to_ansi(r2, g2, b2), term);
      write_char(U'▀', term);
    }
  }
  destroy_image(&tmp);
}

void disable_raw_mode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &og_config);
}
void enable_raw_mode(){
  tcgetattr(STDIN_FILENO, &og_config);
  atexit(disable_raw_mode);
  struct termios raw = og_config;
  raw.c_lflag &= ~ (ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void tui_poll_events(InputState *input){
  for (int i = 0; i < TUIK_COUNT; i++) input->pressed[i] = false;
  char tmp;
  ssize_t status = read(STDIN_FILENO, &tmp, 1);
  if(status != 1) return;
  if (tmp > ' ' && tmp <= '~'){
    input->c_data = tmp;
    input->pressed[TUIK_CHAR] = true;
  } else {
    if (tmp == 0x20) input->pressed[TUIK_SPACE] = true;
    else if (tmp == 0x0A) input->pressed[TUIK_ENTER] = true;
    else if (tmp == 0x09) input->pressed[TUIK_TAB] = true;
    else if (tmp == 0x7F) input->pressed[TUIK_BACK] = true;

    for(int i = 0x01; i <= 0x1A; i++){
      if (tmp == i) {
        input->pressed[TUIK_CONTROL] = true;
	input->pressed[TUIK_CHAR] = true;
	input->c_data = i + 'A' - 1;
      }
    }

    if (tmp == 0x1B){
      status = read(STDIN_FILENO, &tmp, 1);
      if (status != 1) input->pressed[TUIK_ESCAPE] = true;
      else if (tmp == 'O'){
        status = read(STDIN_FILENO, &tmp, 1);
	if (tmp == 'H'&&status==1) input->pressed[TUIK_HOME] = true;
	else if (tmp == 'F'&&status==1) input->pressed[TUIK_END] = true;
	else if (tmp == 'P'&&status==1) input->pressed[TUIK_F1] = true;
	else if (tmp == 'Q'&&status==1) input->pressed[TUIK_F2] = true;
	else if (tmp == 'R'&&status==1) input->pressed[TUIK_F3] = true;
	else if (tmp == 'S'&&status==1) input->pressed[TUIK_F4] = true;
      }
      else if (tmp == '['){
        status = read(STDIN_FILENO, &tmp, 1);
	if (tmp == 'A'&&status==1) input->pressed[TUIK_UP] = true;
	else if (tmp == 'B'&&status==1) input->pressed[TUIK_DOWN] = true;
	else if (tmp == 'C'&&status==1) input->pressed[TUIK_RIGHT]= true;
	else if (tmp == 'D'&&status==1) input->pressed[TUIK_LEFT] = true;
	else if (tmp == 'H'&&status==1) input->pressed[TUIK_HOME] = true;
	else if (tmp == 'F'&&status==1) input->pressed[TUIK_END]  = true;

	else if (tmp == '1'&&status==1){
	  status = read(STDIN_FILENO, &tmp, 1);
	  if (tmp == '5'&&status==1){
	    status = read(STDIN_FILENO, &tmp, 1);
	    input->pressed[TUIK_F5] = (tmp=='~'&&status==1);
	  } else if (tmp == '7'&&status==1){
	    read(STDIN_FILENO, &tmp, 1);
	    input->pressed[TUIK_F6] = (tmp=='~'&&status==1);
	  } else if (tmp == '8'&&status==1){
	    read(STDIN_FILENO, &tmp, 1);
	    input->pressed[TUIK_F7] = (tmp=='~'&&status==1);
	  } else if (tmp == '9'&&status==1){
	    read(STDIN_FILENO, &tmp, 1);
	    input->pressed[TUIK_F8] = (tmp=='~'&&status==1);
	  }
	} else if (tmp == '2'&&status==1){
	    status = read(STDIN_FILENO, &tmp, 1);
	    if (tmp == '~'&&status==1){
	      input->pressed[TUIK_INSERT] = true;
	    } else if (tmp=='0'&&status==1){
	      status = read(STDIN_FILENO, &tmp, 1);
	      input->pressed[TUIK_F9] = (tmp=='~'&&status==1);
	    } else if (tmp=='1'&&status==1){
	      status = read(STDIN_FILENO, &tmp, 1);
	      input->pressed[TUIK_F10] = (tmp=='~'&&status==1);
	    } else if (tmp=='3'&&status==1){
	      status = read(STDIN_FILENO, &tmp, 1);
	      input->pressed[TUIK_F11] = (tmp=='~'&&status==1);
	    } else if (tmp=='4'&&status==1){
	      status = read(STDIN_FILENO, &tmp, 1);
	      input->pressed[TUIK_F12] = (tmp=='~'&&status==1);
	    }
	}
	else if (tmp == '3'&&status==1){
	  status = read(STDIN_FILENO, &tmp, 1);
	  input->pressed[TUIK_DEL] = (tmp=='~'&&status==1);
	} else if (tmp == '5'&&status==1){
	  status = read(STDIN_FILENO, &tmp, 1);
	  input->pressed[TUIK_PG_UP] = (tmp=='~'&&status==1);
	} else if (tmp == '6'&&status==1){
	  status = read(STDIN_FILENO, &tmp, 1);
	  input->pressed[TUIK_PG_DN] = (tmp=='~'&&status==1);
	}
      }
    }
  }
}

char* debug_print_key(int key_code){
  switch (key_code){
    case TUIK_SPACE:
      return ("TUIK_SPACE");
      break;  //
    case TUIK_ENTER:
      return ("TUIK_ENTER");
      break;  //
    case TUIK_TAB:
      return ("TUIK_TAB");
      break;    //
    case TUIK_BACK:
      return ("TUIK_BACK");
      break;   //
    case TUIK_ESCAPE:
      return ("TUIK_ESCAPE");
      break; //
    case TUIK_CONTROL:
      return ("TUIK_CONTROL");
      break;//
    case TUIK_UP:
      return ("TUIK_UP");
      break;     //
    case TUIK_DOWN:
      return ("TUIK_DOWN");
      break;   //
    case TUIK_LEFT:
      return ("TUIK_LEFT");
      break;   //
    case TUIK_RIGHT:
      return ("TUIK_RIGHT");
      break;  //
    case TUIK_HOME:
      return ("TUIK_HOME");
      break;   //
    case TUIK_END:
      return ("TUIK_END");
      break;    // 
    case TUIK_INSERT:
      return ("TUIK_INSERT");
      break;
    case TUIK_DEL:
      return ("TUIK_DEL");
      break;
    case TUIK_PG_UP:
      return ("TUIK_PG_UP");
      break;
    case TUIK_PG_DN:
      return ("TUIK_PG_DN");
      break;
    case TUIK_F1:
      return ("TUIK_F1");
      break;     //
    case TUIK_F2:
      return ("TUIK_F2");
      break;     //
    case TUIK_F3:
      return ("TUIK_F3");
      break;     //
    case TUIK_F4:
      return ("TUIK_F4");
      break;     //
    case TUIK_F5:
      return ("TUIK_F5");
      break;
    case TUIK_F6:
      return ("TUIK_F6");
      break;
    case TUIK_F7:
      return ("TUIK_F7");
      break;
    case TUIK_F8:
      return ("TUIK_F8");
      break;
    case TUIK_F9:
      return ("TUIK_F9");
      break;
    case TUIK_F10:
      return ("TUIK_F10");
      break;
    case TUIK_F11:
      return ("TUIK_F11");
      break;
    case TUIK_F12:
      return ("TUIK_F12");
      break;
    case TUIK_CHAR:
      return ("TUIK_CHAR");
      break;  //
    default:
      return ("Unknown keycode");
  }
}

void show_cursor(){
  dprintf(STDOUT_FILENO, "\x1b[?25h");
}
#endif
#endif
