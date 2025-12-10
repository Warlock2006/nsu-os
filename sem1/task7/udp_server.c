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

  if (sockfd < 0) {
    printf("SOCKET OPEN ERROR!!!\n");
    return 1;
  }

  char buffer[BUFFER_SIZE];
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    printf("BIND ERROR!!!!!!!!!\n");
    close(sockfd);
    return 1;
  }
  
  while (1) {
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &client_len);
    buffer[n] = '\0';

    printf("Data recv: %s\n", buffer);
    
    char client_ip[INET_ADDRSTRLEN];
    
    sendto(sockfd, buffer, n, 0, (struct sockaddr *)&client_addr, client_len);
  }

  close(sockfd);
  return 0;
}