/*******************************************************************************
 * Copyright (c) 2024  xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/util/util.h"

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <thread>

// #include "fmt/core.h"
#include "glog/logging.h"
#include "gtest/gtest.h"
#include "src/common/defs.h"
#include "src/proto/service.pb.h"
#include "src/test/test_util.h"

namespace oceandoc {
namespace util {

// TEST(Util, PrintAllEnv) { Util::PrintAllEnv(); }

TEST(Util, CurrentTimeMillis) {
  EXPECT_GT(Util::CurrentTimeMillis(), 1704038400000);
  EXPECT_LT(Util::CurrentTimeMillis(), 1904038400000);
}

TEST(Util, CurrentTimeNanos) {
  EXPECT_GT(Util::CurrentTimeNanos(), 1727101022292387983);
  EXPECT_LT(Util::CurrentTimeNanos(), 1927101022292387983);
}

// NOTICE: Cannot combine %E#S and %E#f
// NOTICE: bazel run/test will use UTC time zone as default
// and bazel just provide --test_env=TZ=Asia/Shanghai, not have --run_env
// env TZ=Asia/Shanghai bazel run //xxx:target
// https://www.epochconverter.com
TEST(Util, StrToTimeStamp) {
  absl::TimeZone tz;
  if (!absl::LoadTimeZone("UTC", &tz)) {
    LOG(ERROR) << "Load time zone error";
  }
  EXPECT_EQ(tz.name(), "UTC");

  std::string time = "2024-09-24 13:36:44";
  std::string format = "%Y-%m-%d HH:MM:SS";
  EXPECT_EQ(Util::StrToTimeStampUTC(time, format), -1);

  time = "2024-09-24 13:36:44";
  format = "%Y-%m-%d %H:%M:%S";
  EXPECT_EQ(Util::StrToTimeStampUTC(time, format), 1727185004000);
  EXPECT_EQ(Util::StrToTimeStamp(time, format, "UTC"), 1727185004000);
  EXPECT_EQ(Util::StrToTimeStamp(time, format, "Asia/Shanghai"), 1727156204000);
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::StrToTimeStampLocal(time, format), 1727156204000);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::StrToTimeStampUTC(time, format), 1727185004000);
  }

  time = "2024-09-24 13:36:44.123";
  format = "%Y-%m-%d %H:%M:%E3S";
  EXPECT_EQ(Util::StrToTimeStampUTC(time, format), 1727185004123);
  EXPECT_EQ(Util::StrToTimeStamp(time, format, "UTC"), 1727185004123);
  EXPECT_EQ(Util::StrToTimeStamp(time, format, "Asia/Shanghai"), 1727156204123);
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::StrToTimeStampLocal(time, format), 1727156204123);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::StrToTimeStampUTC(time, format), 1727185004123);
  }

  time = "2024-09-24 13:36:44.123";
  format = "%Y-%m-%d %H:%M:%E2S%E3f";
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::StrToTimeStampLocal(time, format), 1727156204123);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::StrToTimeStampUTC(time, format), 1727185004123);
  }

  time = "2024-09-24 13:36:44.123";
  format = "%Y-%m-%d %H:%M:%E3S";
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::StrToTimeStampLocal(time, format), 1727156204123);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::StrToTimeStampUTC(time, format), 1727185004123);
  }

  time = "2024-09-24T13:36:44.123+00:00";
  format = "%Y-%m-%d%ET%H:%M:%E2S%E3f%Ez";
  EXPECT_EQ(Util::StrToTimeStampUTC(time, format), 1727185004123);
  EXPECT_EQ(Util::StrToTimeStampLocal(time, format), 1727185004123);

  time = "2024-09-24 13:36:44.123+08:00:00";
  format = "%Y-%m-%d %H:%M:%E3S%E*z";
  EXPECT_EQ(Util::StrToTimeStampUTC(time, format), 1727156204123);
}

TEST(Util, ToTimeStr) {
  int64_t ts = 1727185004123;
  std::string format = "%Y-%m-%dT%H:%M:%S%z %Z";
  std::string time = "2024-09-24T21:36:44+0800 CST";
  std::string utc_time = "2024-09-24T13:36:44+0000 UTC";

  EXPECT_EQ(Util::ToTimeStr(ts, format, "Asia/Shanghai"), time);
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::ToTimeStrLocal(ts, format), time);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::ToTimeStrLocal(ts, format), utc_time);
  }
  EXPECT_EQ(Util::ToTimeStrUTC(ts, format), utc_time);

  // %ET:T, %Ez:%z
  format = "%Y-%m-%d%ET%H:%M:%S%Ez %Z";
  time = "2024-09-24T21:36:44+08:00 CST";
  utc_time = "2024-09-24T13:36:44+00:00 UTC";
  EXPECT_EQ(Util::ToTimeStr(ts, format, "Asia/Shanghai"), time);
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::ToTimeStrLocal(ts, format), time);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::ToTimeStrLocal(ts, format), utc_time);
  }
  EXPECT_EQ(Util::ToTimeStrUTC(ts, format), utc_time);

  // %E3f
  format = "%Y-%m-%d%ET%H:%M:%S.%E3f%Ez %Z";
  time = "2024-09-24T21:36:44.123+08:00 CST";
  utc_time = "2024-09-24T13:36:44.123+00:00 UTC";
  EXPECT_EQ(Util::ToTimeStr(ts, format, "Asia/Shanghai"), time);
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::ToTimeStrLocal(ts, format), time);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::ToTimeStrLocal(ts, format), utc_time);
  }
  EXPECT_EQ(Util::ToTimeStrUTC(ts, format), utc_time);

  // %E3S:%S.%E3f
  format = "%Y-%m-%d%ET%H:%M:%E3S%Ez %Z";
  time = "2024-09-24T21:36:44.123+08:00 CST";
  utc_time = "2024-09-24T13:36:44.123+00:00 UTC";
  EXPECT_EQ(Util::ToTimeStr(ts, format, "Asia/Shanghai"), time);
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::ToTimeStrLocal(ts, format), time);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::ToTimeStrLocal(ts, format), utc_time);
  }
  EXPECT_EQ(Util::ToTimeStrUTC(ts, format), utc_time);
}

TEST(Util, ToTimeSpec) {
  auto time = Util::ToTimeSpec(2727650275042);
  EXPECT_EQ(time.tv_sec, 2727650275);
  EXPECT_EQ(time.tv_nsec, 42000000);
}

TEST(Util, Random) {
  auto generator = [](int /*thread_num*/) {
    for (int i = 0; i < 10000; ++i) {
      auto ret = Util::Random(0, 100);
      EXPECT_GT(ret, -1);
      EXPECT_LT(ret, 100);
    }
  };

  std::thread threads[12];
  for (int i = 0; i < 12; ++i) {
    threads[i] = std::thread(std::bind(generator, i));
  }

  for (int i = 0; i < 12; ++i) {
    if (threads[i].joinable()) {
      threads[i].join();
    }
  }
}

TEST(Util, UnifyDir) {
  std::string path = "/";
  Util::UnifyDir(&path);
  EXPECT_EQ(path, "/");

  path = "/data";
  Util::UnifyDir(&path);
  EXPECT_EQ(path, "/data");

  path = "/data/";
  Util::UnifyDir(&path);
  EXPECT_EQ(path, "/data");

  EXPECT_EQ(Util::UnifyDir("/"), "/");
  EXPECT_EQ(Util::UnifyDir("/data"), "/data");
  EXPECT_EQ(Util::UnifyDir("/data/"), "/data");
  EXPECT_EQ(Util::UnifyDir("/data//test/"), "/data/test");
}

TEST(Util, SetUpdateTime) {
  auto runfile_dir = Util::GetEnv("TEST_SRCDIR");
  auto workspace_name = Util::GetEnv("TEST_WORKSPACE");
  const auto& path = "test_data/util_test/target";
  std::string final_path = path;
  if (runfile_dir.has_value()) {
    final_path = std::string(*runfile_dir) + "/" +
                 std::string(*workspace_name) + "/" + path;
  }
  EXPECT_EQ(Util::SetUpdateTime(final_path, 2727650275042), true);
}

TEST(Util, UpdateTime) {
  const auto& path = "test_data/util_test/target";
  EXPECT_EQ(Util::UpdateTime(path), 2727650275042);
}

TEST(Util, FileSize) {
  // echo "test" > txt will add a \n to file automatic, vim has same behavior
  std::string path = "test_data/util_test/test1/test2/symlink_to_target";
  if (test::Util::IsBazelTest()) {
    std::string target_path =
        test::Util::ExecRoot() + "/" + test::Util::Workspace() + "/" + path;
    EXPECT_EQ(Util::FileSize(path), target_path.size());
    EXPECT_EQ(Util::FileSize("test_data/util_test/target"), 108);
    EXPECT_EQ(Util::FileSize("test_data"), 4096);
  } else if (test::Util::IsBazelRun()) {
    std::string target_path = Util::CurrentPath() + "/" + path;
    EXPECT_EQ(Util::FileSize(path), target_path.size());
    EXPECT_EQ(Util::FileSize("test_data/util_test/target"), 5);
  } else {
    EXPECT_EQ(Util::FileSize(path), 12);
    EXPECT_EQ(Util::FileSize("test_data/util_test/target"), 5);
  }
}

TEST(Util, FileInfo) {
  std::string path = "test_data/util_test/target";
  int64_t update_time = -1, size = -1;
  std::string user, group;
  auto runfile_dir = Util::GetEnv("TEST_SRCDIR");
  if (runfile_dir.has_value()) {
    EXPECT_EQ(Util::FileInfo(path, &update_time, &size, &user, &group), true);
    EXPECT_EQ(update_time, 2727650275042);
    EXPECT_EQ(size, 108);
  } else {
    EXPECT_EQ(Util::FileInfo(path, &update_time, &size, &user, &group), true);
    EXPECT_EQ(update_time, 2727650275042);
    EXPECT_EQ(size, 5);
  }
}

TEST(Util, Path) {
  std::string path = "/usr/local/";
  EXPECT_EQ(std::filesystem::path(path).string(), "/usr/local/");

  path = "/usr/local";
  EXPECT_EQ(std::filesystem::path(path).string(), "/usr/local");

  path = "test_data/util_test/test1/test2/symlink_to_target";
  std::filesystem::path s_symlink(path);
  std::filesystem::path s_target("test_data/util_test/target");
  EXPECT_EQ(std::filesystem::equivalent(s_symlink, s_target), true);
  EXPECT_EQ(s_symlink == s_target, false);

  s_symlink = "test_data/util_test/test1/test2/symlink_to_target_dir";
  s_target = "test_data/util_test/target_dir";
  // will coredump
  // EXPECT_EQ(std::filesystem::equivalent(s_symlink, s_target), true);
  EXPECT_EQ(s_symlink == s_target, false);

  s_symlink = "test_data/util_test/test1/test2/symlink_to_target";
  EXPECT_EQ(s_symlink.parent_path().string(),
            "test_data/util_test/test1/test2");
}

TEST(Util, Exists) {
  std::string path = "test_data/util_test/test1/test2/target_not_exist";
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::Exists(path), false);
    EXPECT_EQ(std::filesystem::exists(path), false);
  } else if (test::Util::IsBazelRun()) {
    EXPECT_EQ(Util::Exists(path), false);
    EXPECT_EQ(std::filesystem::exists(path), false);
  } else {
  }
}

TEST(Util, CreateFileWithSize) {
  auto path = "test_data/util_test/tes_create_with_size";
  EXPECT_EQ(Util::CreateFileWithSize(path, 5), proto::ErrCode::Success);
  EXPECT_EQ(Util::Exists(path), true);
  EXPECT_EQ(Util::FileSize(path), 5);
  EXPECT_EQ(Util::Remove(path), true);
}

TEST(Util, FindCommonRoot) {
  std::filesystem::path path =
      "test_data/util_test/test1/test2/symlink_to_target";
  std::filesystem::path base =
      "test_data/util_test/test1/test2/symlink_to_target";
  EXPECT_EQ(path.string(), "test_data/util_test/test1/test2/symlink_to_target");

  auto ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, base.string());

  path = "test_data/util_test/test1/test2/symlink_to_target";
  base = "test_data/util_test/test1/test2";
  ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, "test_data/util_test/test1/test2");

  path = "test_data/util_test/test1/test2/symlink_to_target";
  base = "test1";
  ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, "");

  path = "test1";
  base = "test_data/util_test/test1/test2/symlink_to_target";
  ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, "");

  path = "/";
  base = "/";
  ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, "/");

  path = "/";
  base = "/base";
  ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, "/");

  path = "/test";
  base = "/base";
  ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, "/");

  path = "/base/test";
  base = "/base";
  ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, "/base");

  path = "base/test";
  base = "/base";
  ret = Util::FindCommonRoot(path, base);
  EXPECT_EQ(ret, "");
}

TEST(Util, Relative) {
  // std::filesystem::relative don't need file exists
  // std::filesystem::relative will follow symlink

  ////////////////////////////////////////////////////////////////
  std::string relative;
  EXPECT_EQ(Util::Relative("base/test", "/base", &relative), false);
  EXPECT_EQ(relative, "");

  EXPECT_EQ(Util::Relative("/usr/xxxx/llvm", "/usr/xxxx", &relative), true);
  EXPECT_EQ(relative, "llvm");

  EXPECT_EQ(Util::Relative("/usr/local/", "/usr/local/llvm", &relative), true);
  EXPECT_EQ(relative, "..");

  EXPECT_EQ(Util::Relative("/usr/local/llvm", "/usr/local", &relative), true);
  EXPECT_EQ(relative, "llvm");

  EXPECT_EQ(Util::Relative("/usr/local", "/usr/local", &relative), true);
  EXPECT_EQ(relative, "");

  EXPECT_EQ(Util::Relative("/usr/xxxx", "/usr/local", &relative), true);
  EXPECT_EQ(relative, "../xxxx");

  EXPECT_EQ(Util::Relative("/usr/test1/test2/xxxx", "/usr/xxxx1/xxxx2/local",
                           &relative),
            true);
  EXPECT_EQ(relative, "../../../test1/test2/xxxx");

  // Util::Relative will follow symlink
  EXPECT_EQ(Util::Relative("test_data/util_test/test1/test2/symlink_to_target",
                           "test_data/util_test/symlink_test", &relative),
            true);
  EXPECT_EQ(relative, "../test1/test2/symlink_to_target");
}

TEST(Util, ParentPath) {
  std::string relative = "18";
  EXPECT_EQ(Util::ParentPath(relative), "");

  relative = "18/test";
  EXPECT_EQ(Util::ParentPath(relative), "18");
}

TEST(Util, TruncateFile) {
  EXPECT_EQ(Util::Exists("test_data/util_test/txt"), false);
  EXPECT_EQ(Util::CreateFileWithSize("test_data/util_test/txt", 5),
            proto::ErrCode::Success);
  EXPECT_EQ(Util::FileSize("test_data/util_test/txt"), 5);
  EXPECT_EQ(Util::TruncateFile("test_data/util_test/txt"), true);
  EXPECT_EQ(Util::FileSize("test_data/util_test/txt"), 0);
  EXPECT_EQ(Util::WriteToFile("test_data/util_test/txt", "test\n"),
            proto::ErrCode::Success);
  EXPECT_EQ(Util::FileSize("test_data/util_test/txt"), 5);
  EXPECT_EQ(Util::Remove("test_data/util_test/txt"), true);
}

TEST(Util, WriteToFile) {
  std::string path = "test_data/util_test/txt";
  EXPECT_EQ(Util::Exists(path), false);
  EXPECT_EQ(Util::CreateFileWithSize(path, 200), proto::ErrCode::Success);
  EXPECT_EQ(Util::Exists(path), true);
  EXPECT_EQ(Util::WriteToFile(path, "test", (int64_t)100), Err_Success);
  std::string sha256;
  EXPECT_EQ(Util::SmallFileSHA256(path, &sha256), true);
  EXPECT_EQ(sha256,
            "b979494e28d88b1fa87f19a3a5632b93f67e315208b1141404dd5a97778a8367");
  EXPECT_EQ(Util::Remove(path), true);
}

TEST(Util, LoadSmallFile) {
  std::string content;
  std::string path = "test_data/util_test/never_modify";
  EXPECT_EQ(Util::LoadSmallFile(path, &content), true);
  EXPECT_EQ(content, "abcd\n");
}

TEST(Util, SyncSymlink) {
  std::string src = "test_data/util_test";
  std::string dst = "test_data/util_test/test";
  std::string src_symlink = "test_data/util_test/test1/test2/symlink_to_target";
  std::string dst_symlink =
      "test_data/util_test/test/test1/test2/symlink_to_target";
  EXPECT_EQ(Util::Remove(dst_symlink), true);
  EXPECT_EQ(Util::Exists(dst_symlink), false);
  EXPECT_EQ(Util::SyncSymlink(src, dst, src_symlink), true);
  if (!test::Util::IsBazelTest()) {
    EXPECT_EQ(Util::Exists(dst_symlink), true);
    std::string src_target =
        std::filesystem::read_symlink(src_symlink).string();
    std::string dst_target =
        std::filesystem::read_symlink(dst_symlink).string();
    EXPECT_EQ(src_target, dst_target);
  }
}

TEST(Util, PrepareFile) {
  common::FileAttr attr;
  Util::PrepareFile("test_data/util_test/target",
                    common::HashMethod::Hash_BLAKE3,
                    common::NET_BUFFER_SIZE_BYTES, &attr);
  EXPECT_EQ(attr.file_hash,
            "dea2b412aa90f1b43a06ca5e8b8feafec45ae1357971322749480f4e1572eaa2");
  EXPECT_EQ(attr.partition_num, 1);
  if (test::Util::IsBazelTest()) {
    EXPECT_EQ(attr.file_size, 108);
  } else {
    EXPECT_EQ(attr.file_size, 5);
  }
}

TEST(Util, SimplifyPath) {
  std::string result;
  EXPECT_EQ(Util::SimplifyPath("/a/b/../c/./d/..", &result), true);
  EXPECT_EQ(result, "/a/c");

  result.clear();
  EXPECT_EQ(Util::SimplifyPath("a/b/../c/./d/..", &result), true);
  EXPECT_EQ(result, "a/c");

  result.clear();
  EXPECT_EQ(Util::SimplifyPath("a/b/../c/./d/../../../..", &result), false);
  EXPECT_EQ(result, "");

  result.clear();
  EXPECT_EQ(Util::SimplifyPath("/a/../..", &result), false);
  EXPECT_EQ(result, "");

  result.clear();
  EXPECT_EQ(Util::SimplifyPath("a/..", &result), true);
  EXPECT_EQ(result, "");

  result.clear();
  EXPECT_EQ(Util::SimplifyPath("..", &result), false);
  EXPECT_EQ(result, "");

  result.clear();
  EXPECT_EQ(Util::SimplifyPath("../..", &result), false);
  EXPECT_EQ(result, "");
}

TEST(Util, RepoFilePath) {
  EXPECT_EQ(
      Util::RepoFilePath(
          "test_data/util_test",
          "f2ca1bb6c7e907d06dafe4687e579fce76b37e4e93b7605022da52e6ccc26fd2"),
      "test_data/util_test/f2/ca/"
      "f2ca1bb6c7e907d06dafe4687e579fce76b37e4e93b7605022da52e6ccc26fd2");
}

TEST(Util, CalcPartitionStart) {
  int64_t start = 0, end = 0;
  Util::CalcPartitionStart(359621552, 0, common::NET_BUFFER_SIZE_BYTES, &start,
                           &end);
  EXPECT_EQ(start, 0);
  EXPECT_EQ(end, common::NET_BUFFER_SIZE_BYTES - 1);

  Util::CalcPartitionStart(359621552, 1, common::NET_BUFFER_SIZE_BYTES, &start,
                           &end);
  EXPECT_EQ(start, common::NET_BUFFER_SIZE_BYTES);
  EXPECT_EQ(end, common::NET_BUFFER_SIZE_BYTES * 2 - 1);

  Util::CalcPartitionStart(359621552, 6, common::NET_BUFFER_SIZE_BYTES, &start,
                           &end);
  EXPECT_EQ(start, common::NET_BUFFER_SIZE_BYTES * 6);
  EXPECT_EQ(end, common::NET_BUFFER_SIZE_BYTES * 7 - 1);
}

///////////////////////////////////////////////////

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
  content =
      R"(A cyclic redundancy check (CRC) is an error-detecting code used to detect data corruption. When sending data, short checksum is generated based on data content and sent along with data. When receiving data, checksum is generated again and compared with sent checksum. If the two are equal, then there is no data corruption. The CRC-32 algorithm itself converts a variable-length string into an 8-character string.)";
  Util::SHA256(content, &out);
  EXPECT_EQ(out,
            "3f5d419c0386a26df1c75d0d1c488506fb641b33cebaa2a4917127ae33030b31");
}

TEST(Util, Blake3) {
  std::string content;
  std::string out;
  content =
      R"(A cyclic redundancy check (CRC) is an error-detecting code used to detect data corruption. When sending data, short checksum is generated based on data content and sent along with data. When receiving data, checksum is generated again and compared with sent checksum. If the two are equal, then there is no data corruption. The CRC-32 algorithm itself converts a variable-length string into an 8-character string.)";
  Util::Blake3(content, &out);
  EXPECT_EQ(out,
            "9b12d05351596e6851917bc73dcaf39eb12a27a196e56d38492ed730a60edf8e");
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

TEST(Util, JsonToMessage) {
  proto::FileReq req;

  std::string serialized =
      R"({"request_id":"","op":"FilePut","path":"tesxt","hash":"abbc","size":50,"content":"dGVzdA==","partition_num":0,"repo_uuid":"/tmp/test_repo"})";
  if (!Util::JsonToMessage(serialized, &req)) {
    LOG(ERROR) << "Req to json error: " << serialized;
  }
  EXPECT_EQ(req.op(), 2);
}

TEST(Util, HashPassword) {
  std::string salt = "452c0306730b0f3ac3086d4f62effc20";
  std::string standard_hashed_password =
      "e64de2fcaef0b98d035c3c241e4f8fda32f3b09067ef0f1b1706869a54f9d3b7";

  std::string hashed_password;
  if (!Util::HashPassword(Util::SHA256("admin"), salt, &hashed_password)) {
    LOG(INFO) << "error";
  }
  LOG(INFO) << hashed_password;

  EXPECT_EQ(standard_hashed_password, hashed_password);
}

}  // namespace util
}  // namespace oceandoc
