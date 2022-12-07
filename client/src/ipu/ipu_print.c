#include "ipu_interface.h"

static char printbuf[IPUPRINTBUFSIZE];
static char* printbuf_head = printbuf;
static const char* printbuf_end = &printbuf[IPUPRINTBUFSIZE - 1];
static int printed;

void reset_ipuprint() {
  printbuf_head = printbuf;
  *printbuf_head = '\0';
  printed = 0;
}

void ipuprint(const char* str) {
  while (*str != '\0' && printbuf_head != printbuf_end) {
    *(printbuf_head++) = *(str++);
    printed += 1;
  }
  *printbuf_head = '\0';
}

void ipuprintnum(int x) {
  if (x < 0) {
    *(printbuf_head++) = '-';
    x = -x;
  }
  if (x == 0) {
    *(printbuf_head++) = '0';
  }
  const int MAX_DIGITS = 10;
  char digits[MAX_DIGITS];
  int i;
  for (i = 0; x != 0 && i < MAX_DIGITS; ++i) {
    digits[i] = 0x30 + (x % 10);
    x /= 10;
  }
  for (i -= 1; i >= 0; --i) {
    *(printbuf_head++) = digits[i];
  }
  *printbuf_head = '\0';
}

void get_ipuprint_data(char* dst, int dst_size) {
    int limit = (dst_size < IPUPRINTBUFSIZE) ? dst_size : IPUPRINTBUFSIZE;
    for (int i = 0; i < limit; ++i) {
        dst[i] = printbuf[i];
    }
    if (limit <= printed) {
        dst[IPUPRINTBUFSIZE - 10] = '[';
        dst[IPUPRINTBUFSIZE - 9] = 'c';
        dst[IPUPRINTBUFSIZE - 8] = 'o';
        dst[IPUPRINTBUFSIZE - 7] = 'n';
        dst[IPUPRINTBUFSIZE - 6] = 't';
        dst[IPUPRINTBUFSIZE - 5] = '.';
        dst[IPUPRINTBUFSIZE - 4] = '.';
        dst[IPUPRINTBUFSIZE - 3] = '.';
        dst[IPUPRINTBUFSIZE - 2] = ']';
        dst[IPUPRINTBUFSIZE - 1] = '\0';
    }
}
