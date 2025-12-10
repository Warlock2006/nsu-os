#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <signal.h>

int main() {
  pid_t child = fork();

  if (child == -1) {
    printf("fork failed!!!!!\n");
    return 1;
  }

  if (child == 0) {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    kill(getpid(), SIGSTOP);
    syscall(SYS_write, STDOUT_FILENO, "Hello world!\n", 13);
    return 0;
  } else {
    int status;
    struct user_regs_struct regs;
    
    while (1) {
      waitpid(child, &status, 0);

      if (WIFEXITED(status) || WIFSIGNALED(status)) {
        break;
      }

      ptrace(PTRACE_GETREGS, child, NULL, &regs);
      printf("Syscall: %lld\n", regs.orig_rax);
      ptrace(PTRACE_SYSCALL, child, NULL, NULL);
    }
  }

  return 0;
}
