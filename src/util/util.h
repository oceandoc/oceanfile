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

#include "boost/algorithm/string/replace.hpp"
#include "google/protobuf/message.h"
#include "openssl/types.h"
#include "src/common/defs.h"
#include "src/proto/service.pb.h"

namespace oceandoc {
namespace util {

class Util final {
 private:
  Util() {}
  ~Util() {}

 public:
  static int64_t CurrentTimeMillis();
  static int64_t CurrentTimeNanos();
  static int64_t StrToTimeStamp(std::string_view time);
  static int64_t StrToTimeStamp(std::string_view time, std::string_view format);

  static std::string ToTimeStr();
  static std::string ToTimeStr(const int64_t ts);
  static std::string ToTimeStr(const int64_t ts, std::string_view format);
  static std::string ToTimeStr(const int64_t ts, std::string_view format,
                               std::string_view tz);

  static int64_t Random(int64_t start, int64_t end);

  static void Sleep(int64_t ms);

  static void UnifyDir(std::string *path);
  static std::string UnifyDir(std::string_view path);

  static bool IsAbsolute(std::string_view src);

  static int64_t CreateTime(const std::string &path);
  static int64_t UpdateTime(const std::string &path);
  static int64_t FileSize(const std::string &path);
  static bool FileInfo(const std::string &path, int64_t *create_time,
                       int64_t *update_time, int64_t *size);
  static bool Exists(std::string_view path);
  static bool Mkdir(std::string_view path);
  static bool MkParentDir(const std::filesystem::path &path);
  static bool Remove(std::string_view path);
  static bool Create(const std::string &path);
  static bool CreateFileWithSize(const std::string &path, const int64_t size);
  static bool CreateSymlink(std::string_view src, std::string_view target);

  static std::filesystem::path FindCommonRoot(
      const std::filesystem::path &path, const std::filesystem::path &base);
  static bool Relative(std::string_view path, std::string_view base,
                       std::string *out);

  static bool CopyFile(std::string_view src, std::string_view dst,
                       const std::filesystem::copy_options opt =
                           std::filesystem::copy_options::overwrite_existing);
  static bool Copy(std::string_view src, std::string_view dst);
  static bool TruncateFile(const std::filesystem::path &path);
  static bool WriteToFile(const std::filesystem::path &path,
                          const std::string &content,
                          const bool append = false);
  static bool WriteToFile(const std::filesystem::path &path,
                          const std::string &content, const int64_t start);
  static bool LoadSmallFile(const std::string &path, std::string *content);

  static std::string PartitionUUID(std::string_view path);
  static std::string Partition(std::string_view path);
  static bool SetFileInvisible(std::string_view path);
  static bool SyncSymlink(const std::string &src, const std::string &dst,
                          const std::string &src_symlink);

  static int32_t FilePartitionNum(const std::string &path);

  static int32_t FilePartitionNum(const int64_t total_size);

  static int32_t FilePartitionNum(const std::string &path,
                                  const int64_t partition_size);

  static int32_t FilePartitionNum(const int64_t total_size,
                                  int64_t partition_size);

  static bool PrepareFile(const std::string &path, common::FileAttr *attr);

  static bool SimplifyPath(const std::string &path, std::string *out);
  static std::string RepoFilePath(const std::string &repo_path,
                                  const std::string &sha256);

  static void CalcPartitionStart(const int64_t size, const int32_t partition,
                                 const int64_t partition_size, int64_t *start,
                                 int64_t *end);

  static std::string UUID();
  static std::string ToUpper(const std::string &str);

  static std::string ToLower(const std::string &str);

  static void ToLower(std::string *str);

  static void Trim(std::string *s);

  static std::string Trim(const std::string &s);

  static bool ToInt(const std::string &str, uint32_t *value);

  static bool StartWith(const std::string &str, const std::string &prefix);

  static bool EndWith(const std::string &str, const std::string &postfix);

  static void ReplaceAll(std::string *s, const std::string &from,
                         const std::string &to) {
    boost::algorithm::replace_all(*s, from, to);
  }

  template <class TypeName>
  static void ReplaceAll(std::string *s, const std::string &from,
                         const TypeName to) {
    boost::algorithm::replace_all(*s, from, std::to_string(to));
  }

  static void Split(const std::string &str, const std::string &delim,
                    std::vector<std::string> *result, bool trim_empty = true);

  static std::string Base64Encode(std::string_view input);

  static std::string Base64Decode(std::string_view input);

  static void Base64Encode(std::string_view input, std::string *out);

  static void Base64Decode(std::string_view input, std::string *out);

  static uint32_t CRC32(std::string_view content);

  static EVP_MD_CTX *HashInit(const EVP_MD *type);
  static bool HashUpdate(EVP_MD_CTX *context, std::string_view str);
  static bool HashFinal(EVP_MD_CTX *context, std::string *out,
                        bool use_upper_case = false);

  static EVP_MD_CTX *SHA256Init();
  static bool SHA256Update(EVP_MD_CTX *context, std::string_view str);
  static bool SHA256Final(EVP_MD_CTX *context, std::string *out,
                          bool use_upper_case = false);

  static bool Hash(std::string_view str, const EVP_MD *type, std::string *out,
                   bool use_upper_case = false);

  static bool FileHash(const std::string &path, const EVP_MD *type,
                       std::string *out, bool use_upper_case = false);

  static bool SmallFileHash(const std::string &path, const EVP_MD *type,
                            std::string *out, bool use_upper_case = false);

  static bool SHA256(std::string_view str, std::string *out,
                     bool use_upper_case = false);

  static bool SHA256_libsodium(std::string_view str, std::string *out,
                               bool use_upper_case = false);

  static bool SmallFileSHA256(const std::string &path, std::string *out,
                              bool use_upper_case = false);

  static bool FileSHA256(const std::string &path, std::string *out,
                         bool use_upper_case = false);

  static bool MD5(std::string_view str, std::string *out,
                  bool use_upper_case = false);

  static bool SmallFileMD5(const std::string &path, std::string *out,
                           bool use_upper_case = false);

  static bool FileMD5(const std::string &path, std::string *out,
                      bool use_upper_case = false);

  static bool HexStrToInt64(std::string_view in, int64_t *out);

  static std::string ToHexStr(const uint64_t in, bool use_upper_case = false);

  static void ToHexStr(std::string_view in, std::string *out,
                       bool use_upper_case = false);

  static std::string ToHexStr(std::string_view in, bool use_upper_case = false);

  static int64_t MurmurHash64A(std::string_view str);

  static bool LZMACompress(std::string_view data, std::string *out);
  static bool LZMADecompress(std::string_view data, std::string *out);

  static void PrintProtoMessage(const google::protobuf::Message &msg);

  static bool MessageToJson(const google::protobuf::Message &msg,
                            std::string *json);
  static bool JsonToMessage(const std::string &json,
                            google::protobuf::Message *msg);
  static void PrintFileReq(const proto::FileReq &req);
  static bool FileReqToJson(const proto::FileReq &req, std::string *json);
  static bool JsonToFileReq(const std::string &json, proto::FileReq *req);
};

}  // namespace util
}  // namespace oceandoc

#endif /* BAZEL_TEMPLATE_UTIL_UTIL_H */
