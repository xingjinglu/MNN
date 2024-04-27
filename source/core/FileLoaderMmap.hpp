#pragma once

#include<cstddef>
#include <vector>
#include <mutex>
#include "core/AutoStorage.h"
#ifdef MNN_MMAP
#include "core/MmapUtilsIos.hpp"
#endif

namespace MNN{

  class MNN_PUBLIC FileLoaderMmap {
    public:
      FileLoaderMmap(const char* file);

      ~FileLoaderMmap();

      bool read();
      bool read_into_oneblock(void *block);

      static bool write(const char* filePath, std::pair<const void*, size_t> cacheInfo);

      bool valid() const {
        return mFile != nullptr;
      }
      inline size_t size() const {
        return mTotalSize;
      }

      bool merge(MmapStorage<uint8_t>& buffer){

        return true;
      }

      int offset(int64_t offset);

      bool read(char* buffer, int64_t size);

      size_t get_filesize()
      {
        return mFileSize;
      }

    private:
      std::vector<std::pair<size_t, void*>> mBlocks;
      FILE* mFile                 = nullptr;
      static const int gCacheSize = 4096;
      size_t mTotalSize           = 0;
      const char* mFilePath       = nullptr;
      size_t mFileSize = 0;
  };
}
