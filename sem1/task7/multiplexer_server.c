#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define QUEUE 10

int main() {
  int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[BUFFER_SIZE];
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  fd_set read_fds, master_fds;
  int client_sockets[QUEUE] = {0};

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

  FD_ZERO(&master_fds);
  FD_SET(server_sock, &master_fds);
  int max_fd = server_sock;

  while (1) {
    read_fds = master_fds;
    if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
      printf("SELECT ERROR!!!\n");
      return 1;
    }
    for (int fd = 0; fd <= max_fd; fd++) {
      if (!FD_ISSET(fd, &read_fds)) continue;
    
      if (fd == server_sock) {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
          printf("ACCEPT ERROR!!!\n");
          continue;
        }


        char flag = 0;
        for (int i = 0; i < QUEUE; i++) {
          if (!client_sockets[i]) {
            client_sockets[i] = client_sock;
            FD_SET(client_sock, &master_fds);
            if (client_sock > max_fd) max_fd = client_sock;
            flag = 1;
            break; // добавили нашего клиента в свободную ячейку массива клиентов и в наш массив дескрипторов
          }
        }
        if (!flag) {
          printf("TOO MANY CLIENTS!!!\n");
          close(client_sock);
        }
      } else {
        ssize_t n = read(fd, buffer, BUFFER_SIZE);
        if (n <= 0) {
          close(fd);
          FD_CLR(fd, &master_fds);
          
          for (int i = 0; i < QUEUE; i++) {
            if (client_sockets[i] == fd) {
              client_sockets[i] = 0;
              printf("CLIENT %d DISCONNECTED!!!\n", i + 1);
              break;
            }
          }
        } else {
          buffer[n] = '\0';
          printf("Data recv: %s\n", buffer);
          if (write(fd, buffer, n) < 0) {
            printf("WRITE ERROR!!!\n");
          }
        }
      }
    }
  }

  close(server_sock);
  return 0;
}