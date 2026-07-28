#include <stdbool.h>
#define TUI_IMPLEMENTATION
#include "../tui.h"
int main(){
	TerminalWindow term = createTermWindow(40, 10);
	unsigned int FPS = 60;
	FrameLimiter limiter;
	frame_limiter_init(FPS, &limiter);
	int x_pos = 0, y_pos = 0;
	bool x_inc = true, y_inc = true;
	double dt = 0;
	while(1){
		fill_clr(4, &term);
		move_cursor(y_pos, x_pos, &term);
		set_bg_color(3, &term);
		write_char(' ',&term);
		display(&term);
		dt += 1.0 / FPS;
		if(dt > 1.0){
			dt = 0;
			if (x_pos == 39 && x_inc) x_inc = false;
			if (x_pos == 0 && !x_inc) x_inc = true;
			if (y_pos == 9 && y_inc) y_inc = false;
			if (y_pos == 0 && !y_inc) y_inc = true;
			x_pos += x_inc ? 1 : -1;
			y_pos += y_inc ? 1 : -1;
		}
		frame_limiter_wait(&limiter);
	}
	return 0;
}
