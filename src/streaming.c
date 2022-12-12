#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "streaming.h"


static int sockfd;
static struct sockaddr_in cliaddr;
static unsigned int clientaddr_len;


// Driver code
void wait_for_client_connect() {

	LargestMsgType rcv[1];
	struct sockaddr_in servaddr;
		
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
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
	recvfrom(
        sockfd,
        (char *)(rcv), sizeof(rcv),
        MSG_WAITALL,
        (struct sockaddr *) &cliaddr, &clientaddr_len
    );
    if (rcv->type != MSGT_HELLO) {
		perror("Client said something other than hello?");
		exit(EXIT_FAILURE);
    }
	printf("Client says hello\n");

    HelloMsg msg;
    msg.type = MSGT_HELLO;
    sendto(
        sockfd,
        (char *)(&msg), sizeof(msg),
        MSG_CONFIRM,
        (const struct sockaddr *) &cliaddr, clientaddr_len
    );

	return;
}


void send_scanline(unsigned char *data, unsigned char linenum) {
    ScanlineMsg msg;
    msg.type = MSGT_SCANLINE;
    msg.idx = linenum;
    memcpy(msg.data, &data[linenum * STREAMWIDTH], STREAMSCANSIZE);
    ssize_t sent = sendto(
        sockfd,
        (char *)(&msg), sizeof(msg),
		MSG_CONFIRM, 
        (const struct sockaddr *) &cliaddr, clientaddr_len
    );
    if (sent == -1) {
		perror("Failed to send scanline");
		exit(EXIT_FAILURE);
    }
	return;
}
