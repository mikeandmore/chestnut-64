#include "console.h"
#include "libc/stdarg.h"
#include "libc/string.h"

namespace kernel {

static char copykVGABuffer[Terminal::kVGAHeight][Terminal::kVGAWidth];

Console::Console()
	: x(0), y(0)
{
	 for (int i = 0; i < Terminal::kVGAHeight; i++) {
	  	for (int j = 0; j < Terminal::kVGAWidth; j++) {
	  		copykVGABuffer[i][j] = 0;
	  	}
	 }
}

void Console::putchar(char ch)
{
	if (x >= Terminal::kVGAWidth) {
		x = 0;
		y += 1;
	}
	if (y >= Terminal::kVGAHeight) {
		//copy the screen and move up one line...
		for(int i = 0; i < Terminal::kVGAHeight - 1; i++ ) {
			for (int j = 0; j < Terminal::kVGAWidth; j++) {
				copykVGABuffer[i][j] = copykVGABuffer[i + 1][j];
			}
		}
		for (int j = 0; j < Terminal::kVGAWidth; j++) {
			copykVGABuffer[Terminal::kVGAHeight - 1][j] = 0;
		}

		for (int i = 0; i < Terminal::kVGAHeight; i++) {
			for (int j = 0; j < Terminal::kVGAWidth; j++) {
				default_term->DrawChar(copykVGABuffer[i][j], j, i);
			}
		}
		//end of move the screen one line up
		y = Terminal::kVGAHeight - 1;
	}
	if (ch == '\n') {
		x = 0;
		y++;
		return;
	}
	default_term->DrawChar(ch, x, y);
	copykVGABuffer[y][x] = ch;
	x += 1;
}

void Console::printf(const char *fmt, ...)
{
	va_list ap;
	const char *hex = "0123456789abcdef";
	char buf[32], *s;
	unsigned long long u;
	int c, l;

	va_start(ap, fmt);
	while ((c = *fmt++) != '\0') {
		if (c != '%') {
			putchar(c);
			continue;
		}
		l = 0;
nextfmt:
		c = *fmt++;
		switch (c) {
		case 'l':
			l++;
			goto nextfmt;
		case 'c':
			putchar(va_arg(ap, int));
			break;
		case 's':
			for (s = va_arg(ap, char *); *s != '\0'; s++)
				putchar(*s);
			break;
		case 'd':	/* A lie, always prints unsigned */
		case 'u':
		case 'x':
			switch (l) {
			case 2:
				u = va_arg(ap, unsigned long long);
				break;
			case 1:
				u = va_arg(ap, unsigned long);
				break;
			default:
				u = va_arg(ap, unsigned int);
				break;
			}
			s = buf;
			if (c == 'd' || c == 'u') {
				do
					*s++ = '0' + (u % 10U);
				while (u /= 10);
			} else {
				do
					*s++ = hex[u & 0xfu];
				while (u >>= 4);
			}
			while (--s >= buf)
				putchar(*s);
			break;
		}
	}
	va_end(ap);
}

Console *console = 0;

}
