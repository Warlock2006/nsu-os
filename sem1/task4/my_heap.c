#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void *heap = NULL;

typedef struct Block {
  size_t size;
  char is_free;
  struct Block *next;
} Block;

#define HEAP_SIZE 1024

int my_heap_init(size_t heap_size) {
  heap = mmap(NULL, heap_size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (heap == MAP_FAILED) {
    printf("HEAP MMAP FAILE!!!!\n");
    return 1;
  }

  Block *block = (Block *)heap;
  block->size = heap_size - sizeof(Block);
  block->is_free = 1;
  block->next = NULL;

  return 0;
}

void *my_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1); // округление до sizeof(size_t)

  printf("SIZE: %d\n", size);
  
  Block *current = (Block *)heap;
  while (current != NULL) {
    if (current->is_free && current->size >= size) {
      if (current->size >= size + sizeof(Block)) {
        Block *new_block = (Block *)((char *)current + sizeof(Block) + size);
        new_block->size = current->size - size - sizeof(Block);
        new_block->is_free = 1;
        new_block->next = current->next;
        
        current->size = size;
        current->next = new_block;
      }

      current->is_free = 0;
      return (char *)current + sizeof(Block);
    }
    current = current->next;
  }
  return NULL;
}

void my_free(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  Block *block = (Block *)((char *)ptr - sizeof(Block));
  block->is_free = 1;

  Block *current = (Block *)heap;
  while (current != NULL && current->next != NULL) {
    if (current->is_free && current->next->is_free) {
      current->size += current->next->size + sizeof(Block);
      current->next = current->next->next;
    } else {
      current = current->next;
    }
  }
}

int main(int argc, char **argv) {
  if (my_heap_init(HEAP_SIZE)) {
    printf("HEAP INIT FAILED!!!!\n");
    return 1;
  }

  char *ptr = my_malloc(400);

  char *ptr2 = my_malloc(400);

  char *ptr3 = my_malloc(400);

  printf("%p\n", ptr);
  printf("%p\n", ptr2);
  printf("%p\n", ptr3);

  my_free(ptr);
  printf("%p\n", ptr);

  my_free(ptr2);

  char *ptr4 = my_malloc(900);
  printf("%p\n", ptr4);

  munmap(heap, HEAP_SIZE);
  
  return 0;
}