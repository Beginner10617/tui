#include <stdbool.h>
#define TUI_IMPLEMENTATION
#include "tui.h"
int main() {
  TerminalWindow term = createTermWindow(115, 30);
  Rect rect = create_rect(0, 0, 29, 114);
  unsigned int FPS = 60;
  FrameLimiter limiter;
  frame_limiter_init(FPS, &limiter);
  Image *img = load_image("santa.png");
  while (1) {
    draw_image(&term, img, rect, IMG_NEAREST, true);
    display(&term);
    frame_limiter_wait(&limiter);
  }
  return 0;
}
