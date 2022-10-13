/*******************************************
* Group Name  : XXXXXX

* Member1 Name: XXXXXX
* Member1 SIS ID: XXXXXX
* Member1 Login ID: XXXXXX

* Member2 Name: XXXXXX
* Member2 SIS ID: XXXXXX
* Member2 Login ID: XXXXXX
********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
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

void sigchld_handler(int s){
  int save_errno = errno;
  while(waitpid(-1, NULL, WNOHANG) > 0);
  errno = save_errno;
}

void *get_in_addr(struct sockaddr *sa){
  if(sa->sa_family == AF_INET){
    return &(((struct sockaddr_in*)sa)->sin_addr); // IPV$
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr); // IPV6
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
  int sockfd, new_fd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next){
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      perror("server: socket");
      continue;
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
      perror("setsockopt");
      exit(1);
    }
    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
      close(sockfd);
      perror("server: bind");
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo);

  if(p == NULL){
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if(listen(sockfd, BACKLOG) == -1){
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGCHLD, &sa, NULL) == -1){
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  while(1){
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if(new_fd == -1){
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: get connection from %s\n", s);

    if(!fork()){
      close(sockfd);
      if(send(new_fd, "Hello from server!", 18, 0) == -1){
        perror("send");
      }
      close(new_fd);
      exit(0);
    }
    close(new_fd);
  }

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
