#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define TUI_IMPLEMENTATION
#include "tui/tui.h"
#define SCREEN_SIZE 15

enum { MODE_PLAY, MODE_QUIT };
short int grid[9][9];
short int soln[9][9];

void debug_display() {
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++)
      printf("%d ", grid[i][j]);
    printf("\n");
  }
}

void init() {
  for (int i = 0; i < 81; i++)
    grid[i / 9][i % 9] = 0;
}

short int possible(int r, int c) {
  assert(grid[r][c] == 0);
  short int out = 0;
  int boxr = (r / 3) * 3, boxc = (c / 3) * 3;
  for (int i = 0; i < 9; i++) {
    out |= (1 << grid[r][i]) | (1 << grid[i][c]) |
           (1 << grid[boxr + i / 3][boxc + i % 3]);
  }
  return out;
}

short int *possible_set(short int flags) {
  short int arr[10];
  short int count = 0;
  for (int i = 1; i < 10; i++) {
    if (!((1 << i) & flags))
      arr[count++] = i;
  }
  short int *out = calloc(count + 1, sizeof(short int));
  assert(out);
  if (!count) {
    return out;
  }
  memcpy(&out[1], arr, count * sizeof(short int));
  out[0] = count;
  return out;
}

short int *random_possible_set(short int flags) {
  short int arr[10];
  short int count = 0;
  for (int i = 1; i < 10; i++) {
    if (!((1 << i) & flags))
      arr[count++] = i;
  }
  short int *out = calloc(count + 1, sizeof(short int));
  assert(out);
  if (!count) {
    return out;
  }
  for (int i = 0; i < count; i++) {
    while (1) {
      int id = rand() % count;
      if (out[id + 1])
        continue;
      out[id + 1] = arr[i];
      break;
    }
  }
  out[0] = count;
  return out;
}

short int solve() {
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 9; c++) {
      if (grid[r][c] == 0) {
        short int *set = possible_set(possible(r, c));
        short int count = set[0];
        for (int i = 0; i < count; i++) {
          grid[r][c] = set[i + 1];
          assert(grid[r][c] > 0 && grid[r][c] < 10);
          if (solve()) {
            free(set);
            return 1;
          }
        }
        free(set);
        grid[r][c] = 0;
        return 0;
      }
    }
  }
  return 1;
}

short int random_solve() {
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 9; c++) {
      if (grid[r][c] == 0) {
        short int *set = random_possible_set(possible(r, c));
        short int count = set[0];
        for (int i = 0; i < count; i++) {
          grid[r][c] = set[i + 1];
          assert(grid[r][c] > 0 && grid[r][c] < 10);
          if (random_solve()) {
            free(set);
            return 1;
          }
        }
        free(set);
        grid[r][c] = 0;
        return 0;
      }
    }
  }
  return 1;
}

void one_soln(int *ct) {
  if (*ct > 1)
    return;
  for (int r = 0; r < 9; r++) {
    for (int c = 0; c < 9; c++) {
      if (grid[r][c] == 0) {
        short int *set = possible_set(possible(r, c));
        short int count = set[0];
        for (int i = 0; i < count; i++) {
          grid[r][c] = set[i + 1];
          one_soln(ct);
          if (*ct > 1)
            break;
        }
        free(set);
        grid[r][c] = 0;
        return;
      }
    }
  }
  (*ct)++;
}

int random_remove() {
  int r, c;
  while (1) {
    r = rand() % 9;
    c = rand() % 9;
    if (grid[r][c])
      break;
  }
  int tmp = grid[r][c];
  grid[r][c] = 0;
  int ct = 0;
  one_soln(&ct);
  return ct == 1;
  grid[r][c] = tmp;
  return 0;
}

int main(int argc, char **argv) {
  srand((unsigned int)time(NULL));
  init();
  random_solve();
  memcpy(soln, grid, sizeof(grid));
  int rem = 81;
  if (argc > 1)
    rem = atoi(argv[1]);
  while (random_remove() && rem)
    rem--;

  TerminalWindow term = createTermWindow(10 * 3 + 2, SCREEN_SIZE);
  unsigned int FPS = 30;
  FrameLimiter limiter;
  frame_limiter_init(FPS, &limiter);
  bool running = true;
  int mode = MODE_PLAY;
  int cur_r = 0, cur_c = 0, mistakes = 0;
  InputState input;
  enable_raw_mode();
  while (running) {
    // input handling
    tui_poll_events(&input);
    if (mode == MODE_PLAY && input.pressed[TUIK_CHAR] &&
        (input.c_data == 'Q' || input.c_data == 'q'))
      mode = MODE_QUIT;

    if (mode == MODE_PLAY && input.pressed[TUIK_UP] && cur_r)
      cur_r--;
    if (mode == MODE_PLAY && input.pressed[TUIK_LEFT] && cur_c)
      cur_c--;
    if (mode == MODE_PLAY && input.pressed[TUIK_DOWN] && cur_r + 1 < 9)
      cur_r++;
    if (mode == MODE_PLAY && input.pressed[TUIK_RIGHT] && cur_c + 1 < 9)
      cur_c++;
    if (mode == MODE_PLAY && !grid[cur_r][cur_c] && input.pressed[TUIK_CHAR] &&
        '0' < input.c_data && input.c_data <= '9') {
      int x = (input.c_data) - '0';
      if (soln[cur_r][cur_c] == x)
        grid[cur_r][cur_c] = x;
      else
        mistakes++;
    }

    if (mode == MODE_QUIT && input.pressed[TUIK_ENTER]) {
      running = false;
      continue;
    }
    if (mode == MODE_QUIT && input.pressed[TUIK_ESCAPE])
      mode = MODE_PLAY;
    //
    // rendering
    fill_clr(BLACK, &term);
    move_cursor(0, 13, &term);
    write_str("SUDOKU", &term);
    Rect r = {1, 1, 11, 10 * 3};
    draw_borders(r, &term);
    for (int i = 0; i < 9; i++) {
      for (int j = 0; j < 9; j++) {
        if (mode == MODE_PLAY && cur_r == i && cur_c == j)
          set_color_bg(GREY, &term);

        move_cursor(i + 2, j * 3 + 3, &term);
        if (grid[i][j])
          write_char(grid[i][j] + '0', &term);
        else
          write_char(' ', &term);
        if (mode == MODE_PLAY && cur_r == i && cur_c == j)
          set_color_bg(BLACK, &term);
      }
    }

    move_cursor(SCREEN_SIZE - 3, 1, &term);
    char str[25];
    snprintf(str, sizeof(str), "mistakes: %d", mistakes);
    write_str(str, &term);
    set_color_fg(RED, &term);
    move_cursor(SCREEN_SIZE - 2, 1, &term);

    if (mode == MODE_PLAY) {
      write_str("Press Q to quit", &term);
    } else if (mode == MODE_QUIT) {
      write_str("Press ENTER to Quit", &term);
      move_cursor(SCREEN_SIZE - 1, 1, &term);
      set_color_fg(GREEN, &term);
      write_str("Press ESC to go back", &term);
    }
    set_color_fg(WHITE, &term);
    display(&term);

    // frame limiting
    frame_limiter_wait(&limiter);
  }
  show_cursor();
  return 0;
}
