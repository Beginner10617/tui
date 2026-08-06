#define TUI_IMPLEMENTATION
#include "../tui.h"

int main(){
	TerminalWindow term = createTermWindow(40, 10);
	unsigned int FPS = 60;
	Rect rect = create_rect(0, 0, 9, 39);
	FrameLimiter limiter;
	frame_limiter_init(FPS, &limiter);
	while(1){
		fill_clr(RED, &term);
		draw_borders(rect, &term);
		display(&term);
	}
	return 0;
}
