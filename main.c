#define TUI_IMPLEMENTATION
#define DEBUG
#include "tui.h"
int main(){
	TerminalWindow term = createTermWindow(40, 10);
	while(1){
	  display(&term);
	}
	return 0;
}
