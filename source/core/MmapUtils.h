
#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

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
        mData_ = nullptr;
        mSize_ = 0;
      }

    }

    // size: number of element.
    MmapStorage(int size)
    {
      mData_ =static_cast<T*>(mmap(NULL, size * sizeof(T), PROT_WRITE|PROT_READ, 
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));

      if(mData_ == MAP_FAILED){
        std::cerr<<"Failed to mmap" <<std::endl;
        return;
      }
      std::cout<<"MmapStorage: size = " << size << std::endl;
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

      if(mData_ != nullptr){
        if(0 != munmap(mData_, mSize_ * sizeof(T))){
          MNN_PRINT("%s:%s:%d; munmap failed\n", __FILE__, __FUNCTION__, __LINE__);
          return -1;
        }
        mData_ = nullptr;
        mSize_ = 0;
      }

      mData_ =static_cast<T*>(mmap(NULL, size * sizeof(T), PROT_WRITE|PROT_READ, 
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));
      if(mData_ == MAP_FAILED){
        std::cerr<<"Failed to mmap" <<std::endl;
        return -1;
      }

      std::cout<<"MmapStorage: size = " << size << std::endl;
      mSize_ = size;

      return 0;
    }

    int release()
    {
      if(mData_ != nullptr) {
        if(0 != munmap(mData_, mSize_ * sizeof(T))){
          std::cerr<<"nunmap failed" << std::endl;
          return -1;
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
};

void *MmapAllocAlign(size_t size, size_t alignment=8);
int MmapFree(void *m);


