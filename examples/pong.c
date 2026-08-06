#define TUI_IMPLEMENTATION
#include "../tui.h"
#include <time.h>

enum {
  GAME_LOST_1,
  GAME_LOST_2,
  GAME_RUN,
};

int main() {
  srand((unsigned int)time(NULL));
  int game_state = GAME_RUN;

  size_t WIDTH = 80, HEIGHT = 30;
  TerminalWindow term = createTermWindow(WIDTH, HEIGHT);
  Rect rect = create_rect(1, 0, HEIGHT - 2, WIDTH - 1);
  unsigned int FPS = 20;
  FrameLimiter limiter;
  frame_limiter_init(FPS, &limiter);
  size_t BAT = 4;
  size_t player_1 = WIDTH / 2 - BAT / 2;
  size_t player_2 = WIDTH / 2 - BAT / 2;
  InputState input;
  enable_raw_mode();

  size_t ball_x = WIDTH / 2;
  size_t ball_y = HEIGHT / 2;
  bool right = rand() % 2;
  bool down = rand() % 2;
  double wait_time = 0.25, time = 0;
  bool opt_restart = true;
  while (1) {
    // input handling
    tui_poll_events(&input);
    if (game_state == GAME_RUN) {
      if (input.pressed[TUIK_LEFT])
        player_2--;
      if (input.pressed[TUIK_RIGHT])
        player_2++;

      if (player_2 < 3)
        player_2 = 3;
      if (player_2 >= WIDTH - 2)
        player_2 = WIDTH - 3;

      if (input.pressed[TUIK_CHAR] &&
          (input.c_data == 'a' || input.c_data == 'A'))
        player_1--;
      if (input.pressed[TUIK_CHAR] &&
          (input.c_data == 'd' || input.c_data == 'D'))
        player_1++;

      if (player_1 < 3)
        player_1 = 3;
      if (player_1 >= WIDTH - 2)
        player_1 = WIDTH - 3;

      // update
      if (right && ball_x == WIDTH - 2)
        right = false;
      else if (!right && ball_x == 1)
        right = true;

      if (down) {
        if (ball_y == HEIGHT - 2)
          game_state = GAME_LOST_2;
        if (ball_y == HEIGHT - 4 && ball_x >= player_2 - BAT / 2 &&
            ball_x < player_2 + BAT / 2)
          down = false;
      } else if (!down) {
        if (ball_y == 1)
          game_state = GAME_LOST_1;
        if (ball_y == 3 && ball_x >= player_1 - BAT / 2 &&
            ball_x < player_1 + BAT / 2)
          down = true;
      }
      if (wait_time < time) {
        time = 0.0;
        ball_x += right ? 1 : -1;
        ball_y += down ? 1 : -1;
      }
    } else {
      if (input.pressed[TUIK_UP] && !opt_restart)
        opt_restart = true;
      if (input.pressed[TUIK_DOWN] && opt_restart)
        opt_restart = false;
      if (input.pressed[TUIK_ENTER]) {
        if (opt_restart) {
          ball_y = HEIGHT / 2;
          ball_x = WIDTH / 2;
          right = rand() % 2;
          down = rand() % 2;
          game_state = GAME_RUN;
        } else
          break;
      }
    }

    // rendering
    fill_clr(GREY, &term);
    set_color_fg(WHITE, &term);
    move_cursor(0, WIDTH / 2 - 4, &term);
    write_str("PLAYER 1", &term);
    move_cursor(HEIGHT - 1, WIDTH / 2 - 4, &term);
    write_str("PLAYER 2", &term);
    draw_borders(rect, &term);
    move_cursor(ball_y, ball_x, &term);
    set_color_bg(RED, &term);
    write_char(' ', &term);

    set_color_bg(WHITE, &term);
    move_cursor(2, player_1 - BAT / 2, &term);
    write_str("    ", &term);
    move_cursor(HEIGHT - 3, player_2 - BAT / 2, &term);
    write_str("    ", &term);

    if (game_state != GAME_RUN) {
      set_color_bg(BLACK, &term);
      move_cursor(HEIGHT / 2 - 3, WIDTH / 2 - 7, &term);
      set_color_fg(RED, &term);
      write_str("GAME OVER    ", &term);

      set_color_fg(WHITE, &term);
      move_cursor(HEIGHT / 2 - 2, WIDTH / 2 - 7, &term);
      if (game_state == GAME_LOST_1)
        write_str("PLAYER 1 LOST", &term);
      if (game_state == GAME_LOST_2)
        write_str("PLAYER 2 LOST", &term);

      move_cursor(HEIGHT / 2 - 1, WIDTH / 2 - 7, &term);
      if (opt_restart)
        set_color_bg(AQUA, &term);
      write_str("RESTART      ", &term);

      set_color_bg(BLACK, &term);
      move_cursor(HEIGHT / 2, WIDTH / 2 - 7, &term);
      if (!opt_restart)
        set_color_bg(AQUA, &term);
      write_str("QUIT         ", &term);
    }

    display(&term);

    // frame limiting
    frame_limiter_wait(&limiter);
    time += 1.0 / (double)FPS;
  }
  show_cursor();
  return 0;
}
