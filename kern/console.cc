#include "console.h"

namespace kernel {

static char copykVGABuffer[Terminal::kVGAWidth][Terminal::kVGAHeight];

void Console::print(char ch) {
	if(x >= Terminal::kVGAWidth) {
		x = 0;
		y += 1;
	}
	if(y >= Terminal::kVGAHeight) {
		//copy the screen and move up one line...
		for(int i = 0; i < Terminal::kVGAHeight; i++ ) {
			for (int j = 0; j <= Terminal::kVGAWidth; j++) {
				copykVGABuffer[i][j] = copykVGABuffer[i][j + 1];
			}

		}
		for (int i = 0; i < Terminal::kVGAWidth; i++) {
			copykVGABuffer[Terminal::kVGAHeight][i] = 0;
		}

		for (int i = 0; i < Terminal::kVGAWidth; i++) {
			for (int j = 0; j < Terminal::kVGAHeight; j++) {
				default_term->DrawChar(copykVGABuffer[i][j], i, j);
			}

		}

		//end of move the screen one line up
		y = Terminal::kVGAHeight - 1;
	}

	default_term->DrawChar(ch, x, y);
	copykVGABuffer[x][y] = ch;
	x += 1;
}
}
