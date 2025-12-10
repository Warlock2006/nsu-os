#include <unistd.h>
#include <stdio.h>

void recursive_check() {
    char arr[4096];
    printf("ARR IN STACK ALLOCATED!!!\n");
    sleep(1);
    recursive_check();
}

int main() {
    printf("PID: %d\n", getpid());
    sleep(10);
    recursive_check();
    return 0;
}