#include <string>

#ifdef MNN_MMAP_IOS10
#include <Foundation/Foundation.h>

std::string getTemporaryDirectory() 
{

  //NSString* tmpDirectory = NSDocumentDirectory();
  //NSString* tmpDirectory = NSTemporaryDirectory();
  //return [tmpDirectory UTF8String];

#if 0
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString* documentsDirectory = [paths objectAtIndex:0];
  return [documentsDirectory UTF8String];
#endif


#if 1
  @autoreleasepool{
    NSString* tmpDirectory = NSTemporaryDirectory();
    return [tmpDirectory UTF8String];
  }
#endif
}
#endif
