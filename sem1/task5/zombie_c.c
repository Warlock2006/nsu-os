#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
  pid_t pid1 = fork();

  if (pid1 < 0) {
    perror("fork");
    exit(1);
  }
  if (pid1 == 0) {
    printf("PARENT PID: %d, PPID: %d\n", getpid(), getppid());
    pid_t pid2 = fork();
    
    if (pid2 < 0) {
      perror("fork");
      exit(1);
    }

    if (pid2 == 0) {
      printf("CHILD PID: %d, PPID: %d\n", getpid(), getppid());
      sleep(20);
      exit(0);
    } else {
      exit(0);
    }
  } else {
      sleep(30);
  }
  return 0;
}
