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
#include <string_view>
#include <thread>
#include <utility>

#include "absl/time/clock.h"
#include "boost/algorithm/string/predicate.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/trim_all.hpp"
#include "boost/iostreams/device/mapped_file.hpp"
#include "fmt/core.h"
#include "glog/logging.h"
#include "google/protobuf/util/json_util.h"
#include "openssl/md5.h"
#include "src/MurmurHash2.h"
#include "src/MurmurHash3.h"

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

int64_t Util::NanoTime() { return GetCurrentTimeNanos(); }

uint64_t Util::Now() {
  auto duration_since_epoch =
      std::chrono::system_clock::now().time_since_epoch();
  auto microseconds_since_epoch =
      std::chrono::duration_cast<std::chrono::microseconds>(
          duration_since_epoch)
          .count();
  time_t seconds_since_epoch =
      static_cast<time_t>(microseconds_since_epoch / 1000000);  // second
  return seconds_since_epoch;
}

uint64_t Util::StrTimeToTimestamp(const string &time, int32_t offset) {
  absl::Time t;
  string err;
  if (!absl::ParseTime("%Y-%m-%d%ET%H:%M:%E3S%Ez", time, &t, &err)) {
    LOG(ERROR) << "convert " << time << " " << err;
    return 0;
  }
  return absl::ToUnixMillis(t) + offset;
}

string Util::ToTimeStr(const int64_t ts) {
  TimeZone time_zone;
  LoadTimeZone("Asia/Shanghai", &time_zone);
  return FormatTime("%Y-%m-%d %H:%M:%S", FromUnixMillis(ts), time_zone);
}

string Util::TodayStr() {
  absl::Time now = absl::Now();
  absl::TimeZone loc = absl::LocalTimeZone();
  return absl::FormatTime("%Y-%m-%d", now, loc);
}

string Util::DetailTimeStr() {
  absl::Time now = absl::Now();
  TimeZone time_zone;
  LoadTimeZone("Asia/Shanghai", &time_zone);
  return absl::FormatTime("%Y-%m-%d%ET%H:%M:%E3S%Ez", now, time_zone);
}

string Util::DetailTimeStr(const int64_t ts) {
  TimeZone time_zone;
  LoadTimeZone("Asia/Shanghai", &time_zone);
  return absl::FormatTime("%Y-%m-%d%ET%H:%M:%E3S%Ez", FromUnixMillis(ts),
                          time_zone);
}

int64_t Util::Random(int64_t start, int64_t end) {
  static thread_local std::mt19937 generator(NanoTime());
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

bool Util::IsDir(const string &path) {
  std::filesystem::path file_path(path);
  return std::filesystem::is_directory(file_path);
}

bool Util::IsFile(const string &dir, const string &file) {
  std::filesystem::path file_path(dir + "/" + file);
  return std::filesystem::is_regular_file(file_path);
}

string Util::DirName(const string &path) {
  std::filesystem::path file_path(path);
  return file_path.parent_path().string();
}

string Util::BaseName(const string &path) {
  std::filesystem::path file_path(path);
  return file_path.filename().string();
}

std::string Util::FileExtention(const std::string &path) {
  std::filesystem::path file_path(path);
  return file_path.extension().string();
}

bool Util::Remove(const string &dir, const string &file) {
  string path;
  if (dir.empty()) {
    path = file;
  } else if (dir[dir.size() - 1] == '/') {
    path = dir + file;
  } else {
    path = dir + "/" + file;
  }

  try {
    if (std::filesystem::exists(path)) {
      return std::filesystem::remove(path);
    }
  } catch (std::exception &e) {
    LOG(ERROR) << e.what();
    return false;
  }

  return true;
}

bool Util::Remove(const string &dir) {
  try {
    if (std::filesystem::exists(dir)) {
      return std::filesystem::remove_all(dir);
    }
  } catch (const std::filesystem::filesystem_error &e) {
    LOG(ERROR) << e.what();
    return false;
  }

  return true;
}

bool Util::Mkdir(const string &dir) {
  try {
    std::filesystem::create_directories(dir);
  } catch (const std::exception &e) {
    LOG(ERROR) << "Mkdir " << dir << " error";
    return false;
  }
  return true;
}

bool Util::Exist(const string &path) { return std::filesystem::exists(path); }

string Util::TruncatePath(const string &src, const string &path) {
  string real_path = std::filesystem::canonical(src).string();
  std::string::size_type n = real_path.find(path);
  if (n != string::npos) {
    return real_path.substr(0, n);
  }
  return "";
}

string Util::RealPath(const string &path) {
  return std::filesystem::canonical(path).string();
}

bool Util::ListDir(const string &path, std::vector<string> *files,
                   const bool recursive) {
  if (!files) {
    return true;
  }
  try {
    std::filesystem::path dir_path(path);
    if (!std::filesystem::is_directory(dir_path)) {
      return false;
    }
    if (recursive) {
      std::filesystem::recursive_directory_iterator it(dir_path);
      std::filesystem::recursive_directory_iterator end;
      for (; it != end; ++it) {
        files->emplace_back(it->path().string());
      }
    } else {
      std::filesystem::directory_iterator it(dir_path);
      std::filesystem::directory_iterator end;
      for (; it != end; ++it) {
        files->emplace_back(it->path().string());
      }
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what();
    return false;
  }
  return true;
}

bool Util::ListFile(const string &path, std::vector<string> *files,
                    const bool recursive) {
  if (!files) {
    return true;
  }
  try {
    std::filesystem::path dir_path(path);
    if (!std::filesystem::is_directory(dir_path)) {
      return false;
    }
    if (recursive) {
      std::filesystem::recursive_directory_iterator it(dir_path);
      std::filesystem::recursive_directory_iterator end;
      for (; it != end; ++it) {
        if (std::filesystem::is_regular_file(it->status())) {
          files->emplace_back(it->path().string());
        }
      }
    } else {
      std::filesystem::directory_iterator it(dir_path);
      std::filesystem::directory_iterator end;
      for (; it != end; ++it) {
        if (std::filesystem::is_regular_file(it->status())) {
          files->emplace_back(it->path().string());
        }
      }
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what();
    return false;
  }
  return true;
}

bool Util::ListFile(const string &path,
                    std::vector<std::filesystem::path> *files,
                    const std::string &name_pattern,
                    const std::string &ignore_ext, const bool recursive) {
  if (!files) {
    return true;
  }

  try {
    std::filesystem::path dir_path(path);
    if (!std::filesystem::is_directory(dir_path)) {
      LOG(ERROR) << path << " not directory";
      return false;
    }

    if (recursive) {
      std::filesystem::recursive_directory_iterator it(dir_path);
      std::filesystem::recursive_directory_iterator end;
      for (; it != end; ++it) {
        if (std::filesystem::is_regular_file(it->status()) &&
            it->path().extension().string() != ignore_ext) {
          if (!name_pattern.empty() &&
              it->path().string().find(name_pattern) == string::npos) {
            continue;
          }
          files->emplace_back(it->path().string());
        }
      }
    } else {
      std::filesystem::directory_iterator it(dir_path);
      std::filesystem::directory_iterator end;
      for (; it != end; ++it) {
        if (std::filesystem::is_regular_file(it->status()) &&
            it->path().extension().string() != ignore_ext) {
          if (!name_pattern.empty() &&
              it->path().string().find(name_pattern) == string::npos) {
            continue;
          }
          files->emplace_back(it->path());
        }
      }
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what();
    return false;
  }

  return true;
}

bool Util::CopyFile(const string &src_file_path, const string dst_file_path,
                    const std::filesystem::copy_options opt) {
  try {
    return std::filesystem::copy_file(src_file_path, dst_file_path, opt);
  } catch (std::exception &e) {
    LOG(ERROR) << "copy " << src_file_path << " to " << dst_file_path
               << " exception, " << e.what();
  }
  return false;
}

bool Util::Copy(const string &src_path, const string dst_path) {
  try {
    std::filesystem::copy(src_path, dst_path,
                          std::filesystem::copy_options::recursive);
  } catch (std::exception &e) {
    LOG(ERROR) << "copy " << src_path << " to " << dst_path << " exception, "
               << e.what();
    return false;
  }
  return true;
}

bool Util::WriteToFile(std::filesystem::path path, const string &content,
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
  } catch (std::exception &e) {
    LOG(ERROR) << (append ? "Write to " : "Append to ") << path.string()
               << ", error: " << e.what();
    return false;
  }

  return true;
}

std::istream &Util::GetLine(std::istream &is, std::string *line) {
  line->clear();
  std::istream::sentry se(is, true);
  std::streambuf *sb = is.rdbuf();

  for (;;) {
    int c = sb->sbumpc();
    switch (c) {
      case '\n':
        return is;
      case '\r':
        if (sb->sgetc() == '\n') sb->sbumpc();
        return is;
      case std::streambuf::traits_type::eof():
        if (line->empty()) is.setstate(std::ios::eofbit);
        return is;
      default:
        line->push_back(static_cast<char>(c));
    }
  }
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

std::vector<string> Util::LoadLines(const string &file_name) {
  std::ifstream in(file_name);
  if (!in) {
    LOG(ERROR) << "Fail to open " << file_name;
    return std::vector<string>();
  }
  std::vector<string> ret;
  string line;
  while (getline(in, line)) {
    Util::Trim(&line);
    if (line.empty()) {
      continue;
    }
    if (line[0] == '#') {
      continue;
    }
    ret.emplace_back(std::move(line));
  }
  in.close();
  return ret;
}

string Util::FileMd5(const string &file_path) {
  unsigned char result[MD5_DIGEST_LENGTH];
  boost::iostreams::mapped_file_source src(file_path);
  MD5((unsigned char *)src.data(), src.size(), result);
  std::ostringstream sout;
  sout << std::hex << std::setfill('0');
  for (auto c : result) sout << std::setw(2) << static_cast<int>(c);

  return sout.str();
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

void Util::ReplaceAll(string *s, const string &from, const string &to) {
  boost::algorithm::replace_all(*s, from, to);
}

void Util::ReplaceAll(string *s, const string &from, const char *const to) {
  boost::algorithm::replace_all(*s, from, to);
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

string Util::ToString(const std::set<uint64_t> &ids) {
  string ret;
  ret.resize(512);
  bool first = true;
  for (auto id : ids) {
    if (first) {
      first = false;
      ret.append(std::to_string(id));
      continue;
    }
    ret.append(",");
    ret.append(std::to_string(id));
  }
  return ret;
}

std::string Util::ToString(const std::map<string, string> &vars) {
  string ret;
  ret.resize(512);
  bool first = true;
  for (auto &p : vars) {
    if (first) {
      first = false;
      ret.append(p.first);
      ret.append("=");
      ret.append(p.second);
      continue;
    }
    ret.append("&");
    ret.append(p.first);
    ret.append("=");
    ret.append(p.second);
  }
  return ret;
}

string Util::StandardBase64Encode(const string &input) {
  string output;
  // try {
  // output.resize(boost::beast::detail::base64::encoded_size(input.size()));
  // auto const ret = boost::beast::detail::base64::encode(
  // output.data(), input.data(), input.size());
  // output.resize(ret);
  //} catch (exception &e) {
  // LOG(INFO) << "Base64 encode error";
  //}
  return output;
}

string Util::StandardBase64Decode(const string &input) {
  string output;
  // try {
  // output.resize(boost::beast::detail::base64::decoded_size(input.size()));
  // auto const ret = boost::beast::detail::base64::decode(
  // output.data(), input.data(), input.size());
  // output.resize(ret.first);
  //} catch (exception &e) {
  // LOG(INFO) << "Base64 decode error";
  //}
  return output;
}

static const char *base64_chars[2] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-_"};

static unsigned int CharPos(const unsigned char chr) {
  if (chr >= 'A' && chr <= 'Z')
    return chr - 'A';
  else if (chr >= 'a' && chr <= 'z')
    return chr - 'a' + ('Z' - 'A') + 1;
  else if (chr >= '0' && chr <= '9')
    return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
  else if (chr == '+' || chr == '-')
    return 62;  // Be liberal with input and accept both url ('-') and non-url
                // ('+') base 64 characters (
  else if (chr == '/' || chr == '_')
    return 63;  // Ditto for '/' and '_'
  else
    throw std::runtime_error("Input is not valid base64-encoded data.");
}

static std::string InsertLinebreaks(std::string str, size_t distance) {
  if (!str.length()) {
    return "";
  }

  size_t pos = distance;
  while (pos < str.size()) {
    str.insert(pos, "\n");
    pos += distance + 1;
  }
  return str;
}

std::string Base64EncodeImpl(unsigned char const *bytes_to_encode,
                             size_t in_len, bool url) {
  size_t len_encoded = (in_len + 2) / 3 * 4;
  unsigned char trailing_char = url ? '.' : '=';
  const char *base64_chars_ = base64_chars[url];
  std::string ret;
  ret.reserve(len_encoded);

  unsigned int pos = 0;
  while (pos < in_len) {
    ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0xfc) >> 2]);
    if (pos + 1 < in_len) {
      ret.push_back(base64_chars_[((bytes_to_encode[pos + 0] & 0x03) << 4) +
                                  ((bytes_to_encode[pos + 1] & 0xf0) >> 4)]);

      if (pos + 2 < in_len) {
        ret.push_back(base64_chars_[((bytes_to_encode[pos + 1] & 0x0f) << 2) +
                                    ((bytes_to_encode[pos + 2] & 0xc0) >> 6)]);
        ret.push_back(base64_chars_[bytes_to_encode[pos + 2] & 0x3f]);
      } else {
        ret.push_back(base64_chars_[(bytes_to_encode[pos + 1] & 0x0f) << 2]);
        ret.push_back(trailing_char);
      }
    } else {
      ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0x03) << 4]);
      ret.push_back(trailing_char);
      ret.push_back(trailing_char);
    }
    pos += 3;
  }
  return ret;
}

template <typename String>
static std::string Base64EncodeImpl(String s, bool url) {
  return Base64EncodeImpl(reinterpret_cast<const unsigned char *>(s.data()),
                          s.length(), url);
}

template <typename String, unsigned int line_length>
static std::string EncodeWithLineBreaks(String s) {
  return InsertLinebreaks(Base64EncodeImpl(s, false), line_length);
}

template <typename String>
static std::string Base64EncodePemImpl(String s) {
  return EncodeWithLineBreaks<String, 64>(s);
}

template <typename String>
static std::string Base64EncodeMimeImpl(String s) {
  return EncodeWithLineBreaks<String, 76>(s);
}

string Util::Base64Encode(std::string_view s, bool url) {
  return Base64EncodeImpl(s, url);
}

string Util::Base64EncodePem(std::string_view s) {
  return Base64EncodePemImpl(s);
}

string Util::Base64EncodeMime(std::string_view s) {
  return Base64EncodeMimeImpl(s);
}

template <typename String>
static std::string Base64DecodeImpl(String const &encoded_string,
                                    bool remove_linebreaks) {
  if (encoded_string.empty()) return std::string();
  if (remove_linebreaks) {
    std::string copy(encoded_string);
    copy.erase(std::remove(copy.begin(), copy.end(), '\n'), copy.end());
    return Base64DecodeImpl(copy, false);
  }
  size_t length_of_string = encoded_string.length();
  size_t pos = 0;

  size_t approx_length_of_decoded_string = length_of_string / 4 * 3;
  std::string ret;
  ret.reserve(approx_length_of_decoded_string);

  while (pos < length_of_string) {
    size_t CharPos_1 = CharPos(encoded_string.at(pos + 1));
    ret.push_back(static_cast<std::string::value_type>(
        ((CharPos(encoded_string.at(pos + 0))) << 2) +
        ((CharPos_1 & 0x30) >> 4)));

    if ((pos + 2 <
         length_of_string) &&  // Check for data that is not padded with equal
                               // signs (which is allowed by RFC 2045)
        encoded_string.at(pos + 2) != '=' &&
        encoded_string.at(pos + 2) !=
            '.'  // accept URL-safe base 64 strings, too, so check for '.' also.
    ) {
      unsigned int CharPos_2 = CharPos(encoded_string.at(pos + 2));
      ret.push_back(static_cast<std::string::value_type>(
          ((CharPos_1 & 0x0f) << 4) + ((CharPos_2 & 0x3c) >> 2)));

      if ((pos + 3 < length_of_string) && encoded_string.at(pos + 3) != '=' &&
          encoded_string.at(pos + 3) != '.') {
        ret.push_back(static_cast<std::string::value_type>(
            ((CharPos_2 & 0x03) << 6) + CharPos(encoded_string.at(pos + 3))));
      }
    }
    pos += 4;
  }
  return ret;
}

std::string Base64Decode(std::string const &s, bool remove_linebreaks) {
  return Base64DecodeImpl(s, remove_linebreaks);
}
std::string Util::Base64Decode(std::string_view s, bool remove_linebreaks) {
  return Base64DecodeImpl(s, remove_linebreaks);
}

string Util::Md5(const string &str, bool use_upper_case) {
  unsigned char result[MD5_DIGEST_LENGTH];
  MD5((unsigned char *)str.data(), str.size(), result);
  string hex_str;
  hex_str.reserve(512);

  string format_para = "{:02x}";
  if (use_upper_case) {
    format_para = "{:02X}";
  }

  for (auto c : result) {
    hex_str.append(fmt::format(format_para, c));
  }
  return hex_str;
}

uint64_t Util::HexStrToUInt64(const string &in) {
  try {
    return std::stoull(in, nullptr, 16);
  } catch (std::exception &e) {
    LOG(ERROR) << "Parse error: " << in;
  }
  return 0;
}

string Util::UInt64ToHexStr(const uint64_t in) {
  try {
    return fmt::format("{:016x}", in);
  } catch (std::exception &e) {
    LOG(ERROR) << "Parse error: " << in;
  }
  return "";
}

int64_t Util::Hash64(const string &str) {
  return MurmurHash64A(str.c_str(), str.length(), 42L);
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

int64_t Util::ToTimestamp(std::filesystem::file_time_type ftime) {
  auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now());
  auto duration = sctp.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
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

}  // namespace util
}  // namespace oceandoc
