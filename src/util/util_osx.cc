#include "src/util/util_osx.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>

#include <string>

namespace oceandoc {
namespace util {

bool UtilOsx::SetFileInvisible(std::string_view path) {
  std::string invisible_path(".");
  invisible_path.append(path);
  return std::rename(path.data(), invisible_path.data()) == 0;
}

std::string UtilOsx::Partition(std::string_view path) {
  CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOMediaClass);
  CFDictionarySetValue(matchingDict, CFSTR(kIOMediaWholeKey), kCFBooleanFalse);

  io_iterator_t iterator;
  IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iterator);

  io_object_t service;
  while ((service = IOIteratorNext(iterator)) != 0) {
    CFStringRef bsdNameRef = (CFStringRef)IORegistryEntrySearchCFProperty(
        service, kIOServicePlane, CFSTR(kIOBSDNameKey), kCFAllocatorDefault,
        kIORegistryIterateRecursively);

    if (bsdNameRef) {
      char bsdNameCStr[128];
      CFStringGetCString(bsdNameRef, bsdNameCStr, sizeof(bsdNameCStr),
                         kCFStringEncodingUTF8);

      if (bsdName == bsdNameCStr) {
        CFStringRef uuidRef = (CFStringRef)IORegistryEntryCreateCFProperty(
            service, CFSTR(kIOMediaUUIDKey), kCFAllocatorDefault, 0);

        if (uuidRef) {
          char uuidCStr[128];
          CFStringGetCString(uuidRef, uuidCStr, sizeof(uuidCStr),
                             kCFStringEncodingUTF8);
          CFRelease(uuidRef);
          IOObjectRelease(service);
          IOObjectRelease(iterator);
          return std::string(uuidCStr);
        }
      }
    }

    IOObjectRelease(service);
  }

  IOObjectRelease(iterator);
  return "Error";
}

std::string UtilOsx::PartitionUUID(std::string_view path) {
  // std::string partition = Partition(path);
  // struct udev* udev = udev_new();
  // if (!udev) return "Error";

  // struct udev_device* dev = udev_device_new_from_syspath(udev,
  // device.c_str()); if (!dev) { udev_unref(udev); return "Error";
  //}

  // const char* uuid = udev_device_get_property_value(dev,
  // "ID_PART_ENTRY_UUID"); std::string result = uuid ? uuid : "Error";

  // udev_device_unref(dev);
  // udev_unref(udev);

  // return result;
  return "";
}

}  // namespace util
}  // namespace oceandoc
