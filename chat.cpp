/*******************************************
* Group Name  : XXXXXX

* Member1 Name: XXXXXX
* Member1 SIS ID: XXXXXX
* Member1 Login ID: XXXXXX

* Member2 Name: XXXXXX
* Member2 SIS ID: XXXXXX
* Member2 Login ID: XXXXXX
********************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

#define MYPORT "3490" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold

int client() {
  // set up tcp connection
  // loop
  //   prompt user for message
  //   send the message
  //   block to recieve message
  //   recieve message and print
  return 0;
}

int server() {
  // set up TCP port and listen for connection
  // print out IP and Port
  // accept client connection
  // loop
  //   block to revieve message
  //   revieve and print message
  //   prompt user for message to send
  //   send message to client

  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  struct addrinfo hints, *res;
  int sockfd, new_fd;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo(NULL, MYPORT, &hints, &res);

  sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  bind(sockfd, res->ai_addr, res->ai_addrlen);
  listen(sockfd, BACKLOG);

  addr_size = sizeof their_addr;
  new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

  char msg[] = "This is Greyson's server";
  int len = strlen(msg);
  int bytes_sent;

  bytes_sent = send(sockfd, msg, len, 0);

  close(sockfd);

  return 0;
}

void help() {
  char out[] = "Usage:\n  start server: chat\n  start client: chat -p <server port> -s <server address>\n";
  printf(out);
}

int main(int argc, char* argv[]){
  const char* ip = "-1";
  const char* port = "-1";
  bool valid_usage = true;

  int c;
  while ((c = getopt(argc, argv, "p:s:h")) != -1) {
    switch(c) {
      case 'p':
        // server port
        port = optarg;
        //cout << "Port: " << port << "\n";
        break;
      case 's':
        // server address
        ip = optarg;
        //cout << "IP: " << ip << "\n";
        break;
      case 'h':
        help();
        valid_usage = false;
        break;
      case '?':
        // invalid options
        valid_usage = false;
        break;
    }
  }

  if(!strcmp(ip, "-1") && !strcmp(port, "-1") && valid_usage) {
    // server
    printf("starting server\n");
    return server();
  }
  else if(valid_usage){
    // client
    printf("starting client\n");
    return client();
  }
  return 0;
}
