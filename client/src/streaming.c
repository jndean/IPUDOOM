#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>

#include "d_event.h"
#include "i_video.h"
#include "streaming.h"


static int sockfd;
static struct sockaddr_in servaddr;
static unsigned int serveraddr_len;


void connect_to_server(void) {
	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&servaddr, 0, sizeof(servaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(STREAMPORT);
	servaddr.sin_addr.s_addr = INADDR_ANY;

	HelloMsg msg;
	msg.type = MSGT_HELLO;
	sendto(sockfd,
           (const char *)(&msg), sizeof(msg),
           MSG_CONFIRM,
           (const struct sockaddr *) &servaddr, sizeof(servaddr));
	
    LargestMsgType rcv[1];
	recvfrom(sockfd,
             (char *)(rcv), sizeof(rcv),
             MSG_WAITALL,
             (struct sockaddr *) &servaddr, &serveraddr_len);
    if (rcv->type == MSGT_HELLO) {
        printf("Server says hello\n");
    } else {
        printf("No hello from server, are you reconnecting?\n");
    }

	return;
}


void send_events(void) {
    I_GetEvent();

    EventMsg msg;
    msg.type = MSGT_EVENT;
    msg.count = 0;
    
    // Retreive and serialise events
    event_t *ev;
    while ((ev = D_PopEvent()) != NULL) {
        if (msg.count < STREAMEVENTSPERMESSAGE) {
            ((event_t*) msg.events)[msg.count] = *ev;
        } else if (msg.count == 255) {
            printf("\nmsg.count was about to overflow! Too many events?\n");
            exit(EXIT_FAILURE);
        } else {
            printf("WARNING: exceeding %d events per frame, dropping some key presses\n",
                   STREAMEVENTSPERMESSAGE);
        }
        msg.count++;
    }

    // Send the packet
    if (msg.count > 0) {
        sendto(sockfd,
               (const char *)(&msg), sizeof(msg),
               0,
               (const struct sockaddr *) &servaddr, sizeof(servaddr));
    }
}


void ping(void) {
    const int frequency = 64;
    static int call_count = 0;

    call_count = (call_count + 1) % frequency;
    if (call_count) return;

    PingMsg msg;
    msg.type = MSGT_PING;
    gettimeofday(&msg.sent_time, NULL);
    sendto(sockfd,
           (const char *)(&msg), sizeof(msg),
           0,
           (const struct sockaddr *) &servaddr, sizeof(servaddr)
    );
}


void flush_incoming(void) {
    while (1) {
    	LargestMsgType rcv[1];
    	ssize_t n = recvfrom(sockfd, 
                             (char *)rcv, sizeof(rcv),
                             MSG_DONTWAIT,
                             NULL, NULL);
        if (n == -1) return;
    }
}


void recv_loop(void) {
    unsigned char last_scanline = 0;

    while (1) {
    	LargestMsgType rcv[1];
    	ssize_t n = recvfrom(sockfd, 
                             (char *)rcv, sizeof(rcv),
                             MSG_DONTWAIT,
                             (struct sockaddr *) &servaddr, &serveraddr_len);

        if (n == -1) {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000;
            if(nanosleep(&ts, &ts)) {
                printf("Nanosleep failed\n");
                exit(EXIT_FAILURE);
            }
            continue;
        }

        switch (rcv->type) {
            case MSGT_SCANLINE: {
                ScanlineMsg *msg = (ScanlineMsg*) rcv;
                memcpy(&I_VideoBuffer[msg->idx*STREAMWIDTH], msg->data, STREAMSCANSIZE);
                if (msg->idx < last_scanline - 100) {
                    send_events();
                    ping();
                    I_FinishUpdate();
                                
                    /*static int tmp_count = 0;
                    if (tmp_count++ == 128) {
                        printf("----FLUSH!----\n");
                        tmp_count = 0;
                        flush_incoming();
                    }*/
                }
                last_scanline = msg->idx;
                break;
            }

            case MSGT_PING: {
                PingMsg *msg = (PingMsg*) rcv;
                struct timeval T;
                gettimeofday(&T, NULL);
                double delta = (T.tv_usec - msg->sent_time.tv_usec) / 1000.;
                delta += (T.tv_sec - msg->sent_time.tv_sec) * 1000.;
                printf("PING: %.1lf\n", delta);
                break;
            }

            default:
                printf("Received unknown msg type %d\n", (int)rcv->type);
                exit(EXIT_FAILURE);
        }
    }
}