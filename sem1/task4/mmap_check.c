#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

void handle_sigsegv(int signal) {
    printf("SIGSEGV!!!!! ALARM!!!!!\n");
}

int main() {
    int PAGE_SIZE = getpagesize();
    printf("PID: %d\n", getpid());

    sleep(10);

    void *addr = mmap(NULL, 10 * PAGE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (addr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }    

    printf("MAPPED!!!\n");
    sleep(10);

    signal(SIGSEGV, handle_sigsegv);

    // mprotect(addr, 10 * PAGE_SIZE, PROT_READ);
    // strcpy((char*)addr, "HELLO WORLD!");

    // mprotect(addr, 10 * PAGE_SIZE, PROT_WRITE);
    // printf("READING: %s\n", (char*)addr);

    munmap(addr + 4 * PAGE_SIZE, 3 * PAGE_SIZE);

    sleep(600);

    munmap(addr, 10 * PAGE_SIZE);
    return 0;
}