#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#include <cassert>

// Mmap aligned with 4KB default.
// size: number of bytes.
// alignment: default value is 64bit.
void *MmapAllocAlign(size_t size, size_t alignment=8)
{
  assert(size > 0);

  void  *origin = static_cast<void *> (mmap(NULL, size + sizeof(void*), 
        PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));
  if(origin == MAP_FAILED){
    std::cerr<<"Failed to mmap" <<std::endl;
    return nullptr;
  }

  size_t *tag = static_cast<size_t*>(origin);
  tag[0] = size + sizeof(void*);

  //
  origin = tag + 1;
  return origin;
}

int MmapFree(void *m)
{
  // Get size of m.
  size_t *tag = static_cast<size_t *> (m); 
  size_t size = tag[-1];

  if(m != nullptr){
    if(0 != munmap(&tag[-1], size)){
      std::cerr<<"nunmap failed" << std::endl;
      return -1;
    }

    //std::cout<<"m = "<< m << std::endl;
    m = tag - 1; // need?
    //std::cout<<"m-- = "<< m << std::endl;
    m = nullptr;
  }

  return 0;
}



