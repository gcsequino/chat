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

#define MYPORT "3500" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXDATASIZE 140 // max length of chat message
#define CHAT_VERSION 457 // version of chat protocol

struct chat_packet {
  uint16_t  version = htons(CHAT_VERSION); // htons to convert to Big-Endian
  uint16_t  length;
  char*      message;
};



char* pack(struct chat_packet *packet){ 
  uint16_t net_version = htons(packet->version);
  uint16_t net_length = htons(packet->length);

  //char* buf;
  //*buf++ = net_version >> 8;
  //*buf++ = net_version;
  
  //*buf++ = net_length >> 8;
  //*buf++ = net_length;

  // use memcopy here
  //buf = packet->message;

  //printf("%s", packet->message);

  char* buf = (char*)(calloc(144, 1));
  char* temp = (char*)(calloc(2,1));
  memcpy(temp, (char*)&net_version, 2);
  strcat(buf, temp);
  memcpy(temp, (char*)&net_length, 2);
  strcat(buf, temp);
  strcat(buf, packet->message);
  //printf("%s", buf);
  //return buf;
}


struct chat_packet unpack(char* data){
  uint16_t version;
  uint16_t length;

  version = 

  printf("%d", version);
  printf("%s", length);

}

void sigchld_handler(int s) {
  int save_errno = errno;
  while(waitpid(-1, NULL, WNOHANG) > 0);
  errno = save_errno;
}

void *get_in_addr(struct sockaddr *sa) {
  if(sa->sa_family == AF_INET){
    return &(((struct sockaddr_in*)sa)->sin_addr); // IPV$
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr); // IPV6
}

void format_hints(struct addrinfo *hints) {
  memset(hints, 0, sizeof *hints);
  hints->ai_family = AF_UNSPEC;
  hints->ai_socktype = SOCK_STREAM;
  hints->ai_flags = AI_PASSIVE;
}

int resolve_addrinfo(struct addrinfo *hints, struct addrinfo **servinfo){
  int rv;
  if ((rv = getaddrinfo(NULL, MYPORT, hints, servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  return 0;
}

void connect(struct addrinfo *servinfo, int *sockfd) {
  struct addrinfo *p;
  int yes=1;
  for(p = servinfo; p != NULL; p = p->ai_next){
    if((*sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      perror("server: socket");
      continue;
    }
    if(setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
      perror("setsockopt");
      exit(1);
    }
    if(bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1){
      close(*sockfd);
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

  if(listen(*sockfd, BACKLOG) == -1){
    perror("listen");
    exit(1);
  }
}

void reap(){
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGCHLD, &sa, NULL) == -1){
    perror("sigaction");
    exit(1);
  }
}

void child(int *sockfd, int *new_fd) {
  int numbytes;
  char recv_buf[MAXDATASIZE];
  char send_buf[MAXDATASIZE];
  //close(*sockfd);
  if(send(*new_fd, "Hello from server!", 18, 0) == -1){
    perror("send");
  }
  // loop
  //   block to revieve message
  //   revieve and print message
  //   prompt user for message to send
  //   send message to client
  while(1){
    if((numbytes = recv(*new_fd, recv_buf, MAXDATASIZE-1, 0)) == -1){
      perror("recv");
      exit(1);
    }
    recv_buf[numbytes] = '\0';
    printf("server: received '%s'\n", recv_buf);
  }
  close(*new_fd);
  exit(0);
}

void accept_connections(struct sockaddr_storage *their_addr, int *sockfd){
  int new_fd;
  char s[INET6_ADDRSTRLEN];
  socklen_t sin_size;
  printf("server: waiting for connections...\n");
  while(1){
    sin_size = sizeof *their_addr;
    new_fd = accept(*sockfd, (struct sockaddr *)their_addr, &sin_size);
    if(new_fd == -1){
      perror("accept");
      continue;
    }

    inet_ntop(their_addr->ss_family,
              get_in_addr((struct sockaddr *)their_addr),
              s, sizeof s);
    printf("server: get connection from %s\n", s);

    if(!fork()){
      child(sockfd, &new_fd);
    }
    // close(new_fd);
  }
}

int client(const char* hostname, const char* port) {
  // set up tcp connection
  // loop
  //   prompt user for message
  //   send the message
  //   block to recieve message
  //   recieve message and print
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;


  if((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next){
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      perror("client: socket");
      continue;
    }
    if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
      close(sockfd);
      perror("client: connect");
      continue;
    }
    break;
  }

  if(p == NULL){
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
  printf("client: connecting to %s\n", s);

  freeaddrinfo(servinfo);

  if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1){
    perror("recv");
    exit(1);
  }

  buf[numbytes] = '\0';
  
  printf("client: received '%s'\n", buf);

  while(1){ 
    uint16_t length = 0;
    char textToSend[MAXDATASIZE];
    
    //Take Input and Build packet
    printf("You: ");
    fgets(textToSend, MAXDATASIZE, stdin);
    length = strlen(textToSend) - 1;
    chat_packet packetToSend[length] = {CHAT_VERSION, length, textToSend};

    //Pack it up
    char* packedPacket = pack(packetToSend);
    printf(packedPacket);

    //Send it
    if(send(sockfd, packedPacket, length + 2, 0) == -1){
      perror("send");
    }

    unpack(packedPacket);

    

    //printf("%d , %d, %s", packetToSend->version, packetToSend->length, packetToSend->message);



  }

  close(sockfd);
  
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

  int sockfd;
  struct addrinfo hints, *servinfo;
  struct sockaddr_storage their_addr;

  format_hints(&hints);

  if(resolve_addrinfo(&hints, &servinfo)) return 1;

  // make connection from addrinfo list (servinfo, sockfd)
  connect(servinfo, &sockfd);

  // reap zombies
  reap();

  // accept connections (their_addr)
  accept_connections(&their_addr, &sockfd);

  return 0;
}

void help() {
  char out[] = "Usage:\n  start server: chat\n  start client: chat -p <server port> -s <server address>\n";
  printf(out);
}

int main(int argc, char* argv[]){
  const char* hostname = "-1";
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
        hostname = optarg;
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

  if(!strcmp(hostname, "-1") && !strcmp(port, "-1") && valid_usage) {
    // server
    printf("starting server\n");
    return server();
  }
  else if(valid_usage){
    // client
    printf("starting client\n");
    return client(hostname, port);
  }
  return 0;
}
