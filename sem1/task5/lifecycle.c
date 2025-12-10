#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int global = 150;

int main(int argc, char **argv) {
  int local = 200;
  pid_t parent_pid = getpid();
  printf("Global var address: %p, value: %d\n", &global, global);
  printf("Local var address: %p, value: %d\n", &local, local);

  printf("PID: %d\n", parent_pid);

  pid_t child = fork();

  if (child == -1) {
    printf("FORK ERROR!!!!!\n");
    return 1;
  }

  if (child == 0) {
    printf("PARENT PID: %d\n", parent_pid);
    printf("CHILD PID: %d\n", getpid());

    printf("Before\n");
    printf("Global var address: %p, value: %d\n", &global, global);
    printf("Local var address: %p, value: %d\n", &local, local);
    global = 100;
    local = 150;
    printf("After\n");
    printf("Global var address: %p, value: %d\n", &global, global);
    printf("Local var address: %p, value: %d\n", &local, local);
    sleep(30);
  } else {
    printf("Global var address: %p, value: %d\n", &global, global);
    printf("Local var address: %p, value: %d\n", &local, local);
    sleep(30);
    int exit_status;
    wait(&exit_status);
    printf("Child exit status: %d\n", WEXITSTATUS(exit_status));
  }
  return 0;
}