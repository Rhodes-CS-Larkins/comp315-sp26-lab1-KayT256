/*
 * pong.c - UDP ping/pong server code
 *          author: <your name>
 */
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "util.h"

#define PORTNO "1266"
#define MAXBUFLEN 65535 // max UDP size is 65535

int main(int argc, char **argv) {
  int ch;
  int nping = 1;                    // default packet count
  char *pongport = strdup(PORTNO);  // default port

  while ((ch = getopt(argc, argv, "h:n:p:")) != -1) {
    switch (ch) {
    case 'n':
      nping = atoi(optarg);
      break;
    case 'p':
      pongport = strdup(optarg);
      break;
    default:
      fprintf(stderr, "usage: pong [-n #pings] [-p port]\n");
    }
  }

  // pong implementation goes here.
  // printf("nping: %d pongport: %s\n", nping, pongport);

  int sockfd;
  struct addrinfo hints, *servinfo, *p; // for getaddrinfo()
  int rv;

  struct sockaddr_storage their_addr; // storage for sender's address
  socklen_t addr_len; // size of the address

  char buf[MAXBUFLEN]; // hold incoming data array
  int numbytes; // number of bytes we receive

  // Configure address info (put in servinfo)
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; //ipv4
  hints.ai_socktype = SOCK_DGRAM; // UDP
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, pongport, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // Iterate through each result to find one to bind
  for (p = servinfo; p != NULL; p = p->ai_next) {
    // create a socket
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("pong: socket");
      continue;
    }

    // try to bind socket to port
    if (bind(sockfd, p->ai_addr, p->ai_addrlen)) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break; // success
  }

  freeaddrinfo(servinfo);

  // fail to bind
  if (p == NULL) {
    fprintf(stderr, "pong: failed to bind");
    exit(1);
  }

  // printf("pong: listening on port %s for %d pings\n", pongport, nping);

  // Main loop (receiv --> print --> modify --> reply)
  for (int i = 0; i < nping; i++) {
    addr_len = sizeof their_addr;
    
    // i. receive the array elements in a single packet from the ping client
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, 
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
      perror("recvfrom");
      exit(1);
    }

    // display packet with source address
    char s[INET6_ADDRSTRLEN];

    inet_ntop(AF_INET, &(((struct sockaddr_in *)&their_addr)->sin_addr), s, sizeof s);

    printf("pong[%d]: received packet from %s\n", i, s);

    // ii. add one to every element in the array
    for (int j = 0; j < numbytes; j++) {
      buf[j] += 1;
    }

    // iii. send the array back in a UDP packet to the ping client
    if (sendto(sockfd, buf, numbytes, 0, 
        (struct sockaddr *)&their_addr, addr_len) == -1) {
      perror("sendto");
      exit(1);
    }
  }
  close(sockfd);
  return 0;
}

