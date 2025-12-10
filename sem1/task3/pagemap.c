#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

void print_bits(uint64_t val) {
    for (int i = 63; i >= 0; i--) {
        printf("%d", (val >> i) & 1);
        if (i % 8 == 0) printf(" ");
    }
    printf("\n");
}

void decode_entry(uint64_t entry) {
    int present = (entry >> 63) & 1;
    int swapped = (entry >> 62) & 1;
    int file_page = (entry >> 61) & 1;
    int soft_dirty = (entry >> 55) & 1;
    uint64_t pfn = entry & ((1ULL << 55) - 1);

    printf("\tpresent = %d\n", present);
    printf("\tswapped = %d\n", swapped);
    printf("\tfile/shared = %d\n", file_page);
    printf("\tsoft-dirty = %d\n", soft_dirty);
    printf("\tPFN = 0x%llx\n", (unsigned long long)pfn);
}

int read_pagemap_entry(int pagemap_fd, unsigned long vaddr, size_t page_size, uint64_t* entry) {
    uint64_t offset = (vaddr / page_size) * sizeof(uint64_t);
    if (lseek(pagemap_fd, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek pagemap");
        return -1;
    }
    
    if (read(pagemap_fd, entry, sizeof(*entry)) != sizeof(*entry)) {
        if (errno != 0) {
            perror("read pagemap");
        } else {
            printf("read pagemap: EOF reached, no more data.\n");
        }
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    pid_t pid = atoi(argv[1]);
    char maps_path[64], pagemap_path[64];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%d/pagemap", pid);

    FILE *maps = fopen(maps_path, "r");
    if (!maps) {
        perror("fopen maps");
        return 1;
    }

    int pagemap_fd = open(pagemap_path, O_RDONLY);
    if (pagemap_fd < 0) {
        perror("open pagemap");
        fclose(maps);
        return 1;
    }

    size_t page_size = getpagesize();
    char line[512];

    while (fgets(line, sizeof(line), maps)) {
        unsigned long start, end;
        if (sscanf(line, "%lx-%lx", &start, &end) != 2)
            continue;

        for (unsigned long addr = start; addr < end; addr += page_size) {
            uint64_t entry;
            if (read_pagemap_entry(pagemap_fd, addr, page_size, &entry) == 0) {
                int present = (entry >> 63) & 1;
                if (!present)
                    continue;

                printf("Address: 0x%lx\n", addr);
                printf("\tBits: ");
                print_bits(entry);
                decode_entry(entry);
                printf("\n");
            }
        }
    }

    fclose(maps);
    close(pagemap_fd);
    return 0;
}
