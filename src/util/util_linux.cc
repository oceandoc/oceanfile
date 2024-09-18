#include "src/util/util_linux.h"

#include <mntent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

namespace oceandoc {
namespace util {

bool UtilLinux::SetFileInvisible(std::string_view path) {
  std::string invisible_path(".");
  invisible_path.append(path);
  return std::rename(path.data(), invisible_path.data()) == 0;
}

std::string UtilLinux::Partition(std::string_view path) {
  std::string partition;
  struct stat attr;
  if (stat(path.data(), &attr) != 0) {
    return partition;
  }

  FILE* mount_table = setmntent("/etc/mtab", "r");
  if (!mount_table) {
    return partition;
  }

  struct mntent* mount_entry;
  while ((mount_entry = getmntent(mount_table)) != nullptr) {
    struct stat mount_attr;
    if (stat(mount_entry->mnt_dir, &mount_attr) == 0) {
      if (attr.st_dev == mount_attr.st_dev) {
        partition = mount_entry->mnt_fsname;
        break;
      }
    }
  }

  endmntent(mount_table);
  return partition;
}

std::string UtilLinux::PartitionUUID(std::string_view path) {
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
