#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char **argv) {
    printf("PID: %d\n", getpid());
    sleep(10);
    printf("Hello world!\n");
    execvp(argv[0], argv);
}