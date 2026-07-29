#define TUI_IMPLEMENTATION
#include "../tui.h"
int main(){
	TerminalWindow term = createTermWindow(40, 10);
	unsigned int FPS = 60;
	FrameLimiter limiter;
	frame_limiter_init(FPS, &limiter);
	while(1){
		fill_clr(RED, &term);

		move_cursor(0, 0, &term);
		term.cursor_style_flags = 0;
		write_str("This is default", &term);

		move_cursor(1, 0, &term);
		term.cursor_style_flags = BOLD;
		write_str("This is bold", &term);

		move_cursor(2, 0, &term);
		term.cursor_style_flags = ITALIC;
		write_str("This is italic", &term);

		move_cursor(3, 0, &term);
		term.cursor_style_flags = UNDERLINE;
		write_str("This is underline", &term);

		move_cursor(4, 0, &term);
		term.cursor_style_flags = BLINK;
		write_str("This is blinking", &term);

		move_cursor(5, 0, &term);
		term.cursor_style_flags = BOLD | UNDERLINE | ITALIC;
		write_str("This is a combination", &term);

		move_cursor(6, 0, &term);
		term.cursor_style_flags = BOLD | BLINK | ITALIC;
		write_str("This is another combination", &term);

		move_cursor(7, 0, &term);
		term.cursor_style_flags = HIDDEN;
		write_str("This is hidden", &term);
		
		move_cursor(8, 0, &term);
		term.cursor_style_flags = 0;
		set_color_fg(BLUE, &term);
		write_str("This is in blue fg", &term);

		// return back to defaults
		set_color_fg(WHITE, &term);

		display(&term);
	}
	return 0;
}
