#ifndef TUI
#define TUI
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

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
  CELL_REVERSE = 1 << 3,
  CELL_BLINK = 1 << 4,
  CELL_DIM = 1 << 5,
};

TerminalWindow createTermWindow(size_t width, size_t height);
void move_cursor(size_t row, size_t col, TerminalWindow *term);
void set_fg_color(uint8_t clr, TerminalWindow *term);
void set_bg_color(uint8_t clr, TerminalWindow *term);
void write_char(uint8_t c, TerminalWindow *term);
void write_str(const char *str, TerminalWindow *term);
void fill_clr(uint8_t clr, TerminalWindow *term);
void display(TerminalWindow *term);

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
  term->cursor_row = row;
  term->cursor_col = col;
}

void set_fg_color(uint8_t clr, TerminalWindow *term){
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  term->back_buf[index].fg = clr;
}

void set_bg_color(uint8_t clr, TerminalWindow *term){
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  term->back_buf[index].bg = clr;
}

void write_char(uint8_t c, TerminalWindow *term){
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  term->back_buf[index].codepoint = c;
}

void write_str(const char *str, TerminalWindow *term){
  size_t index = term->num_of_cols * term->cursor_row + term->cursor_col;
  // assuimng null terminated
  char c = *str;
  while(c){
    if(index >= term->num_of_cols * term->num_of_rows){
    #ifdef DEBUG
      printf("warning[write_str]: string length exceeded buffer size\n");
    #endif
      return;
    }
  }
}

void fill_clr(uint8_t clr, TerminalWindow *term){
  for(size_t i = 0; i < term->num_of_cols * term->num_of_rows; i++){
    term->back_buf[i].codepoint = ' ';
    term->back_buf[i].bg = clr;
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
  for(size_t i = 0; i < term->num_of_cols * term->num_of_rows; i++){
    if (i != 0 && i % term->num_of_cols == 0) putchar('\n');
    Cell cell = term->back_buf[i];
    printf("\x1b[38;5;%um\x1b[48;5;%um%c\x1b[0m", 
	cell.fg, cell.bg, cell.codepoint);
  }
  fflush(stdout);

  Cell *tmp = term->front_buf;
  term->front_buf = term->back_buf;
  term->back_buf = tmp;
}
#endif
#endif
