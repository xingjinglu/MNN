#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cassert>

#include "MmapUtilsIos.hpp"




// Mmap aligned with 4KB default.
// size: number of bytes.
// alignment: default value is 64bit.
// |---8byte-----|------------size-byte-----|
//     ^         ^
//     |         |
// tag:map-size origin
void *MemoryAllocAlignMmap(size_t size, std::string file_name, size_t alignment)
{
  assert(size > 0);
  void *origin = nullptr;
  //std::string file_name = "test.txt";
  size_t allocated_size = size + 8;

#ifdef MNN_MMAP_IOS
  std::string filePath = createFile(file_name, allocated_size);
  if(filePath == "")
    printf("%s:%d:%s: Error, failed to createFile, %s\n", __FILE__, __LINE__, __FUNCTION__, strerror(errno));
  origin = static_cast<void*>(openAndTruncFile(filePath, allocated_size));
  if(origin == nullptr){
    return nullptr;
  }
#else
  // TBD.
  origin = static_cast<void *> (mmap(NULL,  allocated_size, 
        PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));
  if(origin == MAP_FAILED){
    printf("%s:%d:%s: Error: failed to mmap, %s\n", __FILE__, __LINE__, __FUNCTION__, strerror(errno));
    return nullptr;
  }
#endif

  // Post processing.
  size_t *tag = static_cast<size_t*>(origin);
  tag[0] = allocated_size;


  //
  origin = tag + 1;
  printf("MemoryAllocAlignMmap addr: %p, size: %zu\n", origin, size);
  return origin;
}

// Mmap aligned with 4KB default.
// size: number of bytes.
// alignment: default value is 64bit.
// |---8byte-----|------------size-byte-----|
//     ^         ^
//     |         |
// tag:map-size origin
void *MemoryCallocAlignMmap(size_t size, std::string fileName, size_t alignment)
{
  assert(size > 0);
  void * origin = MemoryAllocAlignMmap(size, fileName, alignment);

  memset(origin, 0, size + alignment);
  return origin;
}


int MemoryFreeAlignMmap(void *m)
{
  // Get size of m.
  size_t *tag = static_cast<size_t *> (m); 
  size_t size = tag[-1];

  if(m != nullptr){
    if(0 != munmap(&tag[-1], size)){
      printf("%s:%d:%s: Error: failed to munmap, addr: %p, size: %zu,  %s\n", __FILE__, __LINE__, __FUNCTION__, &tag[-1], size, strerror(errno));
      return -1;
    }

    m = tag - 1; 
    m = nullptr;
  }
  printf("MemoryFreeAlignMmap size: %zu\n", size-8);

  return 0;
}



