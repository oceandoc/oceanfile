load("@cc_toolchains//toolchain:cc_toolchains_setup.bzl", "cc_toolchains_setup")

def cc_toolchains_register():
    cc_toolchains_setup(
        name = "cc_toolchains_setup",
        toolchains = {
            "x86_64": {
                "linux": [
                    {
                        "vendor": "unknown",
                        "libc": "gnu",
                        "compiler": "gcc",
                        "triple": "x86_64-unknown-linux-gnu",
                        "url": "https://code.xiamu.com/files/gcc14.2.0-x86_64-unknown-linux-gnu.tar.gz",
                        "strip_prefix": "gcc14.2.0-x86_64-unknown-linux-gnu",
                        "sha256sum": "5300d26766abf134dec08edc52950257193d1d1a20a19bfdc4345598947a1d91",
                        "sysroot": "@linux-x86_64-gnu_sysroot",
                        "tool_names": {
                            "ar": "x86_64-unknown-linux-gnu-ar",
                            "as": "x86_64-unknown-linux-gnu-as",
                            "c++": "x86_64-unknown-linux-gnu-c++",
                            "cpp": "x86_64-unknown-linux-gnu-cpp",
                            "g++": "x86_64-unknown-linux-gnu-g++",
                            "gcc": "x86_64-unknown-linux-gnu-gcc",
                            "gcov": "x86_64-unknown-linux-gnu-gcov",
                            "ld": "x86_64-unknown-linux-gnu-ld",
                            "llvm-cov": "x86_64-unknown-linux-gnu-gcov",
                            "nm": "x86_64-unknown-linux-gnu-nm",
                            "objcopy": "x86_64-unknown-linux-gnu-objcopy",
                            "objdump": "x86_64-unknown-linux-gnu-objdump",
                            "strip": "x86_64-unknown-linux-gnu-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "x86_64-unknown-linux-gnu/include/c++/14.2.0/x86_64-unknown-linux-gnu",
                            "x86_64-unknown-linux-gnu/include/c++/14.2.0",
                            "x86_64-unknown-linux-gnu/include/c++/14.2.0/backward",
                            "lib/gcc/x86_64-unknown-linux-gnu/14.2.0/include",
                            "lib/gcc/x86_64-unknown-linux-gnu/14.2.0/include-fixed",
                        ],
                        "sysroot_include_directories": [
                            "usr/include",
                        ],
                        "lib_directories": [
                            "lib",
                            "lib/gcc/x86_64-unknown-linux-gnu/14.2.0",
                        ],
                        "sysroot_lib_directories": [
                            "lib",
                            "usr/lib",
                        ],
                        "link_libs": [],
                        "supports_start_end_lib": True,
                        "debug": True,
                    },
                    {
                        "vendor": "unknown",
                        "libc": "gnu",
                        "compiler": "clang",
                        "triple": "x86_64-unknown-linux-gnu",
                        "url": "https://code.xiamu.com/files/clang18.1.8-x86_64-unknown-linux-gnu.tar.gz",
                        "strip_prefix": "clang18.1.8-x86_64-unknown-linux-gnu",
                        "sha256sum": "8682fb861e4e025477a3fc1eda203072738846ad90bc533dcbd7a2c3963f167c",
                        "sysroot": "@linux-x86_64-gnu_sysroot",
                        #"url": "/usr/local/llvm/clang18.1.8-x86_64-unknown-linux-gnu",
                        "tool_names": {
                            "ar": "llvm-ar",
                            "as": "llvm-as",
                            "c++": "clang++",
                            "cpp": "clang-cpp",
                            "g++": "clang++",
                            "gcc": "clang",
                            "gcov": "llvm-cov",
                            "ld": "ld.lld",
                            "llvm-cov": "llvm-cov",
                            "nm": "llvm-nm",
                            "objcopy": "llvm-objcopy",
                            "objdump": "llvm-objdump",
                            "strip": "llvm-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "include/x86_64-unknown-linux-gnu/c++/v1",
                            "include/c++/v1",
                            "lib/clang/18/include",
                            "lib/clang/18/share",
                            "include",
                        ],
                        "sysroot_include_directories": [
                            "usr/include",
                        ],
                        "lib_directories": [
                            "lib",
                            "lib/x86_64-unknown-linux-gnu",
                            "lib/clang/18/lib/x86_64-unknown-linux-gnu",
                        ],
                        "sysroot_lib_directories": [
                            "lib",
                            "usr/lib",
                        ],
                        "link_libs": [],
                        "supports_start_end_lib": True,
                        "debug": True,
                    },
                ],
                "osx": [
                    {
                        "vendor": "unknown",
                        "libc": "macosx",
                        "compiler": "clang",
                        "triple": "x86_64-apple-darwin",
                        "url": "https://code.xiamu.com/files/clang18.1.8-x86_64-unknown-linux-gnu.tar.gz",
                        "strip_prefix": "clang18.1.8-x86_64-unknown-linux-gnu",
                        "sha256sum": "8682fb861e4e025477a3fc1eda203072738846ad90bc533dcbd7a2c3963f167c",
                        "sysroot": "@macosx14.2_sysroot",
                        #"url": "/usr/local/llvm/clang18.1.8-x86_64-unknown-linux-gnu",
                        #"sysroot": "/zfs/www/files/macosx14.2_sysroot",
                        "tool_names": {
                            "ar": "x86_64-apple-darwin23.2-libtool",
                            "as": "x86_64-apple-darwin23.2-as",
                            "c++": "clang++",
                            "cpp": "clang-cpp",
                            "g++": "clang++",
                            "gcc": "clang",
                            "gcov": "x86_64-apple-darwin23.2-gcov",
                            "ld": "x86_64-apple-darwin23.2-ld",
                            "llvm-cov": "None",
                            "nm": "x86_64-apple-darwin23.2-nm",
                            "objcopy": "x86_64-apple-darwin-objcopy",
                            "objdump": "x86_64-apple-darwin-objdump",
                            "strip": "x86_64-apple-darwin23.2-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "include/c++/v1",
                            "lib/clang/18/include",
                            "lib/clang/18/share",
                            "include",
                        ],
                        "sysroot_include_directories": [
                            "usr/include",
                            "System/Library/Frameworks",
                        ],
                        "lib_directories": [
                            "lib/darwin",
                            "lib/clang/18/lib/darwin",
                        ],
                        "sysroot_lib_directories": [
                            "usr/lib",
                            "System/Library/Frameworks",
                        ],
                        "supports_start_end_lib": False,
                        "debug": True,
                    },
                ],
            },
            "aarch64": {
                "linux": [
                    {
                        "vendor": "unknown",
                        "libc": "gnu",
                        "compiler": "gcc",
                        "triple": "aarch64-unknown-linux-gnu",
                        "url": "https://code.xiamu.com/files/gcc14.2.0-aarch64-unknown-linux-gnu.tar.gz",
                        "strip_prefix": "gcc14.2.0-aarch64-unknown-linux-gnu",
                        "sha256sum": "9dc69b3a46e5e9a9654abcf44391415e164097ecb1191eafd30689c834181f00",
                        "sysroot": "@linux-aarch64-gnu_sysroot",
                        "tool_names": {
                            "ar": "aarch64-unknown-linux-gnu-ar",
                            "as": "aarch64-unknown-linux-gnu-as",
                            "c++": "aarch64-unknown-linux-gnu-c++",
                            "cpp": "aarch64-unknown-linux-gnu-cpp",
                            "g++": "aarch64-unknown-linux-gnu-g++",
                            "gcc": "aarch64-unknown-linux-gnu-gcc",
                            "gcov": "aarch64-unknown-linux-gnu-gcov",
                            "ld": "aarch64-unknown-linux-gnu-ld",
                            "llvm-cov": "aarch64-unknown-linux-gnu-gcov",
                            "nm": "aarch64-unknown-linux-gnu-nm",
                            "objcopy": "aarch64-unknown-linux-gnu-objcopy",
                            "objdump": "aarch64-unknown-linux-gnu-objdump",
                            "strip": "aarch64-unknown-linux-gnu-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "aarch64-unknown-linux-gnu/include/c++/14.2.0/aarch64-unknown-linux-gnu",
                            "aarch64-unknown-linux-gnu/include/c++/14.2.0",
                            "aarch64-unknown-linux-gnu/include/c++/14.2.0/backward",
                            "lib/gcc/aarch64-unknown-linux-gnu/14.2.0/include",
                            "lib/gcc/aarch64-unknown-linux-gnu/14.2.0/include-fixed",
                        ],
                        "sysroot_include_directories": [
                            "usr/include",
                        ],
                        "lib_directories": [
                            "lib",
                            "lib/gcc/aarch64-unknown-linux-gnu/14.2.0",
                        ],
                        "sysroot_lib_directories": [
                            "lib",
                            "usr/lib",
                        ],
                        "link_libs": [],
                        "supports_start_end_lib": True,
                        "debug": True,
                    },
                    {
                        "vendor": "unknown",
                        "libc": "musl",
                        "compiler": "gcc",
                        "triple": "aarch64-unknown-linux-musl",
                        "url": "https://code.xiamu.com/files/gcc14.2.0-aarch64-unknown-linux-musl.tar.gz",
                        "strip_prefix": "gcc14.2.0-aarch64-unknown-linux-musl",
                        "sha256sum": "2dd12401fb08fa41bceac94e6f343071a7e91630b2b8290d21d1ffc4c02f7519",
                        "sysroot": "@linux-aarch64-musl_sysroot",
                        "tool_names": {
                            "ar": "aarch64-unknown-linux-musl-ar",
                            "as": "aarch64-unknown-linux-musl-as",
                            "c++": "aarch64-unknown-linux-musl-c++",
                            "cpp": "aarch64-unknown-linux-musl-cpp",
                            "g++": "aarch64-unknown-linux-musl-g++",
                            "gcc": "aarch64-unknown-linux-musl-gcc",
                            "gcov": "aarch64-unknown-linux-musl-gcov",
                            "ld": "aarch64-unknown-linux-musl-ld",
                            "llvm-cov": "aarch64-unknown-linux-musl-gcov",
                            "nm": "aarch64-unknown-linux-musl-nm",
                            "objcopy": "aarch64-unknown-linux-musl-objcopy",
                            "objdump": "aarch64-unknown-linux-musl-objdump",
                            "strip": "aarch64-unknown-linux-musl-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "aarch64-unknown-linux-musl/include/c++/14.2.0/aarch64-unknown-linux-musl",
                            "aarch64-unknown-linux-musl/include/c++/14.2.0",
                            "aarch64-unknown-linux-musl/include/c++/14.2.0/backward",
                            "lib/gcc/aarch64-unknown-linux-musl/14.2.0/include",
                            "lib/gcc/aarch64-unknown-linux-musl/14.2.0/include-fixed",
                        ],
                        "sysroot_include_directories": [
                            "usr/include",
                        ],
                        "lib_directories": [
                            "lib",
                            "lib/gcc/x86_64-unknown-linux-gnu/14.2.0",
                        ],
                        "sysroot_lib_directories": [
                            "lib",
                            "usr/lib",
                        ],
                        "link_libs": [],
                        "supports_start_end_lib": True,
                        "debug": True,
                    },
                    {
                        "vendor": "unknown",
                        "libc": "gnu",
                        "compiler": "clang",
                        "triple": "aarch64-unknown-linux-gnu",
                        "url": "https://code.xiamu.com/files/clang18.1.8-x86_64-unknown-linux-gnu.tar.gz",
                        "strip_prefix": "clang18.1.8-x86_64-unknown-linux-gnu",
                        "sha256sum": "8682fb861e4e025477a3fc1eda203072738846ad90bc533dcbd7a2c3963f167c",
                        "sysroot": "@linux-aarch64-gnu_sysroot",
                        #"url": "/usr/local/llvm/clang18.1.8-x86_64-unknown-linux-gnu",
                        "tool_names": {
                            "ar": "llvm-ar",
                            "as": "llvm-as",
                            "c++": "clang++",
                            "cpp": "clang-cpp",
                            "g++": "clang++",
                            "gcc": "clang",
                            "gcov": "llvm-cov",
                            "ld": "ld.lld",
                            "llvm-cov": "llvm-cov",
                            "nm": "llvm-nm",
                            "objcopy": "llvm-objcopy",
                            "objdump": "llvm-objdump",
                            "strip": "llvm-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "include/aarch64-unknown-linux-gnu/c++/v1",
                            "include/c++/v1",
                            "lib/clang/18/include",
                            "lib/clang/18/share",
                            "include",
                        ],
                        "sysroot_include_directories": [
                            "usr/include",
                        ],
                        "lib_directories": [
                            "lib",
                            "lib/x86_64-unknown-linux-gnu",
                            "lib/clang/18/lib/x86_64-unknown-linux-gnu",
                        ],
                        "sysroot_lib_directories": [
                            "lib",
                            "usr/lib",
                        ],
                        "link_libs": [],
                        "supports_start_end_lib": True,
                        "debug": True,
                    },
                ],
                "osx": [
                    {
                        "vendor": "unknown",
                        "libc": "macosx",
                        "compiler": "clang",
                        "triple": "aarch64-apple-darwin",
                        "url": "https://code.xiamu.com/files/clang18.1.8-x86_64-unknown-linux-gnu.tar.gz",
                        "strip_prefix": "clang18.1.8-x86_64-unknown-linux-gnu",
                        "sha256sum": "8682fb861e4e025477a3fc1eda203072738846ad90bc533dcbd7a2c3963f167c",
                        "sysroot": "@macosx14.2_sysroot",
                        #"url": "/usr/local/llvm/clang18.1.8-x86_64-unknown-linux-gnu",
                        #"sysroot": "/zfs/www/files/macosx14.2_sysroot",
                        "tool_names": {
                            "ar": "x86_64-apple-darwin23.2-libtool",
                            "as": "x86_64-apple-darwin23.2-as",
                            "c++": "clang++",
                            "cpp": "clang-cpp",
                            "g++": "clang++",
                            "gcc": "clang",
                            "gcov": "x86_64-apple-darwin23.2-gcov",
                            "ld": "x86_64-apple-darwin23.2-ld",
                            "llvm-cov": "None",
                            "nm": "x86_64-apple-darwin23.2-nm",
                            "objcopy": "x86_64-apple-darwin-objcopy",
                            "objdump": "x86_64-apple-darwin-objdump",
                            "strip": "x86_64-apple-darwin23.2-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "include/c++/v1",
                            "lib/clang/18/include",
                            "lib/clang/18/share",
                            "include",
                        ],
                        "sysroot_include_directories": [
                            "usr/include",
                            "System/Library/Frameworks",
                        ],
                        "lib_directories": [
                            "lib/darwin",
                            "lib/clang/18/lib/darwin",
                        ],
                        "sysroot_lib_directories": [
                            "usr/lib",
                            "System/Library/Frameworks",
                        ],
                        "supports_start_end_lib": False,
                        "debug": True,
                    },
                ],
            },
        },
    )
