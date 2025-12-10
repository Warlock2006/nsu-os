#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
    printf("PID: %d\n", getpid());
    while (1) {
        char *arr = malloc(8192);
        sleep(1);
    }
}