#include <string>
#include <cerrno>


#define MNN_MMAP_IOS
#ifdef MNN_MMAP_IOS
#include <Foundation/Foundation.h>

void printAttributes(const std::string& path) {
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error;
    NSDictionary* attributes = [fileManager attributesOfItemAtPath:nsPath error:&error];
    if (attributes == nil) {
        NSLog(@"Error getting attributes: %@", error);
        return;
    }
    NSLog(@"Attributes: %@", attributes);
}

std::string getDocumentDirectory() 
{
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString* documentsDirectory = [paths objectAtIndex:0];
  return [documentsDirectory UTF8String];


#if 0
  @autoreleasepool{
    NSString* tmpDirectory = NSTemporaryDirectory();
    return [tmpDirectory UTF8String];
  }
#endif
}


bool truncateFile(const std::string& path, NSUInteger newSize) {
    NSFileHandle* fileHandle = [NSFileHandle fileHandleForUpdatingAtPath:[NSString stringWithUTF8String:path.c_str()]];
    if (fileHandle == nil) {
        NSLog(@"Error opening file");
        return NO;
    }
    [fileHandle truncateFileAtOffset:newSize];
    [fileHandle closeFile];

    return true;
}

bool deleteFile(const std::string& path) {
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error;
    BOOL success = [fileManager removeItemAtPath:nsPath error:&error];
    if (!success) {
        NSLog(@"Error deleting file: %@", error);
    }
    return true;
}

void createFileWithPermissions(std::string &path, NSNumber *permissions) {
    NSString *filePath =[NSString stringWithUTF8String:path.c_str()];
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:filePath]) {
        NSDictionary *attributes = @{NSFilePosixPermissions: permissions};
        BOOL success = [fileManager createFileAtPath:filePath contents:nil attributes:attributes];
        if (success) {
            NSLog(@"File created successfully");
        } else {
            NSLog(@"Error creating file");
        }
    } else {
        NSLog(@"File already exists");
    }
}




// 
std::string createFile(std::string fileName, size_t size)
{
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString* docDir = [paths objectAtIndex:0];
  std::string docDirStr =  [docDir UTF8String];
  std::string filePath = docDirStr + "/" + fileName;
  NSString *filePathNs = [NSString stringWithUTF8String:filePath.c_str()];

  //
  NSFileManager* fileManager = [NSFileManager defaultManager];
  if (![fileManager fileExistsAtPath:filePathNs]) {
    NSDictionary *attributes = @{NSFilePosixPermissions: @420};
    BOOL success = [fileManager createFileAtPath:filePathNs contents:nil attributes:attributes];
    if (success) {
      printf("File created successful: %s\n", filePath.c_str());
    } else {
      printf("%s:%d: Error failed to create file",__FILE__, __LINE__);
      return "";
    }
  } else {
    printf("File already exists: %s\n", filePath.c_str());
  }

  return filePath;
}

// size_t: byte number
void *openAndTruncFile(std::string filePath, size_t size)
{
  NSString *nsFilePath= [NSString stringWithUTF8String:filePath.c_str()];
  NSFileHandle* fileHandle = [NSFileHandle fileHandleForUpdatingAtPath:nsFilePath];
  if (fileHandle == nil) {
    printf("%s:%d:Error in get FileHandle, %s\n", __FILE__, __LINE__, strerror(errno));
    return nullptr;
  }

#if 1
  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSError *error;
  NSDictionary *fileAttributes = [fileManager attributesOfItemAtPath:nsFilePath error:&error];
  if (fileAttributes == nil) {
    printf("%s:%d:Error getting file attributes\n", __FILE__, __LINE__);
    return nullptr;
  }
  size_t file_size =  [fileAttributes fileSize];

  if(file_size < size){
    @try{
      [fileHandle truncateFileAtOffset:size];
    }
    @catch (NSException *exception) {
        NSLog(@"Error truncating file: %@", exception);
    }
    printf("Update file size from %ulld to %lld \n", file_size, size);
  }
#endif

  //
  int fd = [fileHandle fileDescriptor];
  if(fd == -1)
    printf("%s:%d:%s: Error, failed to get fd, %s\n", __FILE__, __LINE__, __FUNCTION__, strerror(errno));
  //else
  //  printf("%s:%d:%s: Succeed to get fd, %d\n", __FILE__, __LINE__, __FUNCTION__, fd);


  errno = 0;
  void *ptr =static_cast<void*>(mmap(NULL, size, PROT_WRITE|PROT_READ,
        MAP_SHARED, fd, 0));

  if(ptr == MAP_FAILED){
    printf("%s:%d:%s: Error, failed to mmap, %s\n", __FILE__, __LINE__, __FUNCTION__, strerror(errno));
    return nullptr;
  }
  else
    printf("%s:%d:%s: mmap, addr=%p  bytes=%zu\n", __FILE__, __LINE__, __FUNCTION__, ptr, size);

  [fileHandle closeFile];
  //printAttributes(filePath);

  return ptr;
}

int closeFileHandle()
{
    return 0;
}




#endif
