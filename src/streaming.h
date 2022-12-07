#ifndef __STREAMING__
#define __STREAMING__

#include "../common/streaming.h"

void wait_for_client_connect();
void send_scanline(unsigned char *data, unsigned char linenum);


#endif // __STREAMING__