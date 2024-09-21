/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/util.h"

#include <fstream>

#include "glog/logging.h"
#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

using namespace std;

TEST(Util, DetailTimeStr) {
  EXPECT_EQ(Util::ToTimeStr(1646397312000), "2022-03-04T20:35:12.000+08:00");
  LOG(INFO) << Util::ToTimeStr(1646397312000);
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

TEST(Util, UUID) { LOG(INFO) << Util::UUID(); }

TEST(Util, CRC32) {
  std::string content;
  Util::LoadSmallFile(
      "/usr/local/gcc/14.1.0/libexec/gcc/x86_64-pc-linux-gnu/14.1.0/cc1plus",
      &content);
  auto start = Util::CurrentTimeMillis();
  auto crc = Util::CRC32(content);
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, crc32:" << crc
            << ", cost: " << Util::CurrentTimeMillis() - start;
  content =
      "A cyclic redundancy check (CRC) is an error-detecting code used to "
      "detect data corruption. When sending data, short checksum is generated "
      "based on data content and sent along with data. When receiving data, "
      "checksum is generated again and compared with sent checksum. If the two "
      "are equal, then there is no data corruption. The CRC-32 algorithm "
      "itself converts a variable-length string into an 8-character string.";
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, crc32:" << Util::CRC32(content);
  EXPECT_EQ(Util::CRC32(content), 2331864810);
}

TEST(Util, SHA256) {
  std::string content;
  Util::LoadSmallFile(
      "/usr/local/gcc/14.1.0/libexec/gcc/x86_64-pc-linux-gnu/14.1.0/cc1plus",
      &content);
  std::string out;
  auto start = Util::CurrentTimeMillis();
  Util::SHA256(content, &out);
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, sha256:" << out
            << ", cost: " << Util::CurrentTimeMillis() - start;

  start = Util::CurrentTimeMillis();
  Util::SHA256_libsodium(content, &out);
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, sha256:" << out
            << ", cost: " << Util::CurrentTimeMillis() - start;
  content =
      "A cyclic redundancy check (CRC) is an error-detecting code used to "
      "detect data corruption. When sending data, short checksum is generated "
      "based on data content and sent along with data. When receiving data, "
      "checksum is generated again and compared with sent checksum. If the two "
      "are equal, then there is no data corruption. The CRC-32 algorithm "
      "itself converts a variable-length string into an 8-character string.";
  Util::SHA256(content, &out);
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, sha256:" << out;
  EXPECT_EQ(out,
            "3f5d419c0386a26df1c75d0d1c488506fb641b33cebaa2a4917127ae33030b31");
}

TEST(Util, Relative) {
  std::string path = "/usr/local/llvm/18/bin/llvm-dlltool";
  std::string base = "/usr/local/llvm";
  std::string relative;
  EXPECT_EQ(Util::Relative(path, base, &relative), true);
  EXPECT_EQ(relative, "18/bin/llvm-dlltool");

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

TEST(Util, Remove) {
  std::string path = "test_data/util_test/test_remove";
  std::ofstream ofs(path);
  ofs.close();
  EXPECT_EQ(Util::Remove("test_data/util_test/test_remove"), true);
  ofstream ofs1(path);
  ofs1.close();
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
