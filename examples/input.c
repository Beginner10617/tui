#include <stdbool.h>
#define TUI_IMPLEMENTATION
#define DEBUG
#include "../tui.h"
int main() {

  TerminalWindow term = createTermWindow(115, 30);
  unsigned int FPS = 10;
  FrameLimiter limiter;
  frame_limiter_init(FPS, &limiter);
  InputState input;
  enable_raw_mode();
  while (1) {
    tui_poll_events(&input);
    for(int i=0; i<TUIK_COUNT; i++){
      if(input.pressed[i]){
	printf("%s", debug_print_key(i));
	if (i == TUIK_CHAR){
          printf(" char = %c", input.c_data);
	}
	printf("\n");
      }
    }
    frame_limiter_wait(&limiter);
  }
  disable_raw_mode();
  return 0;
}
