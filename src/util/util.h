/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_UTIL_H
#define BAZEL_TEMPLATE_UTIL_UTIL_H

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "absl/time/time.h"
#include "boost/algorithm/string/replace.hpp"
#include "google/protobuf/message.h"
#include "openssl/types.h"

namespace oceandoc {
namespace util {

class Util final {
 private:
  Util() {}
  ~Util() {}

 public:
  static std::string GetServerIp();

  static int64_t CurrentTimeMillis();

  static int64_t CurrentTimeNanos();

  static int64_t StrToTimeStamp(std::string_view time);

  static int64_t StrToTimeStamp(std::string_view time, std::string_view format);

  static std::string ToTimeStr(const int64_t ts);
  static std::string ToTimeStr(const int64_t ts, std::string_view format);
  static std::string ToTimeStr(const int64_t ts, std::string_view format,
                               std::string_view tz);

  static int64_t Random(int64_t start, int64_t end);

  static void Sleep(int64_t ms);

  static void SleepUntil(const absl::Time &time);

  static bool SleepUntil(const absl::Time &time, volatile bool *stop_signal);

  static void UnifyDir(std::string *path);
  static std::string UnifyDir(std::string_view path);

  static bool Remove(std::string_view path);

  static bool Mkdir(std::string_view path);

  static bool CopyFile(std::string_view src, std::string_view dst,
                       const std::filesystem::copy_options opt);

  static bool Copy(std::string_view src, std::string_view dst);

  static bool TruncateFile(const std::filesystem::path &path);

  static bool WriteToFile(const std::filesystem::path &path,
                          const std::string &content,
                          const bool append = false);

  static bool LoadSmallFile(std::string_view path, std::string *content);

  static bool IsAbsolute(std::string_view src);

  static bool Relative(std::string_view path, std::string_view base,
                       std::string *out);
  static int64_t CreateTime(std::string_view path);
  static int64_t UpdateTime(std::string_view path);
  static int64_t FileSize(std::string_view path);
  static void FileInfo(std::string_view path, int64_t *create_time,
                       int64_t *update_time, int64_t *size);
  static std::string PartitionUUID(std::string_view path);
  static std::string Partition(std::string_view path);
  static bool SetFileInvisible(std::string_view path);

  static std::string UUID();
  static std::string ToUpper(const std::string &str);

  static std::string ToLower(const std::string &str);

  static void ToLower(std::string *str);

  static void Trim(std::string *s);

  static std::string Trim(const std::string &s);

  static bool ToInt(const std::string &str, uint32_t *value);

  static bool StartWith(const std::string &str, const std::string &prefix);

  static bool EndWith(const std::string &str, const std::string &postfix);

  static void ReplaceAll(std::string *s, std::string_view from,
                         std::string_view to) {
    boost::algorithm::replace_all(*s, from, to);
  }

  template <class TypeName>
  static void ReplaceAll(std::string *s, std::string_view from,
                         const TypeName to) {
    boost::algorithm::replace_all(*s, from, std::to_string(to));
  }

  static void Split(const std::string &str, const std::string &delim,
                    std::vector<std::string> *result, bool trim_empty = true);

  static std::string Base64Encode(std::string_view input);

  static std::string Base64Decode(std::string_view input);

  static uint32_t CRC32(std::string_view content);

  static bool Hash(std::string_view str, const EVP_MD *type, std::string *out,
                   bool use_upper_case = false);

  static bool ExtraFileHash(const std::string &path, const EVP_MD *type,
                            std::string *out, bool use_upper_case = false);

  static bool BigFileHash(const std::string &path, const EVP_MD *type,
                          std::string *out, bool use_upper_case = false);

  static bool SmallFileHash(const std::string &path, const EVP_MD *type,
                            std::string *out, bool use_upper_case = false);

  static bool SHA256(std::string_view str, std::string *out,
                     bool use_upper_case = false);

  static bool SHA256_libsodium(std::string_view str, std::string *out,
                               bool use_upper_case = false);

  static bool SmallFileSHA256(const std::string &path, std::string *out,
                              bool use_upper_case = false);

  static bool BigFileSHA256(const std::string &path, std::string *out,
                            bool use_upper_case = false);

  // file size >= 1/4 memory
  static bool ExtraFileSHA256(const std::string &path, std::string *out,
                              bool use_upper_case = false);

  static bool MD5(std::string_view str, std::string *out,
                  bool use_upper_case = false);

  static bool SmallFileMD5(const std::string &path, std::string *out,
                           bool use_upper_case = false);

  static bool BigFileMD5(const std::string &path, std::string *out,
                         bool use_upper_case = false);

  // file size >= 1/4 memory
  static bool ExtraFileMD5(const std::string &path, std::string *out,
                           bool use_upper_case = false);

  static bool HexStrToInt64(std::string_view in, int64_t *out);

  static std::string ToHexStr(const uint64_t in, bool use_upper_case = false);

  static void ToHexStr(std::string_view in, std::string *out,
                       bool use_upper_case = false);

  static int64_t MurmurHash64A(std::string_view str);

  static void PrintProtoMessage(const google::protobuf::Message &msg);

  static void PrintProtoMessage(const google::protobuf::Message &msg,
                                std::string *json);
  static bool JsonToMessage(const std::string &json,
                            google::protobuf::Message *msg);

  template <class TypeName>
  static void AppendField(const std::string &key, const TypeName value,
                          std::string *track_str, std::string *search_str,
                          const bool first = false) {
    if (!first) {
      track_str->append("&");
      search_str->append("\t");
    }
    track_str->append(key);
    search_str->append(key);

    track_str->append("=");
    search_str->append("=");

    track_str->append(std::to_string(value));
    search_str->append(std::to_string(value));
  }

  static void AppendField(const std::string &key, const std::string &value,
                          std::string *track_str, std::string *search_str,
                          const bool first = false) {
    if (!first) {
      track_str->append("&");
      search_str->append("\t");
    }
    track_str->append(key);
    search_str->append(key);

    track_str->append("=");
    search_str->append("=");

    track_str->append(value);
    search_str->append(value);
  }

  template <class TypeName>
  static void AppendField(const std::string &key, const TypeName value,
                          std::string *search_str, const bool first = false) {
    if (!first) {
      search_str->append("\t");
    }
    search_str->append(key);
    search_str->append("=");
    search_str->append(std::to_string(value));
  }

  static void AppendField(const std::string &key, const char *const &value,
                          std::string *search_str, const bool first = false) {
    if (!first) {
      search_str->append("\t");
    }
    search_str->append(key);
    search_str->append("=");
    search_str->append(value);
  }

  static void AppendField(const std::string &key, const std::string &value,
                          std::string *search_str, const bool first = false) {
    if (!first) {
      search_str->append("\t");
    }
    search_str->append(key);
    search_str->append("=");
    search_str->append(value);
  }

  static bool SyncSymlink(const std::string &src, const std::string &dst,
                          const std::string &src_symlink);

 public:
  static const char *kPathDelimeter;
};

}  // namespace util
}  // namespace oceandoc

#endif /* BAZEL_TEMPLATE_UTIL_UTIL_H */
