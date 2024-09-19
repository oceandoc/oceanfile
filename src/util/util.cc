/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/util.h"

#include <algorithm>
#include <charconv>
#include <exception>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include "absl/time/clock.h"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/trim_all.hpp"
#include "boost/beast/core/detail/base64.hpp"
#include "boost/iostreams/device/mapped_file.hpp"
#include "boost/uuid/random_generator.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "crc32c/crc32c.h"
#include "fmt/core.h"
#include "glog/logging.h"
#include "google/protobuf/util/json_util.h"
#include "openssl/evp.h"
#include "openssl/md5.h"
#include "sodium/crypto_hash_sha256.h"
#include "src/MurmurHash2.h"
#include "src/MurmurHash3.h"

#if defined(_WIN32)
#include "src/util/util_windows.h"
#elif defined(__linux__)
#include "src/util/util_linux.h"
#elif defined(__APPLE__)
#include "src/util/util_osx.h"
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <sys/stat.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
// #include "src/common/ip_address.h"

using absl::FormatTime;
using absl::FromUnixMillis;
using absl::GetCurrentTimeNanos;
using absl::LoadTimeZone;
using absl::Milliseconds;
using absl::SleepFor;
using absl::Time;
using absl::TimeZone;
using google::protobuf::util::JsonParseOptions;
using google::protobuf::util::JsonPrintOptions;
using std::string;
using std::string_view;

namespace oceandoc {
namespace util {

const char *Util::kPathDelimeter = "/";

string Util::GetServerIp() {
  // common::IPAddress ip_address;
  // if (!common::IPAddress::GetFirstPrivateAddress(&ip_address)) {
  // LOG(ERROR) << "Failed to get local ip address";
  // return "";
  //}
  // return ip_address.ToString();
  return "";
}

int64_t Util::CurrentTimeMillis() {
  return absl::GetCurrentTimeNanos() / 1000000;
}

int64_t Util::CurrentTimeNanos() { return GetCurrentTimeNanos(); }

int64_t Util::StrToTimeStamp(string_view time) {
  return Util::StrToTimeStamp(time, "%Y-%m-%d%ET%H:%M:%E3S%Ez");
}

int64_t Util::StrToTimeStamp(string_view time, string_view format) {
  absl::Time t;
  string err;
  if (!absl::ParseTime(format, time, &t, &err)) {
    LOG(ERROR) << "convert " << time << " " << err;
    return -1;
  }
  return absl::ToUnixMillis(t);
}

string Util::ToTimeStr(const int64_t ts) {
  return Util::ToTimeStr(ts, "Asia/Shanghai", "%Y-%m-%d%ET%H:%M:%E3S%Ez");
}

string Util::ToTimeStr(const int64_t ts, string_view format) {
  TimeZone time_zone;
  LoadTimeZone("Asia/Shanghai", &time_zone);
  return FormatTime(format, FromUnixMillis(ts), time_zone);
}

string Util::ToTimeStr(const int64_t ts, string_view format, string_view tz) {
  TimeZone time_zone;
  LoadTimeZone(tz, &time_zone);
  return FormatTime(format, FromUnixMillis(ts), time_zone);
}

int64_t Util::Random(int64_t start, int64_t end) {
  static thread_local std::mt19937 generator(CurrentTimeNanos());
  std::uniform_int_distribution<int64_t> distribution(start, end - 1);
  return distribution(generator);
}

void Util::Sleep(int64_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void Util::SleepUntil(const Time &time) {
  volatile bool signal = false;
  SleepUntil(time, &signal);
}

bool Util::SleepUntil(const Time &time, volatile bool *stop_signal) {
  auto now = absl::Now();
  if (time <= now) {
    return true;
  }
  while (absl::Now() < time) {
    if (*stop_signal) {
      return false;
    }
    SleepFor(Milliseconds(100));
  }
  return true;
}

void Util::UnifyDir(string *path) {
  if (path->size() > 1 && path->back() == '/') {
    path->resize(path->size() - 1);
  }
}

string Util::UnifyDir(string_view path) {
  if (path.size() > 1 && path.back() == '/') {
    return string(path.substr(0, path.size() - 1));
  }
  return string(path);
}

bool Util::Remove(string_view path) {
  try {
    if (std::filesystem::exists(path)) {
      return std::filesystem::remove_all(path);
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Remove error: " << path << ", " << e.what();
    return false;
  }

  return true;
}

bool Util::Mkdir(string_view path) {
  try {
    std::filesystem::create_directories(path);
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Mkdir error: " << path << ", " << e.what();
    return false;
  }
  return true;
}

bool Util::CopyFile(string_view src, string_view dst,
                    const std::filesystem::copy_options opt) {
  try {
    return std::filesystem::copy_file(src, dst, opt);
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "CopyFile error: " << src << " to " << dst << ", "
               << e.what();
  }
  return false;
}

bool Util::Copy(string_view src, string_view dst) {
  try {
    std::filesystem::copy(src, dst, std::filesystem::copy_options::recursive);
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "Copy error: " << src << " to " << dst << ", " << e.what();
    return false;
  }
  return true;
}

bool Util::TruncateFile(const std::filesystem::path &path) {
  try {
    if (std::filesystem::exists(path)) {
      std::filesystem::remove(path);
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << "TruncateFile error: " << path << ", " << e.what();
    return false;
  }
  return true;
}

bool Util::WriteToFile(const std::filesystem::path &path, const string &content,
                       const bool append) {
  try {
    if (!std::filesystem::exists(path)) {
      if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
      }
    }

    std::ofstream ofs(path, append ? std::ios::app : std::ios::out);
    if (ofs && ofs.is_open()) {
      ofs << content;
      ofs.close();
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << (!append ? "Write to " : "Append to ") << path.string()
               << ", error: " << e.what();
    return false;
  }

  return true;
}

bool Util::LoadSmallFile(string_view file_name, string *content) {
  std::ifstream in(file_name.data());
  if (!in || !in.is_open()) {
    LOG(ERROR) << "Fail to open " << file_name
               << ", please check file exists and file permissions";
    return false;
  }
  std::stringstream buffer;
  buffer << in.rdbuf();
  in.close();
  *content = buffer.str();
  return true;
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

bool Util::ToInt(const string &str, uint32_t *value) {
  auto result = std::from_chars(str.data(), str.data() + str.size(), *value);
  if (result.ec != std::errc{}) {
    return false;
  }
  return true;
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

void Util::ToHexStr(string_view in, std::string *out, bool use_upper_case) {
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

bool Util::HexStrToInt64(string_view in, int64_t *out) {
  *out = 0;
  auto result = std::from_chars(in.data(), in.data() + in.size(), *out, 16);
  if (result.ec == std::errc()) {
    return false;
  }
  return true;
}

uint32_t Util::CRC32(string_view content) { return crc32c::Crc32c(content); }

string Util::Base64Encode(string_view input) {
  string output;
  output.resize(boost::beast::detail::base64::encoded_size(input.size()));
  auto const ret = boost::beast::detail::base64::encode(
      output.data(), input.data(), input.size());
  output.resize(ret);
  return output;
}

string Util::Base64Decode(string_view input) {
  string output;
  output.resize(boost::beast::detail::base64::decoded_size(input.size()));
  auto const ret = boost::beast::detail::base64::decode(
      output.data(), input.data(), input.size());
  output.resize(ret.first);
  return output;
}

int64_t Util::MurmurHash64A(string_view str) {
  return ::MurmurHash64A(str.data(), str.size(), 42L);
}

bool Util::Hash(string_view str, const EVP_MD *type, string *out,
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

  string_view sv(reinterpret_cast<const char *>(hash), length);
  Util::ToHexStr(sv, out, use_upper_case);
  return true;
}

bool Util::ExtraFileHash(const std::string &path, const EVP_MD *type,
                         std::string *out, bool use_upper_case) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length;
  const size_t buffer_size = 64 * 1024 * 1024 * 8;  // 64MB buffer

  std::ifstream file(path, std::ios::binary);
  if (!file || !file.is_open()) {
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

  char buffer[buffer_size];
  while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
    if (EVP_DigestUpdate(context, buffer, file.gcount()) != 1) {
      EVP_MD_CTX_free(context);
      return false;
    }
  }

  if (EVP_DigestFinal_ex(context, hash, &length) != 1) {
    EVP_MD_CTX_free(context);
    return false;
  }

  EVP_MD_CTX_free(context);

  string_view sv(reinterpret_cast<const char *>(hash), length);
  Util::ToHexStr(sv, out, use_upper_case);
  return true;
}

bool Util::BigFileHash(const std::string &path, const EVP_MD *type,
                       std::string *out, bool use_upper_case) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length;
  const size_t buffer_size = 64 * 1024 * 1024 * 8;  // 64MB buffer

  std::ifstream file(path, std::ios::binary);
  if (!file || !file.is_open()) {
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

  char buffer[buffer_size];
  while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
    if (EVP_DigestUpdate(context, buffer, file.gcount()) != 1) {
      EVP_MD_CTX_free(context);
      return false;
    }
  }

  if (EVP_DigestFinal_ex(context, hash, &length) != 1) {
    EVP_MD_CTX_free(context);
    return false;
  }

  EVP_MD_CTX_free(context);

  string_view sv(reinterpret_cast<const char *>(hash), length);
  Util::ToHexStr(sv, out, use_upper_case);
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

bool Util::MD5(string_view str, string *out, bool use_upper_case) {
  return Hash(str, EVP_md5(), out, use_upper_case);
}

bool Util::SmallFileMD5(const string &path, string *out, bool use_upper_case) {
  return SmallFileHash(path, EVP_md5(), out, use_upper_case);
}

bool Util::BigFileMD5(const std::string &path, string *out,
                      bool use_upper_case) {
  return Util::BigFileHash(path, EVP_md5(), out, use_upper_case);
}

bool Util::ExtraFileMD5(const string &path, string *out, bool use_upper_case) {
  return Util::ExtraFileHash(path, EVP_md5(), out, use_upper_case);
}

bool Util::SHA256(string_view str, string *out, bool use_upper_case) {
  return Hash(str, EVP_sha256(), out, use_upper_case);
}

bool Util::SHA256_libsodium(string_view str, string *out, bool use_upper_case) {
  unsigned char hash[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(hash, reinterpret_cast<const unsigned char *>(str.data()),
                     str.size());

  string_view sv(reinterpret_cast<const char *>(hash),
                 crypto_hash_sha256_BYTES);
  Util::ToHexStr(sv, out, use_upper_case);
  return true;
}

bool Util::SmallFileSHA256(const string &path, string *out,
                           bool use_upper_case) {
  return SmallFileHash(path, EVP_sha256(), out, use_upper_case);
}

bool Util::BigFileSHA256(const std::string &path, string *out,
                         bool use_upper_case) {
  return BigFileHash(path, EVP_sha256(), out, use_upper_case);
}

bool Util::ExtraFileSHA256(const string &path, string *out,
                           bool use_upper_case) {
  return ExtraFileHash(path, EVP_sha256(), out, use_upper_case);
}

void Util::PrintProtoMessage(const google::protobuf::Message &msg) {
  JsonPrintOptions option;
  option.add_whitespace = false;
  option.preserve_proto_field_names = true;
  string json_value;
  if (!MessageToJsonString(msg, &json_value, option).ok()) {
    LOG(ERROR) << "to json string failed";
  }
  LOG(INFO) << "json_value: " << json_value;
}

void Util::PrintProtoMessage(const google::protobuf::Message &msg,
                             string *json) {
  JsonPrintOptions option;
  option.add_whitespace = false;
  option.preserve_proto_field_names = true;
  if (!MessageToJsonString(msg, json, option).ok()) {
    LOG(ERROR) << "to json string failed";
  }
}

bool Util::JsonToMessage(const std::string &json,
                         google::protobuf::Message *msg) {
  JsonParseOptions option;
  option.case_insensitive_enum_parsing = false;
  option.ignore_unknown_fields = true;
  if (!google::protobuf::util::JsonStringToMessage(json, msg).ok()) {
    LOG(ERROR) << "json string to msg failed";
    return false;
  }
  return true;
}

int64_t Util::UpdateTime(string_view path) {
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
  if (lstat(path.data(), &attr) == 0) {
#ifdef __APPLE__
    return attr.st_mtime * 1000 + attr.st_mtimespec.tv_nsec / 1000000
#else
    return attr.st_mtime * 1000 + attr.st_mtim.tv_nsec / 1000000;
#endif
  }
#endif
  return 0;
}

int64_t Util::CreateTime(string_view path) {
#if defined(_WIN32)
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
    ULARGE_INTEGER ull;
    ull.LowPart = fileInfo.ftCreationTime.dwLowDateTime;
    ull.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
    return (ull.QuadPart / 10000ULL) - 11644473600000ULL;
  }
#else
  struct stat attr;
  if (lstat(path.data(), &attr) == 0) {
#ifdef __linux__
    return attr.st_ctim.tv_sec * 1000 + attr.st_ctim.tv_nsec / 1000000;
#elif __APPLE__
    return attr.st_ctim.tv_sec * 1000 + attr.st_birthtimespec.tv_sec / 1000000;
#endif
  }
  return 0;
#endif
}

int64_t Util::FileSize(string_view path) {
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
  if (lstat(path.data(), &attr) == 0) {
    return attr.st_size;
  }
#endif
  return 0;
}

void Util::FileInfo(string_view path, int64_t *create_time,
                    int64_t *update_time, int64_t *size) {
#if defined(_WIN32)
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileInfo)) {
    ULARGE_INTEGER ull;
    ull.LowPart = fileInfo.ftCreationTime.dwLowDateTime;
    ull.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
    *create_time = ((ull.QuadPart / 10000ULL) - 11644473600000ULL);

    ull.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    *upadte_time = ((ull.QuadPart / 10000ULL) - 11644473600000ULL);

    ull.LowPart = fileInfo.nFileSizeLow;
    ull.HighPart = fileInfo.nFileSizeHigh;
    *size = ull.QuadPart;
  }
#else
  struct stat attr;
  if (lstat(path.data(), &attr) == 0) {
    *size = attr.st_size;
#ifdef __linux__
    *create_time = attr.st_ctim.tv_sec * 1000 + attr.st_ctim.tv_nsec / 1000000;
    *update_time = attr.st_mtime * 1000 + attr.st_mtim.tv_nsec / 1000000;
#elif __APPLE__
    *create_time =
        attr.st_ctim.tv_sec * 1000 + attr.st_birthtimespec.tv_sec / 1000000;
    *update_time = attr.st_mtime * 1000 + attr.st_mtimespec.tv_nsec / 1000000
#endif
  }
#endif
}

std::string Util::PartitionUUID(string_view path) {
#if defined(_WIN32)
  return UtilWindows::PartitionUUID(path);
#elif defined(__linux__)
  return UtilLinux::PartitionUUID(path);
#elif defined(__APPLE__)
  return UtilOsx::PartitionUUID(path);
#endif
}

std::string Util::Partition(string_view path) {
#if defined(_WIN32)
  return UtilWindows::Partition(path);
#elif defined(__linux__)
  return UtilLinux::Partition(path);
#elif defined(__APPLE__)
  return UtilOsx::Partition(path);
#endif
}

bool Util::SetFileInvisible(string_view path) {
#if defined(_WIN32)
  return UtilWindows::SetFileInvisible(path);
#elif defined(__linux__)
  return UtilLinux::SetFileInvisible(path);
#elif defined(__APPLE__)
  return UtilOsx::SetFileInvisible(path);
#endif
}

bool Util::IsAbsolute(string_view src) {
  std::filesystem::path s_src(src);
  return s_src.is_absolute();
}

bool Util::Relative(string_view path, string_view base, string *relative) {
  relative->clear();
  const string u_path = UnifyDir(path);
  const string u_base = UnifyDir(base);

  auto s_path = std::filesystem::path(u_path);
  auto s_base = std::filesystem::path(u_base);
  if (!s_path.is_absolute() || !s_base.is_absolute()) {
    return false;
  }

  if (boost::algorithm::starts_with(u_path, u_base) &&
      u_path.size() > u_base.size()) {
    *relative = u_path.substr(u_base.size() + 1);
  }
  return true;
}

bool Util::SyncSymlink(const std::string &src, const std::string &dst,
                       const std::string &src_symlink) {
  auto target = std::filesystem::read_symlink(src_symlink);
  std::string target_relative_path;
  Util::Relative(target.string(), src, &target_relative_path);
  std::string symlink_relative_path;
  Util::Relative(src_symlink, src, &symlink_relative_path);
  auto dst_symlink = dst + "/" + symlink_relative_path;

  Util::Remove(dst_symlink);
  if (target.is_absolute()) {
    if (Util::StartWith(target, src)) {
      std::filesystem::create_symlink(dst + "/" + target_relative_path,
                                      dst_symlink);
    } else {
      std::filesystem::create_symlink(target, dst_symlink);
    }
  } else {
    std::filesystem::create_symlink(target, dst_symlink);
  }
  return true;
}

}  // namespace util
}  // namespace oceandoc
