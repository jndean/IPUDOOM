#ifndef __COMMON_STREAMING__
#define __COMMON_STREAMING__

#include <sys/time.h>


#define STREAMPORT             (6666)
#define STREAMWIDTH            (320)
#define STREAMLINESPERMESSAGE  (1)
#define STREAMEVENTSPERMESSAGE (5)
#define STREAMSCANSIZE (STREAMLINESPERMESSAGE * STREAMWIDTH)


typedef struct {
	unsigned char type;
} HelloMsg;

typedef struct {
	unsigned char type;
    unsigned char idx;
	unsigned char data[STREAMSCANSIZE];
} ScanlineMsg;

typedef struct {
	unsigned char type;
    unsigned char count;
	unsigned char events[20 * STREAMEVENTSPERMESSAGE];
} EventMsg;

typedef struct {
	unsigned char type;
    struct timeval sent_time;
} PingMsg;

typedef ScanlineMsg LargestMsgType;


#define MSGT_HELLO (0)
#define MSGT_SCANLINE (1)
#define MSGT_EVENT (2)
#define MSGT_PING (3)


#endif  // __COMMON_STREAMING__