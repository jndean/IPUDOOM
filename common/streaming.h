#ifndef __COMMON_STREAMING__
#define __COMMON_STREAMING__


#define STREAMPORT            (6666)
#define STREAMWIDTH           (320)
#define STREAMLINESPERMESSAGE (2)

#define STREAMSCANSIZE (STREAMLINESPERMESSAGE*STREAMWIDTH)

typedef struct msg_ {
	unsigned char type;
    unsigned char idx;
	unsigned char data[STREAMSCANSIZE];
} StreamMsg;


#define MSGT_HELLO (0)
#define MSGT_SCANLINE (1)


#endif  // __COMMON_STREAMING__