#include <stdbool.h>
#define TUI_IMPLEMENTATION
#include "tui.h"
int main() {
  TerminalWindow term = createTermWindow(40, 10);
  Rect rect = create_rect(0, 0, 9, 39);
  unsigned int FPS = 60;
  FrameLimiter limiter;
  frame_limiter_init(FPS, &limiter);
  while (1) {
    fill_clr(RED, &term);
    draw_borders(rect, &term);
    display(&term);
    frame_limiter_wait(&limiter);
  }
  return 0;
}
