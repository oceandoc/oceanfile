/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/util.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <random>
#include <stack>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/trim_all.hpp"
#include "boost/beast/core/detail/base64.hpp"
#include "boost/uuid/random_generator.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "crc32c/crc32c.h"
#include "fmt/core.h"
#include "glog/logging.h"
#include "google/protobuf/json/json.h"
#include "lzma.h"  // NOLINT
#include "openssl/evp.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "sodium/crypto_hash_sha256.h"
#include "src/MurmurHash2.h"
#include "src/common/defs.h"
#include "src/proto/service.pb.h"

#if defined(_WIN32)
#elif defined(__linux__)
#include <dirent.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#elif defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <sys/stat.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

using google::protobuf::json::ParseOptions;
using google::protobuf::json::PrintOptions;
using std::string;

namespace oceandoc {
namespace util {

int64_t Util::CurrentTimeMillis() {
  return absl::GetCurrentTimeNanos() / 1000000;
}

int64_t Util::CurrentTimeNanos() { return absl::GetCurrentTimeNanos(); }

int64_t Util::StrToTimeStampUTC(const string &time) {
  return Util::StrToTimeStamp(time, "%Y-%m-%d%ET%H:%M:%E3S%Ez");
}

int64_t Util::StrToTimeStampUTC(const string &time, const string &format) {
  absl::TimeZone tz = absl::UTCTimeZone();
  absl::Time t;
  string err;
  if (!absl::ParseTime(format, time, tz, &t, &err)) {
    LOG(ERROR) << err << " " << time << ", format: " << format;
    return -1;
  }
  return absl::ToUnixMillis(t);
}

int64_t Util::StrToTimeStamp(const string &time) {
  return Util::StrToTimeStamp(time, "%Y-%m-%d%ET%H:%M:%E3S%Ez");
}

int64_t Util::StrToTimeStamp(const string &time, const string &format) {
  absl::TimeZone tz = absl::LocalTimeZone();
  absl::Time t;
  string err;
  if (!absl::ParseTime(format, time, tz, &t, &err)) {
    LOG(ERROR) << err << " " << time << ", format: " << format;
    return -1;
  }
  return absl::ToUnixMillis(t);
}

int64_t Util::StrToTimeStamp(const string &time, const string &format,
                             const string &tz_str) {
  absl::TimeZone tz;
  if (!absl::LoadTimeZone(tz_str, &tz)) {
    LOG(ERROR) << "Load time zone error: " << tz_str;
  }
  absl::Time t;
  string err;
  if (!absl::ParseTime(format, time, tz, &t, &err)) {
    LOG(ERROR) << err << " " << time << ", format: " << format;
    return -1;
  }

  return absl::ToUnixMillis(t);
}

string Util::ToTimeStrUTC() {
  return Util::ToTimeStrUTC(Util::CurrentTimeMillis(),
                            "%Y-%m-%d%ET%H:%M:%E3S%Ez");
}

string Util::ToTimeStrUTC(const int64_t ts, const string &format) {
  absl::TimeZone tz = absl::UTCTimeZone();
  return absl::FormatTime(format, absl::FromUnixMillis(ts), tz);
}

string Util::ToTimeStr() {
  return Util::ToTimeStr(Util::CurrentTimeMillis(), "%Y-%m-%d%ET%H:%M:%E3S%Ez",
                         "localtime");
}

string Util::ToTimeStr(const int64_t ts) {
  return Util::ToTimeStr(ts, "%Y-%m-%d%ET%H:%M:%E3S%Ez", "localtime");
}

string Util::ToTimeStr(const int64_t ts, const string &format) {
  absl::TimeZone tz = absl::LocalTimeZone();
  return absl::FormatTime(format, absl::FromUnixMillis(ts), tz);
}

string Util::ToTimeStr(const int64_t ts, const string &format,
                       const string &tz_str) {
  absl::TimeZone tz;
  if (!absl::LoadTimeZone(tz_str, &tz)) {
    LOG(ERROR) << "Load time zone error: " << tz_str;
  }
  return absl::FormatTime(format, absl::FromUnixMillis(ts), tz);
}

struct timespec Util::ToTimeSpec(const int64_t ts) {
  struct timespec time;
  time.tv_sec = ts / 1000;
  time.tv_nsec = (ts % 1000) * 1000000;
  return time;
}

int64_t Util::Random(int64_t start, int64_t end) {
  static thread_local std::mt19937 generator(CurrentTimeNanos());
  std::uniform_int_distribution<int64_t> distribution(start, end - 1);
  return distribution(generator);
}

void Util::Sleep(int64_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void Util::UnifyDir(string *path) {
  if (path->size() > 1 && path->back() == '/') {
    path->resize(path->size() - 1);
  }
  ReplaceAll(path, string("//"), string("/"));
}

string Util::UnifyDir(const string &path) {
  string ret(path);
  UnifyDir(&ret);
  return ret;
}

bool Util::IsAbsolute(const string &src) {
  std::filesystem::path s_src(src);
  return s_src.is_absolute();
}

bool Util::SetUpdateTime(const string &path, int64_t ts) {
#if defined(_WIN32)
  HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    std::cerr << "Failed to open file.\n";
    return false;
  }

  if (!SetFileTime(hFile, NULL, NULL, &newTime)) {
    std::cerr << "Failed to set file time.\n";
    CloseHandle(hFile);
    return false;
  }

  CloseHandle(hFile);
#else
  struct timespec times[2];
  struct timespec time = ToTimeSpec(ts);
  times[0] = time;
  times[1] = time;

  int dir_fd = -1;
  std::filesystem::path s_path(path);
  if (IsAbsolute(path)) {
    dir_fd =
        open(s_path.parent_path().string().c_str(), O_RDONLY | O_DIRECTORY);
  } else {
    dir_fd = AT_FDCWD;
  }

  if (dir_fd == -1) {
    close(dir_fd);
    return false;
  }
  if (dir_fd == AT_FDCWD) {
    if (utimensat(dir_fd, path.c_str(), times, AT_SYMLINK_NOFOLLOW) != 0) {
      close(dir_fd);
      return false;
    }
  } else {
    if (utimensat(dir_fd, s_path.filename().c_str(), times,
                  AT_SYMLINK_NOFOLLOW) != 0) {
      close(dir_fd);
      return false;
    }
  }
  close(dir_fd);
#endif
  return true;
}

int64_t Util::UpdateTime(const std::string &path) {
#if defined(_WIN32)
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
    ULARGE_INTEGER ull;
    ull.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    return ((ull.QuadPart / 10000ULL) - 11644473600000ULL);
  }
#else
  struct stat attr;
  if (lstat(path.c_str(), &attr) == 0) {
#ifdef __APPLE__
    return attr.st_mtime * 1000 + attr.st_mtimespec.tv_nsec / 1000000
#else
    return attr.st_mtime * 1000 + attr.st_mtim.tv_nsec / 1000000;
#endif
  }
#endif
  return -1;
}

int64_t Util::FileSize(const std::string &path) {
#if defined(_WIN32)
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
    LARGE_INTEGER size;
    size.LowPart = fileInfo.nFileSizeLow;
    size.HighPart = fileInfo.nFileSizeHigh;
    return size.QuadPart;
  }
#else
  struct stat attr;
  if (lstat(path.c_str(), &attr) == 0) {
    return attr.st_size;
  }
#endif
  return -1;
}

bool Util::FileInfo(const std::string &path, int64_t *update_time,
                    int64_t *size) {
#if defined(_WIN32)
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
    ULARGE_INTEGER ull;
    ull.LowPart = fileInfo.ftCreationTime.dwLowDateTime;
    ull.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
    *create_time = ((ull.QuadPart / 10000ULL) - 11644473600000ULL);

    ull.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    *update_time = ((ull.QuadPart / 10000ULL) - 11644473600000ULL);

    ull.LowPart = fileInfo.nFileSizeLow;
    ull.HighPart = fileInfo.nFileSizeHigh;
    *size = ull.QuadPart;
    return true;
  }
#else
  struct stat attr;
  if (lstat(path.c_str(), &attr) == 0) {
    *size = attr.st_size;
#ifdef __linux__
    *update_time = attr.st_mtime * 1000 + attr.st_mtim.tv_nsec / 1000000;
#elif __APPLE__
    *update_time = attr.st_mtime * 1000 + attr.st_mtimespec.tv_nsec / 1000000
#endif
    return true;
  }
#endif
  return false;
}

bool Util::Exists(const string &path) {
  try {
    return std::filesystem::exists(std::filesystem::symlink_status(path));
  } catch (const std::filesystem::filesystem_error &e) {
  }
  return false;
}

bool Util::TargetExists(const string &src, const string &dst) {
  if (!std::filesystem::exists(src)) {
    return true;
  }
  if (!std::filesystem::exists(dst)) {
    return false;
  }
  return true;
}

bool Util::Mkdir(const string &path) {
  try {
    if (!Exists(path)) {
      return std::filesystem::create_directories(path);
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Mkdir error: " << path << ", " << e.what();
    return false;
  }
  return true;
}

bool Util::MkParentDir(const string &path) {
  try {
    std::filesystem::path s_path(path);
    if (!s_path.has_parent_path()) {
      return false;
    }
    return Mkdir(s_path.parent_path().string());
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Error: " << path << ", e: " << e.what();
    return false;
  }
  return true;
}

bool Util::Remove(const string &path) {
  try {
    if (Exists(path)) {
      return std::filesystem::remove_all(path);
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Remove error: " << path << ", " << e.what();
    return false;
  }

  return true;
}

bool Util::Create(const string &path) {
  if (path.empty()) {
    LOG(ERROR) << "Empty path";
    return false;
  }

  if (path.size() > 1 && path.back() == '/') {
    LOG(ERROR) << "Create only support file: " << path;
    return false;
  }

  if (!Exists(path)) {
    if (!MkParentDir(path)) {
      LOG(ERROR) << "Mk parent dir error";
      return false;
    }
  }

  try {
    if (Exists(path)) {
      return true;
    }

#if defined(_WIN32)
    HANDLE hFile = CreateFile(path.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
      return false;
    }
    CloseHandle(hFile);
#else
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd == -1) {
      LOG(ERROR) << "Create file error: " << path;
      return false;
    }
    close(fd);
#endif
    return true;
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Mkdir error: " << path << ", " << e.what();
    return false;
  }
  return true;
}

bool Util::Rename(const std::string &src, const std::string &dst) {
  if (!std::filesystem::exists(src)) {
    return false;
  }
  try {
    std::filesystem::rename(src, dst);
  } catch (const std::filesystem::filesystem_error &e) {
  }
  return false;
}

proto::ErrCode Util::CreateFileWithSize(const std::string &path,
                                        const int64_t size) {
  if (std::filesystem::exists(std::filesystem::symlink_status(path))) {
    return proto::ErrCode::Success;
  }

#if defined(_WIN32)
  HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE) {
    DWORD errorCode = GetLastError();
    if (errorCode == ERROR_ACCESS_DENIED) {
      return proto::ErrCode::Permission;
    } else if (errorCode == ERROR_DISK_FULL) {
      return proto::ErrCode::Disk_full;
    }
    return proto::ErrCode::Fail;
  }

  LARGE_INTEGER liSize;
  liSize.QuadPart = size;

  if (!SetFilePointerEx(hFile, liSize, nullptr, FILE_BEGIN) ||
      !SetEndOfFile(hFile)) {
    CloseHandle(hFile);
    return proto::ErrCode::Fail;
  }

  CloseHandle(hFile);
#else
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
  if (fd == -1) {
    if (errno == EACCES) {
      return proto::ErrCode::File_permission;
    } else if (errno == ENOSPC) {
      return proto::ErrCode::File_disk_full;
    }
    return proto::ErrCode::Fail;
  }

  if (ftruncate(fd, size) == -1) {
    close(fd);
    return proto::ErrCode::File_size_set_error;
  }
  close(fd);
#endif
  return proto::ErrCode::Success;
}

bool Util::CreateSymlink(const string &src, const string &target) {
  std::error_code ec;
  std::filesystem::create_symlink(target, src, ec);
  if (ec) {
    LOG(ERROR) << "Create symlink to " << target << " error";
    return false;
  }
  return true;
}

string Util::FindCommonRoot(const std::filesystem::path &path,
                            const std::filesystem::path &base) {
  std::filesystem::path t(base);
  do {
    if (Util::StartWith(path.string(), t.string())) {
      return t.string();
    }
    if (t.string() == "/") {
      return "";
    } else if (t.has_parent_path()) {
      t = t.parent_path();
    } else {
      return "";
    }
  } while (true);
}

bool Util::Relative(const string &path, const string &base, string *relative) {
  relative->clear();
  const string u_path = UnifyDir(path);
  const string u_base = UnifyDir(base);

  auto s_path = std::filesystem::path(u_path);
  auto s_base = std::filesystem::path(u_base);

  std::string common_parent;
  try {
    common_parent = FindCommonRoot(s_path, s_base);
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << e.what();
  }

  if (common_parent.empty()) {
    LOG(ERROR) << "cannot calc relative between " << path << " and " << base;
    return false;
  }

  auto t = s_base;
  while (common_parent != t.string()) {
    relative->append("../");
    t = t.parent_path();
  }

  if (u_path.size() > common_parent.size()) {
    relative->append(u_path.substr(common_parent.size() + 1));
  }
  // LOG(INFO) << *relative;
  UnifyDir(relative);
  return true;
}

string Util::ParentPath(const std::string &path) {
  std::string parent;
  std::filesystem::path s_path(path);
  if (s_path.has_parent_path()) {
    return s_path.parent_path().string();
  }
  return "";
}

bool Util::CopyFile(const string &src, const string &dst,
                    const std::filesystem::copy_options opt) {
  try {
    return std::filesystem::copy_file(src, dst, opt);
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "CopyFile error: " << src << " to " << dst << ", "
               << e.what();
  }
  FDCount();
  std::exit(0);
  return false;
}

bool Util::Copy(const string &src, const string &dst) {
  try {
    std::filesystem::copy(src, dst, std::filesystem::copy_options::recursive);
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Copy error: " << src << " to " << dst << ", " << e.what();
    return false;
  }
  return true;
}

bool Util::TruncateFile(const string &path) {
  try {
    if (!std::filesystem::exists(path)) {
      return true;
    }
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs) {
      return false;
    } else {
      return true;
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "TruncateFile error: " << path << ", " << e.what();
    return false;
  }
  return true;
}

proto::ErrCode Util::WriteToFile(const string &path, const string &content,
                                 const bool append) {
  try {
    if (!std::filesystem::exists(path)) {
      std::filesystem::path s_path(path);
      if (s_path.has_parent_path()) {
        std::filesystem::create_directories(s_path.parent_path());
      }
    }

    std::ofstream ofs(path, (append ? std::ios::app : std::ios::trunc) |
                                std::ios::out | std::ios::binary);
    if (ofs && ofs.is_open()) {
      ofs << content;
      ofs.close();
      return proto::ErrCode::Success;
    } else {
      if (errno == EACCES) {
        return proto::ErrCode::File_permission;
      } else if (errno == ENOSPC) {
        return proto::ErrCode::File_disk_full;
      }
      LOG(INFO) << std::strerror(errno);
      return proto::ErrCode::Fail;
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << (!append ? "Write to " : "Append to ") << path
               << ", error: " << e.what();
  }
  return proto::ErrCode::Fail;
}

proto::ErrCode Util::WriteToFile(const string &path, const string &content,
                                 const int64_t start) {
  try {
    std::ofstream ofs(path, std::ios::binary | std::ios::out | std::ios::in);
    if (ofs && ofs.is_open()) {
      ofs.seekp(start);
      if (ofs.fail()) {
        if (errno == EACCES) {
          return proto::ErrCode::File_permission;
        } else if (errno == ENOSPC) {
          return proto::ErrCode::File_disk_full;
        }
        return proto::ErrCode::Fail;
      }
      ofs.write(content.data(), content.size());
      if (ofs.fail()) {
        if (errno == EACCES) {
          return proto::ErrCode::File_permission;
        } else if (errno == ENOSPC) {
          return proto::ErrCode::File_disk_full;
        }
        return proto::ErrCode::Fail;
      }
      ofs.close();
      return proto::ErrCode::Success;
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Write to " << path << ", error: " << e.what();
  }
  return proto::ErrCode::Fail;
}

bool Util::LoadSmallFile(const std::string &path, string *content) {
  std::ifstream in(path, std::ios::binary);
  if (!in || !in.is_open()) {
    LOG(ERROR) << "Fail to open " << path
               << ", please check file exists and file permission";
    return false;
  }

  in.seekg(0, std::ios::end);
  content->reserve(in.tellg());
  in.seekg(0, std::ios::beg);

  std::copy((std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>(), std::back_inserter(*content));
  in.close();
  return true;
}

bool Util::SyncSymlink(const std::string &src, const std::string &dst,
                       const std::string &src_symlink) {
  try {
    if (!Util::StartWith(src_symlink, src)) {
      LOG(ERROR) << src_symlink << " must start with " << src;
      return false;
    }

    if (std::filesystem::is_symlink(src) || std::filesystem::is_symlink(dst)) {
      LOG(ERROR) << "src and dst cannot be symlink";
      return false;
    }

    if (!std::filesystem::is_symlink(src_symlink)) {
      LOG(ERROR) << "src_symlink must be symlink";
      return false;
    }

    auto target = std::filesystem::read_symlink(src_symlink);

    std::string src_symlink_relative_path;
    Util::Relative(src_symlink, src, &src_symlink_relative_path);

    auto dst_symlink = dst;
    if (!src_symlink_relative_path.empty()) {
      dst_symlink = dst + "/" + src_symlink_relative_path;
    }

    Util::MkParentDir(dst_symlink);
    Util::Remove(dst_symlink);
    std::filesystem::create_symlink(target, dst_symlink);
    return true;
  } catch (std::filesystem::filesystem_error &e) {
    LOG(ERROR) << e.what();
  }
  return true;
}

int32_t Util::FilePartitionNum(const std::string &path) {
  auto ret = FileSize(path);
  if (ret == -1) {
    return -1;
  }
  return FilePartitionNum(ret);
}

int32_t Util::FilePartitionNum(const int64_t size) {
  // file size unit is Bytes
  return size / common::BUFFER_SIZE_BYTES +
         ((size % common::BUFFER_SIZE_BYTES) > 0 ? 1 : 0);
}

int32_t Util::FilePartitionNum(const std::string &path,
                               const int64_t partition_size) {
  auto ret = FileSize(path);
  if (ret == -1) {
    return -1;
  }
  return FilePartitionNum(ret, partition_size);
}

int32_t Util::FilePartitionNum(const int64_t total_size,
                               const int64_t partition_size) {
  // file size unit is Bytes
  return total_size / partition_size +
         ((total_size % partition_size) > 0 ? 1 : 0);
}

bool Util::PrepareFile(const string &path, common::FileAttr *attr) {
  attr->path = path;
  if (!FileSHA256(path, &attr->sha256)) {
    LOG(ERROR) << "Calc " << path << " sha256 error";
    return false;
  }

  attr->size = FileSize(path);
  if (attr->size == -1) {
    LOG(ERROR) << "Get " << path << " size error";
    return false;
  }

  attr->partition_num = FilePartitionNum(attr->size);
  return true;
}

bool Util::SimplifyPath(const string &path, string *out) {
  std::stack<std::string> dirs;
  std::stringstream ss(path);
  std::string token;
  while (std::getline(ss, token, '/')) {
    if (token == "..") {
      if (!dirs.empty()) {
        dirs.pop();
      } else {
        return false;
      }
    } else if (!token.empty() && token != ".") {
      dirs.push(token);
    }
  }

  while (!dirs.empty()) {
    *out = dirs.top() + "/" + *out;
    dirs.pop();
  }

  if (!path.empty() && path[0] == '/') {
    *out = "/" + *out;
  }

  UnifyDir(out);
  return true;
}

std::string Util::RepoFilePath(const std::string &repo_path,
                               const std::string &sha256) {
  std::string repo_file_path(UnifyDir(repo_path));
  repo_file_path.append("/");
  repo_file_path.append(sha256.substr(0, 2));
  repo_file_path.append("/");
  repo_file_path.append(sha256.substr(2, 2));
  repo_file_path.append("/");
  repo_file_path.append(sha256);
  return repo_file_path;
}

void Util::CalcPartitionStart(const int64_t size, const int32_t partition,
                              const int64_t partition_size, int64_t *start,
                              int64_t *end) {
  *start = partition * partition_size;
  *end = *start + partition_size - 1;
  if (*end > size) {
    *end = size - 1;
    return;
  }
}

string Util::ToUpper(const string &str) {
  string ret = str;
  transform(ret.begin(), ret.end(), ret.begin(),
            [](unsigned char c) { return toupper(c); });
  // transform(ret.begin(), ret.end(), ret.begin(), ::toupper);
  return ret;
}

string Util::ToLower(const string &str) {
  string ret = str;
  transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
  return ret;
}

void Util::ToLower(string *str) {
  transform(str->begin(), str->end(), str->begin(), ::tolower);
}

void Util::Trim(string *str) {
  boost::algorithm::trim_right(*str);
  boost::algorithm::trim_left(*str);
}

string Util::Trim(const string &str) {
  string trimmed_str = str;
  boost::algorithm::trim_right(trimmed_str);
  boost::algorithm::trim_left(trimmed_str);
  return trimmed_str;
}

bool Util::StartWith(const string &str, const string &prefix) {
  return boost::starts_with(str, prefix);
}

bool Util::EndWith(const string &str, const string &postfix) {
  return boost::ends_with(str, postfix);
}

void Util::Split(const string &str, const string &delim,
                 std::vector<string> *result, bool trim_empty) {
  result->clear();
  if (str.empty()) {
    return;
  }
  if (trim_empty) {
    string trimed_str = boost::algorithm::trim_all_copy(str);
    boost::split(*result, trimed_str, boost::is_any_of(delim));
    return;
  }
  boost::split(*result, str, boost::is_any_of(delim));
}

std::string Util::UUID() {
  boost::uuids::random_generator generator;
  return boost::uuids::to_string(generator());
}

string Util::ToHexStr(const uint64_t in, bool use_upper_case) {
  if (use_upper_case) {
    return fmt::format("{:016X}", in);
  } else {
    return fmt::format("{:016x}", in);
  }
}

void Util::ToHexStr(const string &in, std::string *out, bool use_upper_case) {
  out->clear();
  out->reserve(in.size() * 2);
  for (std::size_t i = 0; i < in.size(); ++i) {
    if (use_upper_case) {
      out->append(fmt::format("{:02X}", (unsigned char)in[i]));
    } else {
      out->append(fmt::format("{:02x}", (unsigned char)in[i]));
    }
  }
}

string Util::ToHexStr(const string &in, bool use_upper_case) {
  string out;
  out.reserve(in.size() * 2);
  for (std::size_t i = 0; i < in.size(); ++i) {
    if (use_upper_case) {
      out.append(fmt::format("{:02X}", (unsigned char)in[i]));
    } else {
      out.append(fmt::format("{:02x}", (unsigned char)in[i]));
    }
  }
  return out;
}

bool Util::HexStrToInt64(const string &in, int64_t *out) {
  *out = 0;
  auto result = std::from_chars(in.data(), in.data() + in.size(), *out, 16);
  if (result.ec == std::errc()) {
    return false;
  }
  return true;
}

uint32_t Util::CRC32(const string &content) { return crc32c::Crc32c(content); }

void Util::Base64Encode(const string &input, string *out) {
  out->resize(boost::beast::detail::base64::encoded_size(input.size()));
  auto const ret = boost::beast::detail::base64::encode(
      out->data(), input.data(), input.size());
  out->resize(ret);
}

string Util::Base64Encode(const string &input) {
  string out;
  Base64Encode(input, &out);
  return out;
}

void Util::Base64Decode(const string &input, string *out) {
  out->resize(boost::beast::detail::base64::decoded_size(input.size()));
  auto const ret = boost::beast::detail::base64::decode(
      out->data(), input.data(), input.size());
  out->resize(ret.first);
  return;
}

string Util::Base64Decode(const string &input) {
  string out;
  Base64Decode(input, &out);
  return out;
}

int64_t Util::MurmurHash64A(const string &str) {
  return ::MurmurHash64A(str.data(), str.size(), 42L);
}

EVP_MD_CTX *Util::HashInit(const EVP_MD *type) {
  EVP_MD_CTX *context = EVP_MD_CTX_new();
  if (!context) {
    return nullptr;
  }
  if (EVP_DigestInit_ex(context, type, nullptr) != 1) {
    EVP_MD_CTX_free(context);
    return nullptr;
  }
  return context;
}

bool Util::HashUpdate(EVP_MD_CTX *context, const string &str) {
  if (EVP_DigestUpdate(context, str.data(), str.size()) != 1) {
    EVP_MD_CTX_free(context);
    return false;
  }
  return true;
}

bool Util::HashFinal(EVP_MD_CTX *context, string *out, bool use_upper_case) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length;
  if (EVP_DigestFinal_ex(context, hash, &length) != 1) {
    EVP_MD_CTX_free(context);
    return false;
  }

  EVP_MD_CTX_free(context);

  string s(reinterpret_cast<const char *>(hash), length);
  Util::ToHexStr(s, out, use_upper_case);
  return true;
}

EVP_MD_CTX *Util::SHA256Init() { return HashInit(EVP_sha256()); }

bool Util::SHA256Update(EVP_MD_CTX *context, const string &str) {
  return HashUpdate(context, str);
}

bool Util::SHA256Final(EVP_MD_CTX *context, string *out, bool use_upper_case) {
  return HashFinal(context, out, use_upper_case);
}

bool Util::Hash(const string &str, const EVP_MD *type, string *out,
                bool use_upper_case) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length;

  EVP_MD_CTX *context = EVP_MD_CTX_new();
  if (!context) {
    return false;
  }
  if (EVP_DigestInit_ex(context, type, nullptr) != 1 ||
      EVP_DigestUpdate(context, str.data(), str.size()) != 1 ||
      EVP_DigestFinal_ex(context, hash, &length) != 1) {
    EVP_MD_CTX_free(context);
    return false;
  }

  EVP_MD_CTX_free(context);

  string s(reinterpret_cast<const char *>(hash), length);
  Util::ToHexStr(s, out, use_upper_case);
  return true;
}

bool Util::FileHash(const std::string &path, const EVP_MD *type,
                    std::string *out, bool use_upper_case) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length;

  std::ifstream file(path);
  if (!file || !file.is_open()) {
    LOG(ERROR) << "Check file exists or file permissions";
    return false;
  }

  EVP_MD_CTX *context = EVP_MD_CTX_new();
  if (context == nullptr) {
    return false;
  }

  if (EVP_DigestInit_ex(context, type, nullptr) != 1) {
    EVP_MD_CTX_free(context);
    return false;
  }

  std::vector<char> buffer(common::BUFFER_SIZE_BYTES);

  while (file.read(buffer.data(), common::BUFFER_SIZE_BYTES) ||
         file.gcount() > 0) {
    if (EVP_DigestUpdate(context, buffer.data(), file.gcount()) != 1) {
      EVP_MD_CTX_free(context);
      return false;
    }
  }

  if (EVP_DigestFinal_ex(context, hash, &length) != 1) {
    EVP_MD_CTX_free(context);
    return false;
  }

  EVP_MD_CTX_free(context);

  string s(reinterpret_cast<const char *>(hash), length);
  Util::ToHexStr(s, out, use_upper_case);
  return true;
}

bool Util::SmallFileHash(const std::string &path, const EVP_MD *type,
                         std::string *out, bool use_upper_case) {
  string str;
  if (!Util::LoadSmallFile(path, &str)) {
    return false;
  }
  return Hash(str, type, out, use_upper_case);
}

bool Util::MD5(const string &str, string *out, bool use_upper_case) {
  return Hash(str, EVP_md5(), out, use_upper_case);
}

bool Util::SmallFileMD5(const string &path, string *out, bool use_upper_case) {
  return SmallFileHash(path, EVP_md5(), out, use_upper_case);
}

bool Util::FileMD5(const std::string &path, string *out, bool use_upper_case) {
  return Util::FileHash(path, EVP_md5(), out, use_upper_case);
}

bool Util::SHA256(const string &str, string *out, bool use_upper_case) {
  return Hash(str, EVP_sha256(), out, use_upper_case);
}

string Util::SHA256(const string &str, bool use_upper_case) {
  string out;
  Hash(str, EVP_sha256(), &out, use_upper_case);
  return out;
}

bool Util::SHA256_libsodium(const string &str, string *out,
                            bool use_upper_case) {
  unsigned char hash[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(hash, reinterpret_cast<const unsigned char *>(str.data()),
                     str.size());

  string s(reinterpret_cast<const char *>(hash), crypto_hash_sha256_BYTES);
  Util::ToHexStr(s, out, use_upper_case);
  return true;
}

bool Util::SmallFileSHA256(const string &path, string *out,
                           bool use_upper_case) {
  return SmallFileHash(path, EVP_sha256(), out, use_upper_case);
}

bool Util::FileSHA256(const std::string &path, string *out,
                      bool use_upper_case) {
  return FileHash(path, EVP_sha256(), out, use_upper_case);
}

bool Util::LZMACompress(const string &data, string *out) {
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret ret =
      lzma_easy_encoder(&strm, LZMA_PRESET_DEFAULT, LZMA_CHECK_CRC64);

  if (ret != LZMA_OK) {
    return false;
  }

  out->clear();
  out->resize(data.size() + data.size() / 3 + 128);

  strm.next_in = reinterpret_cast<const uint8_t *>(data.data());
  strm.avail_in = data.size();
  strm.next_out = reinterpret_cast<uint8_t *>(out->data());
  strm.avail_out = out->size();

  ret = lzma_code(&strm, LZMA_FINISH);

  if (ret != LZMA_STREAM_END) {
    lzma_end(&strm);
    return false;
  }

  out->resize(out->size() - strm.avail_out);
  lzma_end(&strm);
  return true;
}

bool Util::LZMADecompress(const string &data, string *out) {
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);

  if (ret != LZMA_OK) {
    return false;
  }

  std::vector<uint8_t> decompressed_data(common::BUFFER_SIZE_BYTES);

  strm.next_in = reinterpret_cast<const uint8_t *>(data.data());
  strm.avail_in = data.size();
  strm.next_out = decompressed_data.data();
  strm.avail_out = decompressed_data.size();

  do {
    ret = lzma_code(&strm, LZMA_FINISH);
    if (strm.avail_out == 0 || ret == LZMA_STREAM_END) {
      out->append(reinterpret_cast<const char *>(decompressed_data.data()),
                  common::BUFFER_SIZE_BYTES - strm.avail_out);
      strm.next_out = decompressed_data.data();
      strm.avail_out = common::BUFFER_SIZE_BYTES;
    }

    if (ret != LZMA_OK) {
      break;
    }
  } while (true);

  if (ret != LZMA_STREAM_END) {
    lzma_end(&strm);
    return false;
  }

  lzma_end(&strm);
  return true;
}

void Util::PrintProtoMessage(const google::protobuf::Message &msg) {
  static PrintOptions option = {false, true, true, true, true};
  string json_value;
  if (!MessageToJsonString(msg, &json_value, option).ok()) {
    LOG(ERROR) << "to json string failed";
  }
  LOG(INFO) << "json_value: " << json_value;
}

bool Util::MessageToJson(const google::protobuf::Message &msg, string *json) {
  static PrintOptions option = {false, true, true, true, true};
  if (!MessageToJsonString(msg, json, option).ok()) {
    return false;
  }
  return true;
}

bool Util::MessageToPrettyJson(const google::protobuf::Message &msg,
                               string *json) {
  static PrintOptions option = {true, true, false, true, true};
  if (!MessageToJsonString(msg, json, option).ok()) {
    return false;
  }
  return true;
}

bool Util::JsonToMessage(const std::string &json,
                         google::protobuf::Message *msg) {
  static ParseOptions option = {true, false};
  if (!JsonStringToMessage(json, msg, option).ok()) {
    return false;
  }
  return true;
}

void Util::PrintFileReq(const proto::FileReq &req) {
  LOG(INFO) << "request_id: " << req.request_id() << ", op: " << req.op()
            << ", path: " << req.path() << ", sha256: " << req.sha256()
            << ", size: " << req.size()
            << ", partition_num: " << req.partition_num()
            << ", repo_uuid: " << req.repo_uuid()
            << ", partition_size: " << req.partition_size()
            << ", content size: " << req.content().size();
}

bool Util::FileReqToJson(const proto::FileReq &req, std::string *json) {
  // PrintFileReq(req);
  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

  // Add Protobuf fields to the JSON document
  document.AddMember(
      "request_id",
      rapidjson::Value().SetString(req.request_id().c_str(), allocator),
      allocator);
  document.AddMember(
      "path", rapidjson::Value().SetString(req.path().c_str(), allocator),
      allocator);
  document.AddMember(
      "sha256", rapidjson::Value().SetString(req.sha256().c_str(), allocator),
      allocator);

  std::string base64_content;
  Base64Encode(req.content(), &base64_content);
  document.AddMember(
      "content",
      rapidjson::Value().SetString(base64_content.c_str(), allocator),
      allocator);

  document.AddMember(
      "repo_uuid",
      rapidjson::Value().SetString(req.repo_uuid().c_str(), allocator),
      allocator);

  document.AddMember("op", req.op(), allocator);
  document.AddMember("size", req.size(), allocator);
  document.AddMember("partition_num", req.partition_num(), allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  document.Accept(writer);

  *json = buffer.GetString();
  return true;
}

bool Util::JsonToFileReq(const std::string &json, proto::FileReq *req) {
  rapidjson::Document doc;
  if (doc.Parse(json).HasParseError()) {
    LOG(ERROR) << "Parse error";
    return 1;
  }

  if (doc.HasMember("request_id") && doc["request_id"].IsString()) {
    req->set_request_id(doc["request_id"].GetString());
  }

  if (doc.HasMember("op") && doc["op"].IsInt()) {
    req->set_op(proto::FileOp(doc["op"].GetInt()));
  }

  if (doc.HasMember("path") && doc["path"].IsString()) {
    req->set_path(doc["path"].GetString());
  }

  if (doc.HasMember("sha256") && doc["sha256"].IsString()) {
    req->set_sha256(doc["sha256"].GetString());
  }

  if (doc.HasMember("size") && doc["size"].IsInt64()) {
    req->set_size(doc["size"].GetInt64());
  }

  if (doc.HasMember("content") && doc["content"].IsString()) {
    std::string base64_content;
    Base64Decode(doc["content"].GetString(), &base64_content);
    req->set_content(std::move(base64_content));
  }

  if (doc.HasMember("partition_num") && doc["partition_num"].IsInt()) {
    req->set_partition_num(doc["partition_num"].GetInt());
  }

  if (doc.HasMember("partition_size") && doc["partition_size"].IsInt()) {
    req->set_partition_size(doc["partition_size"].GetInt64());
  }

  if (doc.HasMember("repo_uuid") && doc["repo_uuid"].IsString()) {
    req->set_repo_uuid(doc["repo_uuid"].GetString());
  }
  // PrintFileReq(*req);

  return true;
}

int64_t Util::FDCount() {
  int fd_count = 0;
#if defined(__linux__)

  struct rlimit limit;
  getrlimit(RLIMIT_NOFILE, &limit);

  const char *fd_dir = "/proc/self/fd";
  DIR *dir = opendir(fd_dir);
  if (dir == nullptr) {
    limit.rlim_cur += 10000;
    setrlimit(RLIMIT_NOFILE, &limit);

    dir = opendir(fd_dir);
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      if (entry->d_name[0] == '.') continue;
      fd_count++;
      std::string fd_path = std::string(fd_dir) + "/" + entry->d_name;
      char link_target[256];
      ssize_t len =
          readlink(fd_path.c_str(), link_target, sizeof(link_target) - 1);
      if (len != -1) {
        link_target[len] = '\0';
        LOG(INFO) << link_target;
      } else {
        perror("Failed to read link");
      }
    }
    LOG(INFO) << fd_count << ", soft limit: " << limit.rlim_cur << ", "
              << limit.rlim_max;

    perror("Could not open /proc/self/fd");
    return -1;
  }

  closedir(dir);
#endif
  return fd_count;
}

}  // namespace util
}  // namespace oceandoc
