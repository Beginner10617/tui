#include <stdbool.h>
#define TUI_IMPLEMENTATION
#include "tui.h"
int main(){
	TerminalWindow term = createTermWindow(40, 10);
	unsigned int FPS = 60;
	FrameLimiter limiter;
	frame_limiter_init(FPS, &limiter);
	while(1){
		fill_clr(RED, &term);
		move_cursor(0,0, &term);
		set_color_fg(BLUE, &term);
		write_str("Hello ", &term);
		write_str("World!", &term);
		display(&term);
		frame_limiter_wait(&limiter);
	}
	return 0;
}
