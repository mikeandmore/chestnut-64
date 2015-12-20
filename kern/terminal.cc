#include "terminal.h"
#include "mm/allocator.h"

namespace kernel {

volatile u16 *const Terminal::kVGABuffer = (u16*) PADDR_TO_KPTR(0x0B8000);

void Terminal::DrawChar(char ch, Color fg, Color bg, uint8 x, uint8 y)
{
  if (x < 0 || x >= kVGAWidth || y < 0 || y >= kVGAHeight)
    return;
  uint16 idx = x + y * kVGAWidth;
  u8 color = fg | bg << 4;
  u16 val = ch;
  val |= ((u16) color << 8);
  kVGABuffer[idx] = val;
}

void Terminal::DrawString(const char *str, Color fg, Color bg, uint8 x, uint8 y)
{
  const char *p = str;
  char ch = 0;
  while ((ch = *p++)) {
    DrawChar(ch, fg, bg, x++, y);
  }
}

void Terminal::Reset()
{
  for (int i = 0; i < kVGAWidth * kVGAHeight; i++) {
    kVGABuffer[i] = 0x0f00 | ' ';
  }

}

void Terminal::DrawChar(char ch, uint8 x, uint8 y)
{
  DrawChar(ch, fg_, bg_, x, y);
}

void Terminal::DrawString(const char *str, uint8 x, uint8 y)
{
  DrawString(str, fg_, bg_, x, y);
}

void Terminal::DrawChar(char ch)
{
  DrawChar(ch, x, y);
}

void Terminal::DrawString(const char *str)
{
  DrawString(str, x, y);
}

}

// export global term
static kernel::Terminal term;

template <>
kernel::Terminal &GlobalInstance()
{
  return term;
}
