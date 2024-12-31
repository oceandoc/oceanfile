/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef BAZEL_TEMPLATE_UTIL_UTIL_H
#define BAZEL_TEMPLATE_UTIL_UTIL_H

#include <charconv>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// clang-format off
#include "src/common/fix_header_order.h" // NOLINT
// clang-format on

#include "boost/algorithm/string/replace.hpp"
#include "folly/IPAddress.h"
#include "google/protobuf/message.h"
#include "openssl/types.h"
#include "src/common/defs.h"
#include "src/proto/error.pb.h"
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
  static int64_t StrToTimeStampLocal(const std::string &time);
  static int64_t StrToTimeStampLocal(const std::string &time,
                                     const std::string &format);
  static int64_t StrToTimeStampUTC(const std::string &time);
  static int64_t StrToTimeStampUTC(const std::string &time,
                                   const std::string &format);
  static int64_t StrToTimeStamp(const std::string &time,
                                const std::string &format,
                                const std::string &tz_str);

  static std::string ToTimeStrLocal();
  static std::string ToTimeStrLocal(const int64_t ts);
  static std::string ToTimeStrLocal(const int64_t ts,
                                    const std::string &format);
  static std::string ToTimeStrUTC();
  static std::string ToTimeStrUTC(const int64_t ts);
  static std::string ToTimeStrUTC(const int64_t ts, const std::string &format);
  // CST chines standard time
  static std::string ToTimeStr(const int64_t ts, const std::string &format,
                               const std::string &tz_str);
  static struct timespec ToTimeSpec(const int64_t ts);

  static int64_t Random(int64_t start, int64_t end);

  static void Sleep(int64_t ms);

  static void UnifyDir(std::string *path);
  static std::string UnifyDir(const std::string &path);

  static bool IsAbsolute(const std::string &src);

  static bool SetUpdateTime(const std::string &path, int64_t ts);
  static int64_t UpdateTime(const std::string &path);
  static int64_t FileSize(const std::string &path);
  static bool FileInfo(const std::string &path, int64_t *update_time,
                       int64_t *size, std::string *user, std::string *group);
  static bool Exists(const std::string &path);
  static bool TargetExists(const std::string &src, const std::string &dst);
  static bool Mkdir(const std::string &path);
  static bool MkParentDir(const std::string &path);
  static bool Remove(const std::string &path);
  static bool Create(const std::string &path);
  static bool Rename(const std::string &src, const std::string &dst);
  static int32_t CreateFileWithSize(const std::string &path,
                                    const int64_t size);
  static bool CreateSymlink(const std::string &src, const std::string &target);

  static std::string FindCommonRoot(const std::filesystem::path &path,
                                    const std::filesystem::path &base);
  static bool Relative(const std::string &path, const std::string &base,
                       std::string *out);

  static std::string ParentPath(const std::string &path);
  static std::string CurrentPath();
  static std::string AbsolutePath(const std::string &path);
  static std::string RealPath(const std::string &path);

  static bool CopyFile(const std::string &src, const std::string &dst,
                       const std::filesystem::copy_options opt =
                           std::filesystem::copy_options::overwrite_existing);
  static bool Copy(const std::string &src, const std::string &dst);
  static bool TruncateFile(const std::string &path);
  static int32_t WriteToFile(const std::string &path,
                             const std::string &content,
                             const bool append = false);
  static int32_t WriteToFile(const std::string &path,
                             const std::string &content, const int64_t start);
  static bool LoadSmallFile(const std::string &path, std::string *content);

  static bool SyncSymlink(const std::string &src, const std::string &dst,
                          const std::string &src_symlink);
  static bool SyncRemoteSymlink(const std::string &src,
                                const std::string &src_symlink,
                                std::string *target);

  static int32_t FilePartitionNum(const std::string &path);

  static int32_t FilePartitionNum(const int64_t total_size);

  static int32_t FilePartitionNum(const std::string &path,
                                  const int64_t partition_size);

  static int32_t FilePartitionNum(const int64_t total_size,
                                  int64_t partition_size);

  static bool PrepareFile(const std::string &path,
                          const common::HashMethod hash_method,
                          const int64_t partition_size, common::FileAttr *attr);

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

  template <class TypeName>
  static bool ToInt(std::string_view str, TypeName *value) {
    auto result = std::from_chars(str.data(), str.data() + str.size(), *value);
    if (result.ec != std::errc{}) {
      return false;
    }
    return true;
  }

  template <class TypeName>
  static TypeName ToInt(std::string_view str) {
    TypeName num = 0;
    auto result = std::from_chars(str.data(), str.data() + str.size(), num);
    if (result.ec != std::errc{}) {
      return 0;
    }
    return num;
  }

  static bool StartWith(const std::string &str, const std::string &prefix);

  static bool EndWith(const std::string &str, const std::string &postfix);
  static bool Contain(const std::string &str, const std::string &p);

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

  static std::string Base64Encode(const std::string &input);

  static std::string Base64Decode(const std::string &input);

  static void Base64Encode(const std::string &input, std::string *out);

  static void Base64Decode(const std::string &input, std::string *out);

  static uint32_t CRC32(const std::string &content);

  static bool Blake3(const std::string &content, std::string *out,
                     const bool use_upper_case = false);
  static bool FileBlake3(const std::string &path, std::string *out,
                         const bool use_upper_case = false);

  static EVP_MD_CTX *HashInit(const EVP_MD *type);
  static bool HashUpdate(EVP_MD_CTX *context, const std::string &str);
  static bool HashFinal(EVP_MD_CTX *context, std::string *out,
                        const bool use_upper_case = false);

  static EVP_MD_CTX *SHA256Init();
  static bool SHA256Update(EVP_MD_CTX *context, const std::string &str);
  static bool SHA256Final(EVP_MD_CTX *context, std::string *out,
                          const bool use_upper_case = false);

  static bool Hash(const std::string &str, const EVP_MD *type, std::string *out,
                   const bool use_upper_case = false);

  static bool FileHash(const std::string &path, const EVP_MD *type,
                       std::string *out, const bool use_upper_case = false);

  static bool SmallFileHash(const std::string &path, const EVP_MD *type,
                            std::string *out,
                            const bool use_upper_case = false);

  static bool SHA256(const std::string &str, std::string *out,
                     const bool use_upper_case = false);

  static std::string SHA256(const std::string &str,
                            const bool use_upper_case = false);

  static bool SHA256_libsodium(const std::string &str, std::string *out,
                               const bool use_upper_case = false);

  static bool SmallFileSHA256(const std::string &path, std::string *out,
                              const bool use_upper_case = false);

  static bool FileSHA256(const std::string &path, std::string *out,
                         const bool use_upper_case = false);

  static bool MD5(const std::string &str, std::string *out,
                  const bool use_upper_case = false);

  static bool SmallFileMD5(const std::string &path, std::string *out,
                           const bool use_upper_case = false);

  static bool FileMD5(const std::string &path, std::string *out,
                      const bool use_upper_case = false);

  static std::string GenerateSalt();
  static bool HashPassword(const std::string &password, const std::string &salt,
                           std::string *hash);
  static bool VerifyPassword(const std::string &password,
                             const std::string &salt,
                             const std::string &stored_hash);

  static bool HexStrToInt64(const std::string &in, int64_t *out);

  static std::string ToHexStr(const uint64_t in,
                              const bool use_upper_case = false);

  static void ToHexStr(const std::string &in, std::string *out,
                       const bool use_upper_case = false);

  static std::string ToHexStr(const std::string &in,
                              const bool use_upper_case = false);

  static int64_t MurmurHash64A(const std::string &str);

  static bool LZMACompress(const std::string &data, std::string *out);
  static bool LZMADecompress(const std::string &data, std::string *out);

  static void PrintProtoMessage(const google::protobuf::Message &msg);

  static bool MessageToJson(const google::protobuf::Message &msg,
                            std::string *json);
  static std::string MessageToJson(const google::protobuf::Message &msg);
  static bool MessageToPrettyJson(const google::protobuf::Message &msg,
                                  std::string *json);
  static bool JsonToMessage(const std::string &json,
                            google::protobuf::Message *msg);
  static void PrintFileReq(const proto::FileReq &req);
  static bool FileReqToJson(const proto::FileReq &req, std::string *json);
  static bool JsonToFileReq(const std::string &json, proto::FileReq *req);

  static std::optional<std::string> GetEnv(const char *var_name) {
    const char *value = std::getenv(var_name);
    if (value) {
      return std::string(value);
    } else {
      return std::nullopt;
    }
  }

  static void PrintAllEnv();

  static int64_t FDCount();
  static int64_t MemUsage();
  static bool IsMountPoint(const std::string &path);
  static void ListAllIPAddresses(std::vector<folly::IPAddress> *ip_addrs);
  static std::string ExecutablePath();
  static std::string HomeDir();
};

}  // namespace util
}  // namespace oceandoc

#endif /* BAZEL_TEMPLATE_UTIL_UTIL_H */
