workspace(name = "oceandoc")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:local.bzl", "local_repository", "new_local_repository")
load("//bazel:common.bzl", "gen_local_config_git")

git_repository(
    name = "bazel_skylib",
    remote = "git@github.com:bazelbuild/bazel-skylib.git",
    tag = "1.7.1",
)

git_repository(
    name = "platforms",
    remote = "git@github.com:bazelbuild/platforms.git",
    tag = "0.0.9",
)

git_repository(
    name = "bazel_gazelle",
    remote = "git@github.com:bazelbuild/bazel-gazelle.git",
    tag = "v0.37.0",
)

git_repository(
    name = "bazel_features",
    remote = "git@github.com:bazel-contrib/bazel_features.git",
    tag = "v1.12.0",
)

git_repository(
    name = "rules_cc",
    remote = "git@github.com:bazelbuild/rules_cc.git",
    tag = "0.0.9",
)

git_repository(
    name = "rules_foreign_cc",
    remote = "git@github.com:bazelbuild/rules_foreign_cc.git",
    tag = "0.11.1",
)

git_repository(
    name = "rules_perl",
    remote = "git@github.com:bazelbuild/rules_perl.git",
    tag = "0.2.3",
)

git_repository(
    name = "rules_python",
    remote = "git@github.com:bazelbuild/rules_python.git",
    tag = "0.33.0",
)

git_repository(
    name = "build_bazel_rules_swift",
    remote = "git@github.com:bazelbuild/rules_swift.git",
    tag = "1.18.0",
)

git_repository(
    name = "io_bazel_rules_go",
    remote = "git@github.com:bazelbuild/rules_go.git",
    tag = "v0.48.0",
)

git_repository(
    name = "rules_pkg",
    remote = "git@github.com:bazelbuild/rules_pkg.git",
    tag = "1.0.1",
)

git_repository(
    name = "io_bazel_rules_closure",
    remote = "git@github.com:bazelbuild/rules_closure.git",
    tag = "0.13.0",
)

git_repository(
    name = "rules_java",
    remote = "git@github.com:bazelbuild/rules_java.git",
    tag = "7.5.0",
)

git_repository(
    name = "rules_jvm_external",
    remote = "git@github.com:bazelbuild/rules_jvm_external.git",
    tag = "6.0",
)

git_repository(
    name = "contrib_rules_jvm",
    remote = "git@github.com:bazel-contrib/rules_jvm.git",
    tag = "v0.24.0",
)

git_repository(
    name = "io_bazel_rules_docker",
    remote = "git@github.com:bazelbuild/rules_docker.git",
    tag = "v0.25.0",
)

git_repository(
    name = "apple_rules_lint",
    remote = "git@github.com:apple/apple_rules_lint.git",
    tag = "0.3.2",
)

git_repository(
    name = "build_bazel_rules_apple",
    remote = "git@github.com:bazelbuild/rules_apple.git",
    tag = "3.5.1",
)

git_repository(
    name = "build_bazel_apple_support",
    remote = "git@github.com:bazelbuild/apple_support.git",
    tag = "1.15.1",
)

new_git_repository(
    name = "cpplint",
    build_file = "//bazel:cpplint.BUILD",
    commit = "7b88b68187e3516540fab3caa900988d2179ed24",
    remote = "git@github.com:cpplint/cpplint.git",
)

new_git_repository(
    name = "liburing",
    build_file = "//bazel:liburing.BUILD",
    commit = "7b3245583069bd481190c9da18f22e9fc8c3a805",
    remote = "git@github.com:axboe/liburing.git",
)

new_git_repository(
    name = "libaio",
    build_file = "//bazel:libaio.BUILD",
    commit = "b8eadc9f89e8f7ab0338eacda9f98a6caea76883",
    remote = "https://pagure.io/libaio.git",
    #remote = "git@github.com:root/libaio.git",
)

new_git_repository(
    name = "xz",
    build_file = "//bazel:xz.BUILD",
    commit = "bf901dee5d4c46609645e50311c0cb2dfdcf9738",
    remote = "git@github.com:tukaani-project/xz.git",
)

new_git_repository(
    name = "zlib",
    build_file = "//bazel:zlib.BUILD",
    remote = "git@github.com:madler/zlib.git",
    tag = "v1.3.1",
)

new_git_repository(
    name = "bzip2",
    build_file = "//bazel:bzip2.BUILD",
    commit = "66c46b8c9436613fd81bc5d03f63a61933a4dcc3",
    remote = "https://gitlab.com/bzip2/bzip2.git",
    #remote = "git@github.com:bzip2/bzip2.git",
)

new_git_repository(
    name = "lz4",
    build_file = "//bazel:lz4.BUILD",
    commit = "5b0ccb8b62eba9f0ed4b46ff3680c442c3e58188",
    remote = "git@github.com:lz4/lz4.git",
)

new_git_repository(
    name = "zstd",
    build_file = "//bazel:zstd.BUILD",
    remote = "git@github.com:facebook/zstd.git",
    tag = "v1.5.6",
)

git_repository(
    name = "brotli",
    build_file = "//bazel:brotli.BUILD",
    remote = "git@github.com:google/brotli.git",
    tag = "v1.1.0",
)

new_git_repository(
    name = "libsodium",
    build_file = "//bazel:libsodium.BUILD",
    remote = "git@github.com:jedisct1/libsodium.git",
    tag = "1.0.20-RELEASE",
)

http_archive(
    name = "nasm",
    build_file = "//bazel:nasm.BUILD",
    sha256 = "f5c93c146f52b4f1664fa3ce6579f961a910e869ab0dae431bd871bdd2584ef2",
    strip_prefix = "nasm-2.15.05",
    urls = [
        "https://mirror.bazel.build/www.nasm.us/pub/nasm/releasebuilds/2.15.05/win64/nasm-2.15.05-win64.zip",
        "https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/win64/nasm-2.15.05-win64.zip",
    ],
)

http_archive(
    name = "perl",
    build_file = "//bazel:perl.BUILD",
    sha256 = "aeb973da474f14210d3e1a1f942dcf779e2ae7e71e4c535e6c53ebabe632cc98",
    urls = [
        "https://mirror.bazel.build/strawberryperl.com/download/5.32.1.1/strawberry-perl-5.32.1.1-64bit.zip",
        "https://strawberryperl.com/download/5.32.1.1/strawberry-perl-5.32.1.1-64bit.zip",
    ],
)

http_archive(
    name = "openssl",
    build_file = "//bazel:openssl.make.BUILD",
    patch_args = ["-p1"],
    patches = ["//bazel:openssl.patch"],
    sha256 = "777cd596284c883375a2a7a11bf5d2786fc5413255efab20c50d6ffe6d020b7e",
    strip_prefix = "openssl-3.3.1",
    urls = ["https://github.com/openssl/openssl/releases/download/openssl-3.3.1/openssl-3.3.1.tar.gz"],
)

new_git_repository(
    name = "c-ares",
    build_file = "//bazel:c-ares.BUILD",
    commit = "5e1c3a7575e458ae51863da9b8d3d5d3ec6ffab8",
    remote = "git@github.com:c-ares/c-ares.git",
)

new_git_repository(
    name = "curl",
    build_file = "//bazel:curl.BUILD",
    commit = "2d5aea9c93bae110ffe5107ba2c118b8442b495d",
    remote = "git@github.com:curl/curl.git",
)

git_repository(
    name = "com_github_google_benchmark",
    remote = "git@github.com:google/benchmark.git",
    tag = "v1.9.0",
)

git_repository(
    name = "com_google_absl",
    remote = "git@github.com:abseil/abseil-cpp.git",
    tag = "20240116.2",
)

new_git_repository(
    name = "com_github_gflags_gflags",
    remote = "git@github.com:gflags/gflags.git",
    tag = "v2.2.2",
)

new_git_repository(
    name = "com_github_glog_glog",
    remote = "git@github.com:google/glog.git",
    repo_mapping = {
        "@gflags": "@com_github_gflags_gflags",
    },
    tag = "v0.5.0",
)

git_repository(
    name = "com_google_googletest",
    remote = "git@github.com:google/googletest.git",
    repo_mapping = {
        "@abseil-cpp": "@com_google_absl",
    },
    tag = "v1.15.2",
)

git_repository(
    name = "com_github_google_snappy",
    remote = "git@github.com:google/snappy.git",
    tag = "1.2.1",
)

git_repository(
    name = "com_googlesource_code_re2",
    remote = "git@github.com:google/re2.git",
    repo_mapping = {
        "@abseil-cpp": "@com_google_absl",
    },
    tag = "2024-07-02",
)

git_repository(
    name = "double-conversion",
    remote = "git@github.com:google/double-conversion.git",
    tag = "v3.3.0",
)

git_repository(
    name = "com_google_protobuf",
    remote = "git@github.com:protocolbuffers/protobuf.git",
    repo_mapping = {
        "@com_github_google_glog": "@com_github_glog_glog",
        "@com_github_curl_curl": "@curl",
    },
    tag = "v27.1",
)

git_repository(
    name = "rules_proto",
    remote = "git@github.com:bazelbuild/rules_proto.git",
    repo_mapping = {
        "@abseil-cpp": "@com_google_absl",
        "@protobuf": "@com_google_protobuf",
    },
    tag = "6.0.2",
)

git_repository(
    name = "com_github_nelhage_rules_boost",
    commit = "45ed6bf51f659c7db830fd15ddd4495dadc5afd1",
    remote = "git@github.com:nelhage/rules_boost.git",
    repo_mapping = {
        "@boringssl": "@openssl",
        "@org_lzma_lzma": "@xz",
        "@org_bzip_bzip2": "@bzip2",
    },
)

http_archive(
    name = "boost",
    #build_file = "@com_github_nelhage_rules_boost//:boost.BUILD",
    build_file = "//bazel:boost.BUILD",
    patch_cmds = ["rm -f doc/pdf/BUILD"],
    patch_cmds_win = ["Remove-Item -Force doc/pdf/BUILD"],
    repo_mapping = {
        "@boringssl": "@openssl",
        "@org_lzma_lzma": "@xz",
        "@org_bzip_bzip2": "@bzip2",
        "@com_github_facebook_zstd": "@zstd",
    },
    sha256 = "0c6049764e80aa32754acd7d4f179fd5551d8172a83b71532ae093e7384e98da",
    strip_prefix = "boost-1.83.0",
    url = "https://github.com/boostorg/boost/releases/download/boost-1.83.0/boost-1.83.0.tar.gz",
)

new_git_repository(
    name = "fmt",
    build_file = "//bazel:fmt.BUILD",
    remote = "git@github.com:fmtlib/fmt.git",
    tag = "9.1.0",
)

new_git_repository(
    name = "libdwarf",
    build_file = "//bazel:libdwarf.BUILD",
    patch_args = ["-p1"],
    patches = ["//bazel:libdwarf.patch"],
    remote = "git@github.com:davea42/libdwarf-code.git",
    tag = "v0.10.1",
)

new_git_repository(
    name = "mbedtls",
    build_file = "//bazel:mbedtls.BUILD",
    remote = "git@github.com:Mbed-TLS/mbedtls.git",
    tag = "v3.6.0",
)

new_git_repository(
    name = "libevent",
    build_file = "//bazel:libevent.BUILD",
    commit = "90b9520f3ca04dd1278c831e61a82859e3be090e",
    remote = "git@github.com:libevent/libevent.git",
)

http_archive(
    name = "libev",
    build_file = "//bazel:libev.BUILD",
    sha256 = "507eb7b8d1015fbec5b935f34ebed15bf346bed04a11ab82b8eee848c4205aea",
    strip_prefix = "libev-4.33",
    url = "https://dist.schmorp.de/libev/libev-4.33.tar.gz",
)

new_git_repository(
    name = "libuv",
    build_file = "//bazel:libuv.BUILD",
    remote = "git@github.com:libuv/libuv.git",
    tag = "v1.48.0",
)

http_archive(
    name = "libiberty",
    build_file = "//bazel:libiberty.BUILD",
    sha256 = "f6e4d41fd5fc778b06b7891457b3620da5ecea1006c6a4a41ae998109f85a800",
    strip_prefix = "binutils-2.42",
    url = "https://ftp.gnu.org/gnu/binutils/binutils-2.42.tar.xz",
)

new_git_repository(
    name = "libunwind",
    build_file = "//bazel:libunwind.BUILD",
    commit = "3c47821d681777e3cff33edb25c804d93102e1c6",
    remote = "git@github.com:libunwind/libunwind.git",
)

new_git_repository(
    name = "jemalloc",
    build_file = "//bazel:jemalloc.BUILD",
    commit = "8dc97b11089be6d58a52009ea3da610bf90331d3",
    remote = "git@github.com:jemalloc/jemalloc.git",
)

new_git_repository(
    name = "tcmalloc",
    commit = "bd13fb84b359f6cdc7e0d393b91226dbb904bf75",
    remote = "git@github.com:google/tcmalloc.git",
)

new_git_repository(
    name = "gperftools",
    build_file = "//bazel:gperftools.BUILD",
    commit = "285908e8c7cfa98659127a23532c060f8dcbd148",
    remote = "git@github.com:gperftools/gperftools.git",
)

new_git_repository(
    name = "folly",
    build_file = "//bazel:folly.BUILD",
    patch_args = ["-p1"],
    patches = ["//bazel:folly.patch"],
    remote = "git@github.com:facebook/folly.git",
    tag = "v2024.07.08.00",
)

new_git_repository(
    name = "fizz",
    build_file = "//bazel:fizz.BUILD",
    remote = "git@github.com:facebookincubator/fizz.git",
    tag = "v2024.07.08.00",
)

new_git_repository(
    name = "mvfst",
    build_file = "//bazel:mvfst.BUILD",
    remote = "git@github.com:facebook/mvfst.git",
    tag = "v2024.07.08.00",
)

new_git_repository(
    name = "wangle",
    build_file = "//bazel:wangle.BUILD",
    remote = "git@github.com:facebook/wangle.git",
    tag = "v2024.07.08.00",
)

new_git_repository(
    name = "fatal",
    build_file = "//bazel:fatal.BUILD",
    remote = "git@github.com:facebook/fatal.git",
    tag = "v2024.07.08.00",
)

new_git_repository(
    name = "xxhash",
    build_file = "//bazel:xxhash.BUILD",
    commit = "d5fe4f54c47bc8b8e76c6da9146c32d5c720cd79",
    remote = "git@github.com:Cyan4973/xxHash.git",
)

git_repository(
    name = "yaml-cpp",
    commit = "1d8ca1f35eb3a9c9142462b28282a848e5d29a91",
    remote = "git@github.com:jbeder/yaml-cpp.git",
    repo_mapping = {
        "@abseil-cpp": "@com_google_absl",
    },
)

new_git_repository(
    name = "fbthrift",
    build_file = "//bazel:fbthrift.BUILD",
    patch_args = ["-p1"],
    patches = ["//bazel:fbthrift.patch"],
    remote = "git@github.com:facebook/fbthrift.git",
    tag = "v2024.07.08.00",
)

new_git_repository(
    name = "fb303",
    build_file = "//bazel:fb303.BUILD",
    remote = "git@github.com:facebook/fb303.git",
    tag = "v2024.07.08.00",
)

new_git_repository(
    name = "proxygen",
    build_file = "//bazel:proxygen.BUILD",
    patch_args = ["-p1"],
    patches = ["//bazel:proxygen.patch"],
    remote = "git@github.com:facebook/proxygen.git",
    tag = "v2024.07.08.00",
)

new_git_repository(
    name = "smhasher",
    build_file = "//bazel:smhasher.BUILD",
    commit = "61a0530f28277f2e850bfc39600ce61d02b518de",
    remote = "git@github.com:aappleby/smhasher.git",
)

git_repository(
    name = "com_google_googleapis",
    commit = "ba245fa19c1e6f1f2a13055a437f0c815c061867",
    remote = "git@github.com:googleapis/googleapis.git",
)

http_archive(
    name = "opencensus_proto",
    sha256 = "e3d89f7f9ed84c9b6eee818c2e9306950519402bf803698b15c310b77ca2f0f3",
    strip_prefix = "opencensus-proto-0.4.1/src",
    urls = ["https://github.com/census-instrumentation/opencensus-proto/archive/v0.4.1.tar.gz"],
)

git_repository(
    name = "envoy_api",
    commit = "4118c17a2905aaf20554d0154bb8d0cd424163c4",
    remote = "git@github.com:envoyproxy/data-plane-api.git",
)

git_repository(
    name = "com_envoyproxy_protoc_gen_validate",
    remote = "git@github.com:envoyproxy/protoc-gen-validate.git",
    tag = "v1.0.4",
)

git_repository(
    name = "cel-spec",
    remote = "git@github.com:google/cel-spec.git",
    tag = "v0.15.0",
)

git_repository(
    name = "com_github_cncf_xds",
    commit = "024c85f92f20cab567a83acc50934c7f9711d124",
    remote = "git@github.com:cncf/xds.git",
    repo_mapping = {
        "@dev_cel": "@cel-spec",
    },
)

load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
    grpc = True,
)

git_repository(
    name = "com_github_grpc_grpc",
    patch_args = ["-p1"],
    patches = ["//bazel:grpc.patch"],
    remote = "git@github.com:grpc/grpc.git",
    repo_mapping = {
        "@com_github_cares_cares": "@c-ares",
        "@boringssl": "@openssl",
    },
    tag = "v1.66.1",
)

new_git_repository(
    name = "swig",
    build_file = "//bazel:swig.BUILD",
    patch_args = ["-p1"],
    patches = ["//bazel:swig.patch"],
    remote = "git@github.com:swig/swig.git",
    tag = "v4.2.0",
)

new_git_repository(
    name = "pcre2",
    build_file = "//bazel:pcre2.BUILD",
    remote = "git@github.com:PCRE2Project/pcre2.git",
    tag = "pcre2-10.42",
)

new_git_repository(
    name = "crc32c",
    build_file = "//bazel:crc32c.BUILD",
    commit = "1c51f87c9ad8157b4461e2216b9272f13fd0be3b",
    remote = "git@github.com:google/crc32c.git",
)

new_git_repository(
    name = "rapidjson",
    build_file = "//bazel:rapidjson.BUILD",
    commit = "815e6e7e7e14be44a6c15d9aefed232ff064cad0",
    remote = "git@github.com:Tencent/rapidjson.git",
)

git_repository(
    name = "hedron_compile_commands",
    commit = "e43e8eaeed3e252ac7c02983f4b1792bdff2e2f0",
    remote = "git@github.com:xiedeacc/bazel-compile-commands-extractor.git",
)

#new_git_repository(
#name = "libudev",
#build_file = "//bazel:libudev.BUILD",
#remote = "git@github.com:systemd/systemd.git",
#tag = "v256.6",
#)

new_local_repository(
    name = "libudev",
    build_file = "//bazel:libudev.BUILD",
    path = "../systemd",
)

new_git_repository(
    name = "multipart-parser",
    build_file = "//bazel:multipart-parser.BUILD",
    commit = "61e234f2100f39d405b6e9c2689e2482b31f3976",
    remote = "git@github.com:FooBarWidget/multipart-parser.git",
)

gen_local_config_git(name = "local_config_git")

#################### java ####################
load("@rules_java//java:repositories.bzl", "rules_java_dependencies", "rules_java_toolchains")

rules_java_dependencies()

rules_java_toolchains()

load("@rules_jvm_external//:repositories.bzl", "rules_jvm_external_deps")

rules_jvm_external_deps()

load("@rules_jvm_external//:setup.bzl", "rules_jvm_external_setup")

rules_jvm_external_setup()

JUNIT_PLATFORM_VERSION = "1.9.2"

JUNIT_JUPITER_VERSION = "5.9.2"

load("@rules_jvm_external//:defs.bzl", "maven_install")

maven_install(
    artifacts = [
        "net.java.dev.jna:jna:5.14.0",
        "com.google.truth:truth:0.32",
        "org.junit.platform:junit-platform-launcher:%s" % JUNIT_PLATFORM_VERSION,
        "org.junit.platform:junit-platform-reporting:%s" % JUNIT_PLATFORM_VERSION,
        "org.junit.jupiter:junit-jupiter-api:%s" % JUNIT_JUPITER_VERSION,
        "org.junit.jupiter:junit-jupiter-params:%s" % JUNIT_JUPITER_VERSION,
        "org.junit.jupiter:junit-jupiter-engine:%s" % JUNIT_JUPITER_VERSION,
    ],
    repositories = [
        "https://repo1.maven.org/maven2",
        #"https://maven.aliyun.com/repository/central",
    ],
)

load("@contrib_rules_jvm//:repositories.bzl", "contrib_rules_jvm_deps")

contrib_rules_jvm_deps()

load("@contrib_rules_jvm//:setup.bzl", "contrib_rules_jvm_setup")

contrib_rules_jvm_setup()

load("@bazel_skylib//lib:versions.bzl", "versions")

versions.check("7.2.0")

load("@bazel_features//:deps.bzl", "bazel_features_deps")
load("@io_bazel_rules_closure//closure:repositories.bzl", "rules_closure_dependencies", "rules_closure_toolchains")
load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")
load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")
load("@rules_perl//perl:deps.bzl", "perl_register_toolchains")
load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
load("@rules_proto//proto:toolchains.bzl", "rules_proto_toolchains")
load("@rules_python//python:repositories.bzl", "py_repositories")

bazel_features_deps()

rules_foreign_cc_dependencies()

py_repositories()

go_rules_dependencies()

rules_pkg_dependencies()

go_register_toolchains(version = "1.22.1")

perl_register_toolchains()

rules_closure_dependencies()

rules_closure_toolchains()

rules_proto_toolchains()

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

boost_deps()

#################### hedron_compile_commands ####################
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
load("@hedron_compile_commands//:workspace_setup_transitive.bzl", "hedron_compile_commands_setup_transitive")
load("@hedron_compile_commands//:workspace_setup_transitive_transitive.bzl", "hedron_compile_commands_setup_transitive_transitive")
load("@hedron_compile_commands//:workspace_setup_transitive_transitive_transitive.bzl", "hedron_compile_commands_setup_transitive_transitive_transitive")

hedron_compile_commands_setup()

hedron_compile_commands_setup_transitive()

hedron_compile_commands_setup_transitive_transitive()

hedron_compile_commands_setup_transitive_transitive_transitive()

register_toolchains(
    "@openssl//:preinstalled_make_toolchain",
    "@openssl//:preinstalled_pkgconfig_toolchain",
)

http_archive(
    name = "linux-x86_64-gnu_sysroot",
    build_file = "//bazel:toolchains.BUILD",
    sha256 = "30546f20d6a16bf5de3a15eb63418a488f5317402b4907484b1521a7a4e2bc7c",
    strip_prefix = "linux-x86_64-gnu_sysroot",
    urls = ["https://code.xiamu.com/files/linux-x86_64-gnu_sysroot.tar.gz"],
)

http_archive(
    name = "linux-aarch64-gnu_sysroot",
    build_file = "//bazel:toolchains.BUILD",
    sha256 = "16bfff2c7306118f2bebc6d8d35188768bf312e60cf3fc363e2ab8d96f53240e",
    strip_prefix = "linux-aarch64-gnu_sysroot",
    urls = ["https://code.xiamu.com/files/linux-aarch64-gnu_sysroot.tar.gz"],
)

http_archive(
    name = "linux-aarch64-musl_sysroot",
    build_file = "//bazel:toolchains.BUILD",
    sha256 = "a76ef46d1815d465cb2079825104aca507a230bc973477f4ce1e9d94a325d7e8",
    strip_prefix = "linux-aarch64-musl_sysroot",
    urls = ["https://code.xiamu.com/files/linux-aarch64-musl_sysroot.tar.gz"],
)

http_archive(
    name = "macosx14.2_sysroot",
    build_file = "//bazel:toolchains.BUILD",
    sha256 = "dfda4d0f437035242c865fb10c3e183f348fab41251847e2f9c6930cf7772768",
    strip_prefix = "macosx14.2_sysroot",
    urls = ["https://code.xiamu.com/files/macosx14.2_sysroot.tar.gz"],
)

new_git_repository(
    name = "blake3",
    build_file = "//bazel:blake3.BUILD",
    remote = "git@github.com:BLAKE3-team/BLAKE3.git",
    tag = "1.5.4",
)

http_archive(
    name = "sqlite",
    build_file = "//bazel:sqlite.BUILD",
    sha256 = "77823cb110929c2bcb0f5d48e4833b5c59a8a6e40cdea3936b99e199dbbe5784",
    strip_prefix = "sqlite-amalgamation-3460100",
    urls = ["https://sqlite.org/2024/sqlite-amalgamation-3460100.zip"],
)

#new_git_repository(
#name = "imagemagick",
#build_file = "//bazel:imagemagick.BUILD",
#remote = "git@github.com:ImageMagick/ImageMagick.git",
#tag = "7.1.1-41",
#)

new_local_repository(
    name = "imagemagick",
    build_file = "//bazel:imagemagick.BUILD",
    path = "../ImageMagick",
)

new_git_repository(
    name = "libxml2",
    build_file = "//bazel:libxml2.BUILD",
    commit = "71c37a565d3726440aa96d648db0426deb90157b",
    remote = "git@github.com:GNOME/libxml2.git",
)

new_git_repository(
    name = "libwebp",
    build_file = "//bazel:libwebp.BUILD",
    commit = "2af6c034ac871c967e04c8c9f8bf2dbc2e271b18",
    remote = "git@github.com:webmproject/libwebp.git",
)

new_git_repository(
    name = "libpng",
    build_file = "//bazel:libpng.BUILD",
    commit = "c1cc0f3f4c3d4abd11ca68c59446a29ff6f95003",
    remote = "git@github.com:pnggroup/libpng.git",
)

new_git_repository(
    name = "libyuv",
    build_file = "//bazel:libyuv.BUILD",
    commit = "b5a18f9d937492c0e2d3dc88d6a2ed39c6cfda24",
    remote = "git@github.com:lemenkov/libyuv.git",
)

new_git_repository(
    name = "freetype2",
    build_file = "//bazel:freetype2.BUILD",
    commit = "10b3b14da2a60151dd9242364ad7a375d0d7590a",
    remote = "git@github.com:freetype/freetype.git",
)

new_git_repository(
    name = "openexr",
    build_file = "//bazel:openexr.BUILD",
    commit = "8bc3faebc66b92805f7309fa7a2f46a66e5cc5c9",
    remote = "git@github.com:AcademySoftwareFoundation/openexr.git",
)

new_git_repository(
    name = "imath",
    build_file = "//bazel:imath.BUILD",
    commit = "aa28eb56e2be4547e220f2dc7bbf9961e2d8c9c4",
    remote = "git@github.com:AcademySoftwareFoundation/Imath.git",
)

new_git_repository(
    name = "libdeflate",
    build_file = "//bazel:libdeflate.BUILD",
    commit = "78051988f96dc8d8916310d8b24021f01bd9e102",
    remote = "git@github.com:ebiggers/libdeflate.git",
)

new_git_repository(
    name = "cc_toolchains",
    commit = "2a194c24161d9488bbfbb2bd9f486967edcc7170",
    remote = "git@github.com:xiedeacc/cc_toolchains.git",
)

load("//bazel:toolchains.bzl", "cc_toolchains_register")

cc_toolchains_register()
