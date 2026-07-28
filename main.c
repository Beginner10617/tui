#define TUI_IMPLEMENTATION
#define DEBUG
#include "tui.h"
int main(){
	TerminalWindow term = createTermWindow(40, 10);
	unsigned int FPS = 60;
	FrameLimiter limiter;
	frame_limiter_init(FPS, &limiter);
	while(1){
	  fill_clr(4, &term);
    move_cursor(0, 0, &term);
    write_str("Hello World!", &term);
    move_cursor(50,0, &term);
	  display(&term);
	  frame_limiter_wait(&limiter);
	}
	return 0;
}
