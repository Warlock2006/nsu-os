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
  int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[BUFFER_SIZE];
  struct sockaddr_in server_addr;
  socklen_t server_len = sizeof(server_addr);

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (connect(sockfd, (struct sockaddr *)&server_addr, server_len) < 0) {
    printf("CONNECT ERROR!!!\n");
    close(sockfd);
    return 1;
  }

  while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
    size_t len = strlen(buffer);
    if (buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    if (write(sockfd, buffer, len) < 0) {
        perror("WRITE IN SOCKET ERROR");
        close(sockfd);
        return 1;
    }
    printf("Data send: %s\n", buffer);

    ssize_t n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (n < 0) {
        perror("READ ERROR");
        close(sockfd);
        return 1;
    }
    if (n == 0) {
        printf("Server closed connection\n");
        break;
    }
    buffer[n] = '\0';
    printf("Data recv: %s\n", buffer);
  }

  close(sockfd);
  return 0;
}