#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
  int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  char buffer[BUFFER_SIZE];
  struct sockaddr_in server_addr;
  socklen_t server_len = sizeof(server_addr);

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  while (1) {
    snprintf(buffer, BUFFER_SIZE, "Hello server from PID: %d", getpid());
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("Data send: %s\n", buffer);


    memset(buffer, 0, BUFFER_SIZE);
    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
    printf("Data recv: %s\n", buffer);
    
    sleep(5);
  }
  return 0;
}