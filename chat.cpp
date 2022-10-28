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
#include <climits>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

#define MYPORT "0" // the port users will be connecting to
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

  //printf("Packed Version: %d\n", net_version);
  //printf("Packed Length: %d\n", net_length);

  char* buf = (char*)(calloc(144, 1));
  char* version = (char*)(calloc(2,1));
  char* length = (char*)(calloc(2,1));

  memcpy(version, (char*)&net_version, 2);
  memcpy(buf, version, 2);
  
  memcpy(length, (char*)&net_length, 2);
  memcpy(buf+2, length, 2);

  memcpy(buf+4, packet->message, packet->length);

  return buf;
}


bool unpack(char* data){
  uint16_t version;
  uint16_t length;

  version = (unsigned char)data[0] << CHAR_BIT | (unsigned char)data[1];
  length = (unsigned char)data[2] << CHAR_BIT | (unsigned char)data[3];

  char packetText[length];
  packetText[length] = '\0';
  
  memcpy(packetText, &data[4], length);

  //TODO: Check Version number, return false if number is wrong. 

  if(version != 457){
    printf("ERROR: Wrong packet version \n");
    return false;
  }


  //printf("Unpacked Version: %d\n", version);
  //printf("Unpacked Length: %d\n", length);
  printf("Friend: %s \n", packetText);
  //dont even have to return a chat packet, just need to display the text. 

  return true;
}

void sigchld_handler(int s) {
  int save_errno = errno;
  while(waitpid(-1, NULL, WNOHANG) > 0);
  errno = save_errno;
}

void *get_in_addr(struct sockaddr *sa) {
  if(sa->sa_family == AF_INET){
    return &(((struct sockaddr_in*)sa)->sin_addr); // IPV4
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
  //char send_buf[MAXDATASIZE];
  close(*sockfd);
  
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

    if(!unpack(recv_buf)){
      continue;
    }

    uint16_t length = 0;

    //Take Input and Build packet
    printf("You: ");
    char* textToSend = NULL;
    size_t len = 0;
    getline(&textToSend, &len, stdin);
    while (len > 144){
      printf("ERROR: Input too long. \n");
      printf("You: ");
      len = 0;
      getline(&textToSend, &len, stdin);
    }

    length = strlen(textToSend) - 1;
    struct chat_packet packetToSend = {CHAT_VERSION, length, textToSend};

    //Pack it up
    char* packedPacket = pack(&packetToSend);
    //printf(packedPacket);

    //Send it
    if(send(*new_fd, packedPacket, length+4, 0) == -1){
      perror("send");
    }

    //if(unpack(packedPacket)){
    //  continue;
    //}
  }
  close(*new_fd);
  exit(0);
}

void accept_connections(struct sockaddr_storage *their_addr, int *sockfd){
  int new_fd;
  char s[INET6_ADDRSTRLEN];
  socklen_t sin_size;

  uint16_t port = 0;
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(*sockfd, (struct sockaddr *)&sin, &len) == -1)
    perror("getsockname");
  else
    port = ntohs(sin.sin_port);

  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, "eno1", IFNAMSIZ-1);
  ioctl(*sockfd, SIOCGIFADDR, &ifr);
  printf("Waiting for connection on %s port %d\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), port);


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
    printf("Found a friend! You recieve first.\n");

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
  int sockfd;
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
  printf("Sonnecting to server...\n");
  printf("Connected!\n");
  printf("Connected to a friend! You send first.\n");

  freeaddrinfo(servinfo);

  while(1){ 
    uint16_t length = 0;
    
    //Take Input and Build packet
    printf("You: ");
    char* textToSend = NULL;
    size_t len = 0;
    getline(&textToSend, &len, stdin);
    if (len > 144){
      printf("ERROR: Input too long. \n");
      continue;
    }

    //fgets(textToSend, MAXDATASIZE, stdin);
    length = strlen(textToSend) - 1;
    struct chat_packet packetToSend = {CHAT_VERSION, length, textToSend};

    //Pack it up
    char* packedPacket = pack(&packetToSend);
    //printf(packedPacket);

    //Send it
    if(send(sockfd, packedPacket, length+4, 0) == -1){
      perror("send");
    }

    int numbytes;
    char recv_buf[MAXDATASIZE];
    if((numbytes = recv(sockfd, recv_buf, MAXDATASIZE-1, 0)) == -1){
      perror("recv");
      exit(1);
    }
    
    if(!unpack(recv_buf)){
      continue;
    }


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
    printf("Welcome to chat!\n");
    return server();
  }
  else if(valid_usage){
    // client
    //printf("starting client\n");
    return client(hostname, port);
  }
  return 0;
}
