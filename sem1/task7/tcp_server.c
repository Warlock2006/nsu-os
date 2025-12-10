#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define QUEUE 2

int main() {
  int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[BUFFER_SIZE];
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);


  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    printf("BIND ERROR!!!!!!!!!\n");
    close(server_sock);
    return 1;
  }

  if (listen(server_sock, QUEUE) < 0) {
    printf("LISTEN ERROR!!!\n");
    close(server_sock);
    return 1;
  }

  printf("TCP server listening on port %d...\n", PORT);

  while (1) {
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

    pid_t child = fork();

    if (child == 0) {
      close(server_sock);

      char client_ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
      int client_port = ntohs(client_addr.sin_port);

      ssize_t n;
      while ((n = read(client_sock, buffer, BUFFER_SIZE - 1)) > 0) {
          buffer[n] = '\0';
          printf("Data recv from client %s:%d: %s\n", client_ip, client_port, buffer);
          if (write(client_sock, buffer, n) < 0) {
              printf("WRITE ERROR!!!!\n");
              break;
          }
      }

      close(client_sock);
      exit(0);
    }
  }

  close(server_sock);
  return 0;
}