#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/signal.h>

#define STACK_SIZE 1024 * 1024

void rec_hello(int depth) {
  if (depth == 0) return;
  char message[] = "Hello world!";
  rec_hello(depth - 1);
}

int clone_start() {
  rec_hello(10);
  return 0;
}

int main(int argc, char **argv) {
  int file_d = open("stack.bin", O_RDWR | O_TRUNC | O_CREAT, 0600);
  
  ftruncate(file_d, STACK_SIZE);
  
  void *my_stack = mmap(NULL, STACK_SIZE, PROT_WRITE | PROT_READ | PROT_GROWSDOWN, MAP_SHARED, file_d, 0);

  void *my_stack_top = my_stack + STACK_SIZE;

  pid_t pid = clone(clone_start, my_stack_top, SIGCHLD, NULL);

  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status)) {
    printf("CLONE FINISHED!!!!\n");
  }

  munmap(my_stack, STACK_SIZE);
  close(file_d);
  return 0;
}
