/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/util.h"

#include <filesystem>
#include <fstream>

#include "glog/logging.h"
#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

using namespace std;

TEST(Util, CurrentTimeMillis) {
  LOG(INFO) << Util::CurrentTimeMillis();
  EXPECT_GT(Util::CurrentTimeMillis(), 1704038400000);
  EXPECT_LT(Util::CurrentTimeMillis(), 1904038400000);
}

TEST(Util, CurrentTimeNanos) {
  LOG(INFO) << Util::CurrentTimeNanos();
  EXPECT_GT(Util::CurrentTimeNanos(), 1727101022292387983);
  EXPECT_LT(Util::CurrentTimeNanos(), 1927101022292387983);
}

TEST(Util, StrToTimeStamp) {
  EXPECT_EQ(Util::StrToTimeStamp("2024-09-24 13:36:44", "%Y-%m-%d HH:MM:SS"),
            -1);

  EXPECT_EQ(Util::StrToTimeStamp("2024-09-24 13:36:44", "%Y-%m-%d %H:%M:%S"),
            1727156204000);  // CST time

  EXPECT_EQ(Util::StrToTimeStamp("2024-09-24 22:48:50", "%Y-%m-%d %H:%M:%S"),
            1727189330000);  // CST time

  EXPECT_EQ(
      Util::StrToTimeStamp("2024-09-24 13:36:44.123", "%Y-%m-%d %H:%M:%E3S"),
      1727156204123);

  EXPECT_EQ(Util::StrToTimeStamp("2024-09-24 13:36:44.123",
                                 "%Y-%m-%d %H:%M:%E2S%E3f"),
            1727156204123);

  EXPECT_EQ(
      Util::StrToTimeStamp("2024-09-24 13:36:44.123", "%Y-%m-%d %H:%M:%E3S"),
      1727156204123);

  EXPECT_EQ(Util::StrToTimeStamp("2024-09-24T13:36:44.000+0000"),
            1727185004000);
  EXPECT_EQ(Util::StrToTimeStamp("2024-09-24T13:36:44.000+0800"),
            1727156204000);
  EXPECT_EQ(Util::StrToTimeStamp("2024-09-24 13:36:44.123+08:00",
                                 "%Y-%m-%d %H:%M:%E3S%Ez"),
            1727156204123);
  EXPECT_EQ(Util::StrToTimeStamp("2024-09-24 13:36:44.123+08:30:24",
                                 "%Y-%m-%d %H:%M:%E3S%E*z"),
            1727154380123);
}

TEST(Util, ToTimeStr) {
  EXPECT_EQ(Util::ToTimeStr(1646397312000), "2022-03-04T20:35:12.000+08:00");
  EXPECT_EQ(Util::ToTimeStr(1646397312000, "%Y-%m-%d %H:%M:%S"),
            "2022-03-04 20:35:12");
}

TEST(Util, UpdateTime) {
  const auto& path = "test_data/util_test/txt_symlink";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  EXPECT_EQ(Util::UpdateTime(path), 1726798844930);
  LOG(INFO) << Util::UpdateTime(path);
}

TEST(Util, CreateTime) {
  const auto& path = "test_data/util_test/txt_symlink";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  EXPECT_EQ(Util::CreateTime(path), 1726798844930);
  LOG(INFO) << Util::CreateTime(path);
}

TEST(Util, FileSize) {
  string path = "test_data/util_test/txt_symlink";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  EXPECT_EQ(Util::FileSize(path), 3);
  LOG(INFO) << Util::FileSize(path);

  path = "/root/src/Dr.Q/oceanfile";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is dir";
  }
  EXPECT_EQ(Util::FileSize(path), 4096);
  LOG(INFO) << Util::FileSize(path);
}

TEST(Util, UUID) {
  // eg: 2b68792c-0580-41a2-a7b1-ab3a96a1a58e
  LOG(INFO) << Util::UUID();
}

TEST(Util, CRC32) {
  // https://crccalc.com/?crc=123456789&method=&datatype=0&outtype=0
  // CRC-32/ISCSI
  auto start = Util::CurrentTimeMillis();
  std::string content =
      "A cyclic redundancy check (CRC) is an error-detecting code used to "
      "detect data corruption. When sending data, short checksum is generated "
      "based on data content and sent along with data. When receiving data, "
      "checksum is generated again and compared with sent checksum. If the two "
      "are equal, then there is no data corruption. The CRC-32 algorithm "
      "itself converts a variable-length string into an 8-character string.";
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, crc32:" << Util::CRC32(content)
            << ", cost: " << Util::CurrentTimeMillis() - start;
  EXPECT_EQ(Util::CRC32(content), 2331864810);
}

TEST(Util, SHA256) {
  std::string content;
  std::string out;
  auto start = Util::CurrentTimeMillis();
  content =
      "A cyclic redundancy check (CRC) is an error-detecting code used to "
      "detect data corruption. When sending data, short checksum is generated "
      "based on data content and sent along with data. When receiving data, "
      "checksum is generated again and compared with sent checksum. If the two "
      "are equal, then there is no data corruption. The CRC-32 algorithm "
      "itself converts a variable-length string into an 8-character string.";
  Util::SHA256(content, &out);
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, sha256:" << out
            << ", cost: " << Util::CurrentTimeMillis() - start;
  EXPECT_EQ(out,
            "3f5d419c0386a26df1c75d0d1c488506fb641b33cebaa2a4917127ae33030b31");
}

TEST(Util, Relative) {
  std::string path = "/usr/local/llvm/18/bin/llvm-dlltool";
  std::string base = "/usr/local/llvm";
  std::string relative;
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "18/bin/llvm-dlltool");

  path = "usr/local/llvm/18/bin/llvm-dlltool";
  base = "usr/local/llvm/";
  EXPECT_EQ(Util::Relative(path, base, &relative), false);

  path = "/usr/local/llvm/18/bin/llvm-dlltool";
  base = "/usr/local/llvm/";
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "18/bin/llvm-dlltool");

  path = "/usr/local/llvm/18/bin/llvm-dlltool/";
  base = "/usr/local/llvm";
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "18/bin/llvm-dlltool");

  path = "/usr/local/llvm/18/bin/llvm-dlltool/";
  base = "/usr/local/llvm/";
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "18/bin/llvm-dlltool");

  path = "/usr/local/llvm/";
  base = "/usr/local/llvm/";
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "");

  path = "/usr/local/llvm";
  base = "/usr/local/llvm/";
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "");

  path = "/usr/local/llvm/";
  base = "/usr/local/llvm";
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "");

  path = "/";
  base = "/";
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "");
}

TEST(Util, Exists) {
  std::string symlink_path = "test_data/util_test/txt_symlink";
  std::string target_path = "test_data/util_test/txt";

  if (!std::filesystem::exists(target_path)) {
    ofstream ofs(target_path);
    ofs.close();
  }
  EXPECT_EQ(std::filesystem::exists(target_path), true);
  EXPECT_EQ(Util::Exists(symlink_path), true);

  EXPECT_EQ(Util::Remove(target_path), true);
  EXPECT_EQ(std::filesystem::exists(target_path), false);
  EXPECT_EQ(Util::Exists(symlink_path), true);

  if (!std::filesystem::exists(target_path)) {
    ofstream ofs(target_path);
    ofs.close();
  }
}

TEST(Util, Remove) {
  std::string symlink_path = "test_data/util_test/txt_symlink";
  std::string target_path = "test_data/util_test/txt";

  if (!std::filesystem::exists(target_path)) {
    ofstream ofs(target_path);
    ofs.close();
  }

  EXPECT_EQ(Util::Remove(symlink_path), true);
  EXPECT_EQ(Util::Exists(target_path), true);
  EXPECT_EQ(Util::Exists(symlink_path), false);

  EXPECT_EQ(Util::Remove(target_path), true);
  EXPECT_EQ(Util::Exists(target_path), false);

  if (!std::filesystem::exists(target_path)) {
    ofstream ofs(target_path);
    ofs.close();
  }
}

TEST(Util, SyncSymlink) {
  Util::SyncSymlink("/usr/local/llvm", "/usr/local/test",
                    "/usr/local/llvm/18/bin/llvm-dlltool");
}

TEST(Util, LZMA) {
  std::string data = "/usr/local/llvm";
  std::string compressed_data;
  std::string decompressed_data;

  std::string compressed_data_hex;
  std::string decompressed_data_hex;

  if (!Util::LZMACompress(data, &compressed_data)) {
    LOG(ERROR) << "compress error";
  }
  Util::ToHexStr(compressed_data, &compressed_data_hex);

  EXPECT_EQ(compressed_data_hex,
            "fd377a585a000004e6d6b4460200210116000000742fe5a301000e2f7573722f6c"
            "6f63616c2f6c6c766d0000efb8e6d242765b590001270fdf1afc6a1fb6f37d0100"
            "00000004595a");

  if (!Util::LZMADecompress(compressed_data, &decompressed_data)) {
    LOG(ERROR) << "decompress error";
  }

  EXPECT_EQ(decompressed_data, "/usr/local/llvm");
}

}  // namespace util
}  // namespace oceandoc
