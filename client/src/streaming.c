#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#include "i_video.h"
#include "streaming.h"


static int sockfd;
static struct sockaddr_in servaddr;
static unsigned int serveraddr_len;

void connect_to_server() {
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&servaddr, 0, sizeof(servaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(STREAMPORT);
	servaddr.sin_addr.s_addr = INADDR_ANY;

	StreamMsg msg;
	msg.type = MSGT_HELLO;
	char response[] = "Where are my frames?";
	memcpy(msg.data, response, sizeof(response));
	sendto(
        sockfd,
        (const char *)(&msg), sizeof(msg),
		MSG_CONFIRM,
        (const struct sockaddr *) &servaddr, sizeof(servaddr)
    );
		
	recvfrom(
        sockfd,
        (char *)(&msg), sizeof(msg),
		MSG_WAITALL,
        (struct sockaddr *) &servaddr, &serveraddr_len
    );
    if (msg.type == MSGT_HELLO) {
        printf("Server says hello\n");
    } else {
        printf("No hello from server, are you reconnecting?\n");
    }

	return;
}


void recv_msgs() {
    while (1) {
    	StreamMsg msg;
    	ssize_t n = recvfrom(
            sockfd, 
            (char *)(&msg), sizeof(msg),
            MSG_DONTWAIT,
            (struct sockaddr *) &servaddr, &serveraddr_len
        );
        if (n == -1) return;

        switch (msg.type) {
            case MSGT_SCANLINE:
                memcpy(&I_VideoBuffer[msg.idx*STREAMWIDTH], msg.data, STREAMSCANSIZE);
                break;

            default:
                perror("Received unknown msg type\n");
                exit(EXIT_FAILURE);
        }
    }
}


void recv_loop() {
    unsigned char last_scanline = 0;

    while (1) {
    	StreamMsg msg;
    	ssize_t n = recvfrom(
            sockfd, 
            (char *)(&msg), sizeof(msg),
            MSG_DONTWAIT,
            (struct sockaddr *) &servaddr, &serveraddr_len
        );
        if (n == -1) {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000;
            if(nanosleep(&ts, &ts)) {
                printf("Nanosleep failed\n");
                exit(1);
            }
            continue;
        }

        switch (msg.type) {
            case MSGT_SCANLINE:
                memcpy(&I_VideoBuffer[msg.idx*STREAMWIDTH], msg.data, STREAMSCANSIZE);
                if (msg.idx < last_scanline) {
                    I_FinishUpdate();
                }
                last_scanline = msg.idx;
                break;

            default:
                perror("Received unknown msg type\n");
                exit(EXIT_FAILURE);
        }
    }
}