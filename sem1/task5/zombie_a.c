#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  pid_t parent_pid = getpid();
  printf("PID: %d\n", parent_pid);

  pid_t child = fork();

  if (child == -1) {
    printf("FORK ERROR!!!!!\n");
    return 1;
  }

  if (child == 0) {
    printf("CHILD PID: %d\n", getpid());
    sleep(10);
    exit(5);
  } else {
    sleep(30);
    int status;
    pid_t waited = waitpid(child, &status, 0);
    
    if (waited == -1) {
        perror("waitpid failed");
        return 1;
    }
    
    if (WIFEXITED(status)) {
      printf("Child exited with code: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("Child terminated by signal: %d\n", WTERMSIG(status));
    } else {
      printf("Child exited abnormally.\n");
    }
    exit(0);
  }
  return 0;
}