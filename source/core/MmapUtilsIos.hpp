#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <errno.h>

#define NDEBUG
#include <cassert>


#if defined(__clang__)
#define SHARED_EXPORT __attribute__((visibility("default")))
#define SHARED_LOCAL __attribute__((visibility("hidden")))
#endif

#if defined(IS_BUILDING_SHARED)
#define API SHARED_EXPORT
#else
#define API
#endif



std::string createFile(std::string fileName, size_t size);
void* openAndTruncFile(std::string filePath, size_t size);

template<typename T>
class MmapStorage{
  public:
    MmapStorage(){
      mData_ = nullptr;
      mSize_ = 0;
      mFd_ = -2;
    }

    // 
    ~MmapStorage(){
      if(mData_ != nullptr){
        printf("MmapStorage ~mmap addr:%p size %zu \n", mData_, mSize_ * sizeof(T));
        munmap(mData_, mSize_ * sizeof(T));
        if(mFd_ != -2 && mFd_ != -1){
          close(mFd_);
          mFd_ = -2;
        }
        if(mFilePath_ != ""){
          mFilePath_ = "";
        }

        mData_ = nullptr;
        mSize_ = 0;
      }
    
    }

    // size: number of element.
    int alloc(size_t size, int fd)
    {
      ftruncate(fd, size * sizeof(T));
      mData_ =static_cast<T*>(mmap(NULL, size * sizeof(T), PROT_WRITE|PROT_READ, 
            MAP_SHARED, fd, 0));

      if(mData_ == MAP_FAILED){
        printf("%s:%s:%d: Error, failed to mmap %s\n", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
        return -1;
      }
      mSize_ = size;
      mFd_ = fd;

      return 0;
    }

    // Allocate mmap meomry with implicit file.
    int alloc(size_t  size, std::string file_name="example.txt")
    {
#ifdef MNN_MMAP_IOS
      std::string filePath = createFile(file_name, size * sizeof(T));
      mData_ = static_cast<T*>(openAndTruncFile(filePath, size * sizeof(T)));
      if(mData_ == nullptr)
        return -1;
#endif

#if 0
      mData_ =static_cast<T*>(mmap(NULL, size * sizeof(T), PROT_WRITE|PROT_READ, 
            MAP_SHARED, fd, 0));

      if(mData_ == MAP_FAILED){
        printf("%s:%d:%s: Error, failed to mmap, %s\n", __FILE__, __LINE__, __FUNCTION__, strerror(errno));
        return -1;
      }
#endif

      //close(mFd_);
      mSize_ = size;
#ifdef MNN_MMAP_IOS
      mFilePath_ = filePath;
#endif
      printf("MmapStorage mmap addr:%p size %zu \n", mData_, mSize_ * sizeof(T));

      return 0;
    }

    // size: number of element.
    MmapStorage(size_t size, std::string file_name="test.txt")
    {
#ifdef MNN_MMAP_IOS
      std::string filePath = createFile(file_name, size * sizeof(T));
      mData_ = static_cast<T*>(openAndTruncFile(filePath, size * sizeof(T)));
#endif
#if 0 
      mData_ =static_cast<T*>(mmap(NULL, size * sizeof(T), PROT_WRITE|PROT_READ, 
            MAP_SHARED, mFd_, 0));

      if(mData_ == MAP_FAILED){
        printf("%s:%s:%d: Error, failed to mmap\n", __FILE__, __FUNCTION__, __LINE__);
        return;
      }
#endif
      //close(mFd_);
      mFd_ = 0;
      mSize_ = size;
#ifdef MNN_MMAP_IOS
      mFilePath_ = filePath;
#endif
      printf("%s:%d MmapStorage mmap addr:%p size %zu \n", __FILE__, __LINE__,  mData_, mSize_ * sizeof(T));
    }

    // return number of element.
    inline size_t size() const {
      return mSize_;
    }

    T *get()const{
      return mData_;
    }

    //  TBD
    int set(T* data, size_t size)
    {
      if(nullptr != mData_ && mData_ != data){
        int ret = munmap(mData_, mSize_);
        if(ret != 0){
          std::cerr<<"Failed to munmap" << std::endl;
          return -1;
        }
      }

      mData_ = data;
      mSize_ = size;

      return 0;
    }

    // 
    int reset(size_t size, std::string fileName = "test.txt")
    {
      if(size == mSize_)
        return 0;

      if(mData_ != nullptr){
        if(0 != munmap(mData_, mSize_ * sizeof(T))){
          printf("%s:%s:%d; munmap failed\n", __FILE__, __FUNCTION__, __LINE__);
          return -1;
        }
        mData_ = nullptr;
        mSize_ = 0;
        mFd_ = -2;
      }

      return alloc(size, fileName);
    }

    int release()
    {
      if(mData_ != nullptr) {
        if(0 != munmap(mData_, mSize_ * sizeof(T))){
          printf("%s:%s:%d; munmap failed\n", __FILE__, __FUNCTION__, __LINE__);
          return -1;
        }
        if(mFd_ != 0) close(mFd_);
        if(mFilePath_ != ""){
          mFilePath_ = "";
          ;
        }
        printf("MmapStorage release mmap addr:%p size %zu \n", mData_, mSize_ * sizeof(T));
        mData_ = nullptr;
        mSize_ = 0;
      }

      return 0;
    }



  private:
    T *mData_ = nullptr;
    // number of element with type T.
    size_t mSize_ = 0; 
    int mFd_ = 0; // not used
    std::string mFilePath_ = ""; // not used

};

// reduant with AutoStorage.
class BufferStorageMmap{

  public:
    BufferStorageMmap(){
    }

    // size: byte number.
    BufferStorageMmap(size_t size, size_t in_offset=0, std::string file_name="test.txt"){
      allocated_size = size + in_offset;
#ifdef MNN_MMAP_IOS
      std::string filePath = createFile(file_name, allocated_size);
      storage = static_cast<uint8_t*>(openAndTruncFile(filePath, size + allocated_size));
#endif
#if 0
      storage =(uint8_t*)(mmap(NULL, size + in_offset, PROT_WRITE|PROT_READ, 
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));

      if(storage == MAP_FAILED){
        std::cerr<<"Failed to mmap" <<std::endl;
        return;
      }
#endif
      if(storage == nullptr){
        return;
      }
      printf("MmapStorage: addr:%p, size:%zu ",storage, size);
      offset = in_offset;
    }

    ~BufferStorageMmap(){
      if(storage != nullptr){
        munmap(storage, allocated_size);
        storage = nullptr;
        offset = 0;
        allocated_size = 0;
        file_path = "";
      }
    }

    size_t size() const{
      return allocated_size - offset;
    } 

    const uint8_t *buffer()const{
      return storage + offset;
    } 


    int alloc(size_t size, size_t in_offset = 0, std::string file_name = "test.txt")
    {
      allocated_size = size + in_offset;
#ifdef MNN_MMAP_IOS
      std::string filePath = createFile(file_name, allocated_size);
      storage = static_cast<uint8_t*>(openAndTruncFile(filePath, allocated_size));
      file_path = filePath;
#endif

      if(storage == nullptr){
        return -1;
      }


      printf("BufferStorage mmap addr:%p, size:%zu ", storage, allocated_size);

      return 0;
    }


    // size: user-can-use memory size.
    // allocated_size_ = size + offset.
    int set(uint8_t* data, size_t size, size_t in_offset=0)
    {
      if(nullptr != storage ){
        int ret = munmap(storage, allocated_size);
        if(ret != 0){
          std::cerr<<"Failed to munmap" << std::endl;
          return -1;
        }
      }

      // 
      storage = (uint8_t*)data;
      allocated_size = size + in_offset;
      offset = in_offset;

      return 0;
    }

    // allocated_size = size + offset.
    size_t offset = 0;
    size_t allocated_size = 0;
    uint8_t * storage = nullptr;
    std::string file_path = "";
    int fd = 0;
};


void *MemoryAllocAlignMmap(size_t size, std::string file_name = "test.txt", size_t alignment=8);
void *MemoryCallocAlignMmap(size_t size, std::string file_name="test.txt", size_t alignment=8);
int  MemoryFreeAlignMmap(void *m);

