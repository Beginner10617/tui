#define TUI_IMPLEMENTATION
#include "../tui.h"

int main(){
	TerminalWindow term = createTermWindow(40, 10);
	unsigned int FPS = 60;
	Rect rect = create_rect(0, 0, 9, 39);
	Image *img = load_image("image.png");
	if(img == NULL){
	  printf("Unable to load image image.png\n");
	  return 1;
	}
	FrameLimiter limiter;
	frame_limiter_init(FPS, &limiter);
	while(1){
		draw_image(img, &term, rect, IMG_NEAREST, true);
		display(&term);
	}
	return 0;
}
