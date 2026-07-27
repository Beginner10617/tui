#define TUI_IMPLEMENTATION
#define DEBUG
#include "tui.h"
int main(){
	TerminalWindow term = createTermWindow(40, 10);
	unsigned int FPS = 60;
	double frame_duration = 1 / FPS;
	while(1){
	  fill_clr(8, &term);
	  display(&term);
	}
	return 0;
}
