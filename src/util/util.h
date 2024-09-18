/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_UTIL_H
#define BAZEL_TEMPLATE_UTIL_UTIL_H

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "absl/time/time.h"
#include "boost/algorithm/string/replace.hpp"
#include "google/protobuf/message.h"

namespace oceandoc {
namespace util {

class Util final {
 private:
  Util() {}
  ~Util() {}

 public:
  static std::string GetServerIp();

  static int64_t CurrentTimeMillis();

  static int64_t NanoTime();

  static uint64_t Now();

  static uint64_t StrTimeToTimestamp(const std::string &time,
                                     int32_t offset = 0);

  static std::string ToTimeStr(const int64_t ts);

  static std::string TodayStr();

  static std::string DetailTimeStr(const int64_t ts);
  static std::string DetailTimeStr();

  static int64_t Random(int64_t start, int64_t end);

  static void Sleep(int64_t ms);

  static void SleepUntil(const absl::Time &time);

  static bool SleepUntil(const absl::Time &time, volatile bool *stop_signal);

  static bool IsDir(const std::string &path);

  static bool IsFile(const std::string &dir, const std::string &file);

  static std::string DirName(const std::string &path);

  static std::string BaseName(const std::string &path);

  static std::string FileExtention(const std::string &path);

  static bool Remove(const std::string &dir, const std::string &file);

  static bool Remove(const std::string &dir);

  static bool Mkdir(const std::string &dir);

  static bool Exist(const std::string &path);

  static std::string RealPath(const std::string &path);

  static bool CopyFile(const std::string &src_file_path,
                       const std::string dst_file_path,
                       const std::filesystem::copy_options opt);

  static bool Copy(const std::string &src_path, const std::string dst_path);

  static bool TruncateFile(const std::filesystem::path &path);

  static bool WriteToFile(const std::filesystem::path &path,
                          const std::string &content,
                          const bool append = false);

  static std::istream &GetLine(std::istream &is, std::string *line);

  static bool LoadSmallFile(std::string_view path, std::string *content);

  static std::vector<std::string> LoadLines(const std::string &file_name);

  static std::string FileMd5(const std::string &file_path);

  static std::string ToUpper(const std::string &str);

  static std::string ToLower(const std::string &str);

  static void ToLower(std::string *str);

  static void Trim(std::string *s);

  static std::string Trim(const std::string &s);

  static bool ToInt(const std::string &str, uint32_t *value);

  static bool StartWith(const std::string &str, const std::string &prefix);

  static bool EndWith(const std::string &str, const std::string &postfix);

  template <class TypeName>
  static void ReplaceAll(std::string *s, const std::string &from,
                         const TypeName &to) {
    boost::algorithm::replace_all(*s, from, std::to_string(to));
  }

  static void ReplaceAll(std::string *s, const std::string &from,
                         const std::string &to);

  static void ReplaceAll(std::string *s, const std::string &from,
                         const char *const to);

  static void Split(const std::string &str, const std::string &delim,
                    std::vector<std::string> *result, bool trim_empty = true);

  static std::string ToString(const std::set<uint64_t> &ids);

  static std::string ToString(const std::map<std::string, std::string> &vars);

  static std::string StandardBase64Encode(const std::string &input);

  static std::string StandardBase64Decode(const std::string &input);

  static std::string Base64Encode(std::string_view s, bool url = false);

  static std::string Base64EncodePem(std::string_view s);

  static std::string Base64EncodeMime(std::string_view s);

  static std::string Base64Decode(std::string_view s,
                                  bool remove_linebreaks = false);

  static std::string Md5(const std::string &str, bool use_upper_case = false);

  static uint64_t HexStrToUInt64(const std::string &in);

  static std::string ToHexStr(const uint64_t in);
  static void ToHexStr(std::string_view in, std::string *out);

  static int64_t Hash64(const std::string &str);

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

  static int64_t ToTimestamp(std::filesystem::file_time_type ftime);
  static int64_t CreateTime(std::string_view path);
  static int64_t UpdateTime(std::string_view path);
  static int64_t FileSize(std::string_view path);
  static void FileInfo(std::string_view path, int64_t *create_time,
                       int64_t *update_time, int64_t *size);
  static std::string PartitionUUID(std::string_view path);
  static std::string Partition(std::string_view path);
  static bool SetFileInvisible(std::string_view path);

  static std::string UUID();

  static uint32_t CRC32(std::string_view content);

  static bool SHA256(std::string_view content, std::string *out);

  static bool SHA256_libsodium(std::string_view content, std::string *out);

 public:
  static const char *kPathDelimeter;
};

}  // namespace util
}  // namespace oceandoc

#endif /* BAZEL_TEMPLATE_UTIL_UTIL_H */
