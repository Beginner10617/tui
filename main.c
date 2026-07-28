#define TUI_IMPLEMENTATION
#define DEBUG
#include "tui.h"
int main(){
	TerminalWindow term = createTermWindow(40, 10);
	unsigned int FPS = 60;
	FrameLimiter limiter;
	frame_limiter_init(FPS, &limiter);
	while(1){
	  fill_clr(6, &term);
    move_cursor(0, 0, &term);
    write_char(U'✓', &term);
	  display(&term);
	  frame_limiter_wait(&limiter);
	}
	return 0;
}
