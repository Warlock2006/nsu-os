#include "storage.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("usage: ./main <sync_type> <length>");
    return EXIT_FAILURE;
  }

  int sync_choice = 0;
  sync_type_t type = SYNC_MUTEX;

  switch (sync_choice) {
    case 0: type = SYNC_MUTEX; break;
    case 1: type = SYNC_RWLOCK; break;
    case 2: type = SYNC_SPIN; break;
  }

  Storage *st = storage_create(type);
  if (st == NULL) {
    return EXIT_FAILURE;
  }

  

  
}