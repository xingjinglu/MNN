
#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstdio>

//#define NDEBUG
#include <cassert>

template<typename T>
class MmapStorage{
  public:
    MmapStorage(){
      mData_ = nullptr;
      mSize_ = 0;
    }

    // 
    ~MmapStorage(){
      if(mData_ != nullptr){
        std::cout<<"MmapStorage::release size = " << mSize_ * sizeof(T) << std::endl;
        munmap(mData_, mSize_ * sizeof(T));
        if(mFd_ != 0){
          close(mFd_);
          mFd_ = 0;
        }
        if(mFilePath_ != ""){
          if(remove(mFilePath_.c_str()) != 0)
            MNN_PRINT("%s:%s:%d: Error, remove file failed\n", __FILE__, __FUNCTION__, __LINE__);

          mFilePath_ = "";
        }


        mData_ = nullptr;
        mSize_ = 0;
      }
    

    }

    // size: number of element.
    int alloc(int size, int fd)
    {
      ftruncate(fd, size * sizeof(T));
      mData_ =static_cast<T*>(mmap(NULL, size * sizeof(T), PROT_WRITE|PROT_READ, 
            MAP_SHARED, fd, 0));

      if(mData_ == MAP_FAILED){
        MNN_PRINT("%s:%s:%d: Error, failed to mmap\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
      }
      mSize_ = size;
      mFd_ = fd;

      return 0;
    }

    // Allocate mmap meomry with implicit file.
    int alloc(int size)
    {
      std::string filePath = "example.txt";
      std::ofstream file(filePath);
      if(file.is_open())
        file.close();
      else
        MNN_PRINT("%s:%s:%d: Error, failed to create file\n", __FILE__, __FUNCTION__, __LINE__);

      int mFd_ = open(filePath.c_str(), O_RDWR);
      ftruncate(mFd_, size * sizeof(T));
      mData_ =static_cast<T*>(mmap(NULL, size * sizeof(T), PROT_WRITE|PROT_READ, 
            MAP_SHARED, mFd_, 0));

      if(mData_ == MAP_FAILED){
        MNN_PRINT("%s:%s:%d: Error, failed to mmap\n", __FILE__, __FUNCTION__, __LINE__);
        return -1;
      }

      std::cout<<"MmapStorage: size = " << size << std::endl;
      close(mFd_);
      mSize_ = size;
      mFilePath_ = filePath;

      return 0;
    }



    // size: number of element.
    MmapStorage(int size)
    {
      std::string filePath = "example.txt";
      std::ofstream file(filePath);
      if(file.is_open())
        file.close();
      else
        MNN_PRINT("%s:%s:%d: Error, failed to create file\n", __FILE__, __FUNCTION__, __LINE__);
      
      int mFd_ = open(filePath.c_str(), O_RDWR);
      ftruncate(mFd_, size * sizeof(T));
      mData_ =static_cast<T*>(mmap(NULL, size * sizeof(T), PROT_WRITE|PROT_READ, 
            MAP_SHARED, mFd_, 0));

      if(mData_ == MAP_FAILED){
        MNN_PRINT("%s:%s:%d: Error, failed to mmap\n", __FILE__, __FUNCTION__, __LINE__);
        return;
      }
      std::cout<<"MmapStorage: size = " << size << std::endl;
      close(mFd_);
      mFd_ = 0;
      mFilePath_ = filePath;
      mSize_ = size;
    }

    // return number of element.
    inline int size() const {
      return mSize_;
    }

    T *get()const{
      return mData_;
    }

    //  TBD
    int set(T* data, int size)
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
    int reset(int size)
    {
      if(size == mSize_)
        return 0;

      // free mmap memory.
      if(mData_ != nullptr){
        if(0 != munmap(mData_, mSize_ * sizeof(T))){
          MNN_PRINT("%s:%s:%d; munmap failed\n", __FILE__, __FUNCTION__, __LINE__);
          return -1;
        }
        mData_ = nullptr;
        mSize_ = 0;
      }

      alloc(size);
      std::cout<<"MmapStorage: size = " << size << std::endl;

      return 0;
    }

    int release()
    {
      if(mData_ != nullptr) {
        if(0 != munmap(mData_, mSize_ * sizeof(T))){
          MNN_PRINT("%s:%s:%d; munmap failed\n", __FILE__, __FUNCTION__, __LINE__);
          return -1;
        }
        if(mFd_ != 0) close(mFd_);
        if(mFilePath_ != ""){
          if(remove(mFilePath_) != 0)
            MNN_PRINT("%s:%s:%d; delete %s failed\n", __FILE__, __FUNCTION__, __LINE__, mFilePath_);
        }
        mData_ = nullptr;
        mSize_ = 0;
      }
      return 0;
    }



  private:
    T *mData_ = nullptr;
    // number of element with type T.
    int mSize_ = 0; 
    int mFd_ = 0;
    std::string mFilePath_ = "";

};

// reduant with AutoStorage.
class BufferStorageMmap{

  public:
    BufferStorageMmap(){
      storage = nullptr;
      allocated_size = 0;
      offset = 0;
    }

    BufferStorageMmap(int size, int in_offset=0){
      storage =(uint8_t*)(mmap(NULL, size + in_offset, PROT_WRITE|PROT_READ, 
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));

      if(storage == MAP_FAILED){
        std::cerr<<"Failed to mmap" <<std::endl;
        return;
      }
      std::cout<<"MmapStorage: size = " << size << std::endl;
      allocated_size = size + in_offset;
      offset = in_offset;
    }

    ~BufferStorageMmap(){
      if(storage != nullptr){
        munmap(storage, (allocated_size + offset));
        storage = nullptr;
        offset = 0;
        allocated_size = 0;
      }
    }

    //
    size_t size() const{
      return allocated_size - offset;
    } 
    const uint8_t *buffer()const{
      return storage + offset;
    } 

    int alloc(size_t size, size_t in_offset=0)
    {

      storage =(uint8_t*)(mmap(NULL, size + in_offset, PROT_WRITE|PROT_READ, 
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));
      if(storage == MAP_FAILED){
        std::cerr<<"Failed to mmap" <<std::endl;
        return -1;
      }

      std::cout<<"MmapStorage: size = " << size << std::endl;
      allocated_size = size + in_offset;
      offset = in_offset;

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
    std::string file_path = 0;
    int fd = 0;
};

void *MmapAllocAlign(size_t size, size_t alignment=8);
int MmapFree(void *m);


