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
                        "sha256sum": "b02b436119b4e5689ad51a376e255797305b424da9638b27e592b1cfd15e7101",
                        "sysroot": "@linux-x86_64-gnu_sysroot",
                        #"url": "/zfs/www/files/gcc14.2.0-x86_64-unknown-linux-gnu",
                        #"sysroot": "/zfs/www/files/linux-x86_64-gnu_sysroot",
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
                            #because build tool will also use toolchain by url on a x86_64 linux host,
                            #so if a  generated binary that need run during in the build process,
                            # it will coredump for cannot find libc
                            "/usr/lib/x86_64-linux-gnu",
                        ],
                        "sysroot_lib_directories": [
                            #"lib",
                            #"usr/lib",
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
                        "sha256sum": "404b5723f1e8ed3a005d587fbad6218beb58e8e58142a87fb9f6a78a526c946e",
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
                            "ld": "lld",
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
                            "/usr/lib/x86_64-linux-gnu",
                        ],
                        "sysroot_lib_directories": [
                            #"lib",
                            #"usr/lib",
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
                        "sha256sum": "404b5723f1e8ed3a005d587fbad6218beb58e8e58142a87fb9f6a78a526c946e",
                        "sysroot": "@macosx14.2_sysroot",
                        #"url": "/zfs/www/files/clang18.1.8-x86_64-unknown-linux-gnu",
                        #"sysroot": "/zfs/www/files/macosx14.2_sysroot",
                        "tool_names": {
                            "ar": "x86_64-apple-darwin23.2-libtool",
                            "as": "x86_64-apple-darwin23.2-as",
                            "c++": "clang++",
                            "cpp": "clang-cpp",
                            "g++": "clang++",
                            "gcc": "clang",
                            "gcov": "llvm-cov",
                            "ld": "x86_64-apple-darwin23.2-ld",
                            "llvm-cov": "llvm-cov",
                            "nm": "x86_64-apple-darwin23.2-nm",
                            "objcopy": "x86_64-apple-darwin-objcopy",
                            "objdump": "x86_64-apple-darwin-objdump",
                            "strip": "x86_64-apple-darwin23.2-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "include/darwin/c++/v1",
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
                            "lib/x86_64-darwin",
                            "lib/clang/18/lib/x86_64-darwin",
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
                        "sha256sum": "268b88f4215ea14d4acd1878cca45bcc925ba530a6bee73710404fee40d472a0",
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
                        "sha256sum": "df50a1807433bf1f8eaeaeaecf8c53aa12e72e5285fb539c7b64532833f017a4",
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
                        "sha256sum": "404b5723f1e8ed3a005d587fbad6218beb58e8e58142a87fb9f6a78a526c946e",
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
                        "sha256sum": "404b5723f1e8ed3a005d587fbad6218beb58e8e58142a87fb9f6a78a526c946e",
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
                            "llvm-cov": "llvm-cov",
                            "nm": "x86_64-apple-darwin23.2-nm",
                            "objcopy": "x86_64-apple-darwin-objcopy",
                            "objdump": "x86_64-apple-darwin-objdump",
                            "strip": "x86_64-apple-darwin23.2-strip",
                        },
                        "cxx_builtin_include_directories": [
                            "include/darwin/c++/v1",
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
                            "lib/aarch64-darwin",
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
