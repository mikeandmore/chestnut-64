// -*- c++ -*-
#ifndef CONSOLE_H
#define CONSOLE_H

#include "terminal.h"

namespace kernel {
class Console
{
public:
	Console() : x(0), y(0) {}

private:
	int x;
	int y;

public:
	void print(char ch);
};
}

#endif /* CONSOLE_H */
