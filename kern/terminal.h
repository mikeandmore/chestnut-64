// -*- c++ -*-

#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"

namespace kernel {

class Terminal
{
public:
	enum Color {
		COLOR_BLACK = 0,
		COLOR_BLUE = 1,
		COLOR_GREEN = 2,
		COLOR_CYAN = 3,
		COLOR_RED = 4,
		COLOR_MAGENTA = 5,
		COLOR_BROWN = 6,
		COLOR_LIGHT_GREY = 7,
		COLOR_DARK_GREY = 8,
		COLOR_LIGHT_BLUE = 9,
		COLOR_LIGHT_GREEN = 10,
		COLOR_LIGHT_CYAN = 11,
		COLOR_LIGHT_RED = 12,
		COLOR_LIGHT_MAGENTA = 13,
		COLOR_LIGHT_BROWN = 14,
		COLOR_WHITE = 15,
	};

	static const int kVGAWidth = 80;
	static const int kVGAHeight = 25;

	static void DrawChar(char ch, Color fg, Color bg, uint8 x, uint8 y);
	static void DrawString(const char *str, Color fg, Color bg, uint8 x, uint8 y);
	static void Reset();

	Terminal() : x(0), y(0), bg_(COLOR_BLACK), fg_(COLOR_WHITE) {}

	void DrawChar(char ch, uint8 x, uint8 y);
	void DrawChar(char ch);
	void DrawString(const char *str, uint8 x, uint8 y);
	void DrawString(const char *str);

	Color bg() const { return bg_; }
	Color fg() const { return fg_; }

	uint8 x, y; // positions

	void set_bg(Color bg) { bg_ = bg; }
	void set_fg(Color fg) { fg_ = fg; }

private:
	static volatile u16 *const kVGABuffer;

	Color bg_, fg_;
};

extern Terminal *default_term;

}

#endif /* TERMINAL_H */
