/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/util.h"

#include "glog/logging.h"
#include "gtest/gtest.h"

namespace oceandoc {
namespace util {

TEST(Util, DetailTimeStr) {
  EXPECT_EQ(Util::DetailTimeStr(1646397312000),
            "2022-03-04T20:35:12.000+08:00");
  LOG(INFO) << Util::DetailTimeStr(1646397312000);
}

TEST(Util, UpdateTime) {
  const auto& path = "/usr/include/x86_64-linux-gnu/openmpi";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  EXPECT_EQ(Util::UpdateTime(path), 1646397312000);
  LOG(INFO) << Util::UpdateTime(path);
}

TEST(Util, CreateTime) {
  auto path = "/usr/include/x86_64-linux-gnu/openmpi";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  EXPECT_EQ(Util::CreateTime(path), 1720900552965);
  LOG(INFO) << Util::CreateTime(path);
}

TEST(Util, FileSize) {
  auto path = "/usr/include/x86_64-linux-gnu/openmpi";
  if (std::filesystem::is_symlink(path)) {
    LOG(INFO) << "is symlink";
  }
  EXPECT_EQ(Util::FileSize(path), 42);
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
  auto start = Util::Now();
  auto crc = Util::CRC32(content);
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, crc32:" << crc << ", cost: " << Util::Now() - start;
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
  auto start = Util::Now();
  Util::SHA256(content, &out);
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, sha256:" << out << ", cost: " << Util::Now() - start;

  start = Util::Now();
  Util::SHA256_libsodium(content, &out);
  LOG(INFO) << "file size: " << content.size() / 1024 / 1024
            << "M, sha256:" << out << ", cost: " << Util::Now() - start;
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

}  // namespace util
}  // namespace oceandoc
