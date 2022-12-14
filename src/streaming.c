#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "d_event.h"
#include "i_video.h"
#include "streaming.h"


static int sockfd;
static struct sockaddr_in cliaddr;
static unsigned int clientaddr_len;


// Driver code
void wait_for_client_connect(void) {

	LargestMsgType rcv[1];
	struct sockaddr_in servaddr;
		
	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
		
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(STREAMPORT);
		
	// Bind the socket with the server address
	if (bind(sockfd, (const struct sockaddr *)&servaddr,
			sizeof(servaddr)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
		
	clientaddr_len = sizeof(cliaddr);
	recvfrom(sockfd,
             (char *)(rcv), sizeof(rcv),
             MSG_WAITALL,
             (struct sockaddr *) &cliaddr, &clientaddr_len);
    if (rcv->type != MSGT_HELLO) {
		perror("Client said something other than hello?");
		exit(EXIT_FAILURE);
    }
	printf("Client says hello\n");

    HelloMsg msg;
    msg.type = MSGT_HELLO;
    sendto(sockfd,
           (char *)(&msg), sizeof(msg),
           MSG_CONFIRM,
           (const struct sockaddr *) &cliaddr, clientaddr_len);

	return;
}


void send_scanline(unsigned char *data, unsigned char linenum) {
    ScanlineMsg msg;
    msg.type = MSGT_SCANLINE;
    msg.idx = linenum;
    memcpy(msg.data, &data[linenum * STREAMWIDTH], STREAMSCANSIZE);
    ssize_t sent = sendto(sockfd,
                         (char *)(&msg), sizeof(msg),
                         MSG_CONFIRM, 
                         (const struct sockaddr *) &cliaddr, clientaddr_len);
    if (sent == -1) {
		perror("Failed to send scanline");
		exit(EXIT_FAILURE);
    }
	return;
}


void recv_events(void) {
    while (1) {
    	LargestMsgType rcv[1];
    	ssize_t n = recvfrom(sockfd, 
                             (char *)rcv, sizeof(rcv),
                             MSG_DONTWAIT,
                             (struct sockaddr *) &cliaddr, &clientaddr_len);
        if (n == -1) break;

        if  (rcv->type != MSGT_EVENT) {
            printf("Expected to receive MSGT_EVENT(%d), but got %d\n", MSGT_EVENT, rcv->type);
            exit(EXIT_FAILURE);
        }

        EventMsg *msg = (EventMsg*) rcv;
        for (int i = 0; i < msg->count; ++i) {
            D_PostEvent(&((event_t*)msg->events)[i]);
        }
    }
}


void stream_send_receive(unsigned char *data) {
    const int stride = 1;  // Controls interlacing, 1 = no-interlacing 
    static int offset = 0;
    for (int i = offset; i < SCREENHEIGHT; i += STREAMLINESPERMESSAGE * stride) {
        send_scanline(data, i);
    }
    offset = (offset + 1) % stride;

    while (1) {
    	LargestMsgType rcv[1];
    	ssize_t n = recvfrom(sockfd, 
                             (char *)rcv, sizeof(rcv),
                             MSG_DONTWAIT,
                             (struct sockaddr *) &cliaddr, &clientaddr_len);
        if (n == -1) break;

        switch(rcv->type) {
            case MSGT_EVENT: { 
                EventMsg *msg = (EventMsg*) rcv;
                for (int i = 0; i < msg->count; ++i) {
                    D_PostEvent(&((event_t*)msg->events)[i]);
                }
                break;
            }

            case MSGT_PING: {
                PingMsg *msg = (PingMsg*) rcv;
                sendto(sockfd,
                       (char *)(msg), sizeof(*msg),
                       MSG_CONFIRM,
                       (const struct sockaddr *) &cliaddr, clientaddr_len);
                break;
            }

            default:
                printf("Unexpected message type %d\n", (int) rcv->type);
                exit(EXIT_FAILURE);
        }

    }
}