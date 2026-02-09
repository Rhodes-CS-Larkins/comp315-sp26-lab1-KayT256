/*
 * ping.c - UDP ping/pong client code
 *          author: <your name>
 */
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "util.h"

#define PORTNO "1266"


int main(int argc, char **argv) {
  int ch, errors = 0;
  int nping = 1;                        // default packet count
  char *ponghost = strdup("localhost"); // default host
  char *pongport = strdup(PORTNO);      // default port
  int arraysize = 100;                  // default packet size

  while ((ch = getopt(argc, argv, "h:n:p:")) != -1) {
    switch (ch) {
    case 'h':
      ponghost = strdup(optarg);
      break;
    case 'n':
      nping = atoi(optarg);
      break;
    case 'p':
      pongport = strdup(optarg);
      break;
    case 's':
      arraysize = atoi(optarg);
      break;
    default:
      fprintf(stderr, "usage: ping [-h host] [-n #pings] [-p port] [-s size]\n");
    }
  }

  // UDP ping implemenation goes here
  // printf("nping: %d arraysize: %d errors: %d ponghost: %s pongport: %s\n",
  //    nping, arraysize, errors, ponghost, pongport);

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;

  double start_time, end_time, total_time = 0.0;

  // I don't know if you will stress test the program, which cause overflow in stack,
  // so I use heap to be safe
  char *data = (char *) malloc(arraysize * sizeof(char));

  // Again, in case you stress test it to over the heap limit :)) 
  // I know you don't do it. But yeah, just to be safe
  if (data == NULL) {
    fprintf(stderr, "failed to allocate memory\n");
  }

  memset(data, 200, arraysize);

  char *recv_buf = (char *) malloc(arraysize * sizeof(char));

  // Configure address info (put in servinfo)
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; //ipv4
  hints.ai_socktype = SOCK_DGRAM; // UDP

  if ((rv = getaddrinfo(ponghost, pongport, &hints, &servinfo)) != 0) {
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

    break; // success
  }

  if (p == NULL) {
    fprintf(stderr, "ping: failed to create socket\n");
    exit(1);
  }

  // Communication loop
  for (int i = 0; i < nping; i++) {
    // i. start the timer
    start_time = get_wctime();

    // ii. send the array elements in a single packet to the pong server
    if ((numbytes = sendto(sockfd, data, arraysize, 0, p->ai_addr, p->ai_addrlen)) == -1) {
      perror("ping: sendto");
      exit(1);
    }

    // iii. wait and receive the reply from the pong server
    if ((numbytes = recvfrom(sockfd, recv_buf, arraysize, 0, NULL, NULL)) == -1) {
      perror("ping: recvfrom");
      exit(1);
    }

    // iv. stop the timer
    end_time = get_wctime();

    // calculate RTT
    double rtt = (end_time - start_time) * 1000;
    total_time += rtt;

    // v. validate the results sent from the pong server
    for (int j = 0; j < numbytes; j++) {
      if ((unsigned char) recv_buf[j] != 201) {
        errors++;
        break; // assume that we only need to count the packets that have error (not all errors)
      }
    }

    // vi. print out the round-trip time
    printf("ping[%d]: round-trip time: %.3f ms\n", i, rtt);
  }

  // Final statistics
  if (errors == 0) {
    printf("no errors detected\n");
  } else {
    printf("%d errors detected\n", errors);
  }

  double avg_time = total_time / nping;
  printf("time to send %d packets of %d bytes %.3f ms (%.3f avg per packet)\n", nping, arraysize, total_time, avg_time);

  // cleanup
  free(data);
  free(recv_buf);
  freeaddrinfo(servinfo);
  close(sockfd);

  return 0;
}
