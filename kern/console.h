// -*- c++ -*-
#ifndef CONSOLE_H
#define CONSOLE_H

#include "terminal.h"

namespace kernel {
class Console
{
public:
	Console() : x(0), y(0) {}

	void putchar(char ch);
	void printf(const char *fmt, ...);
private:
	int x;
	int y;
};

}

#endif /* CONSOLE_H */
