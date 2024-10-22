load("@oceandoc//bazel:common.bzl", "GLOBAL_COPTS", "GLOBAL_LINKOPTS", "GLOBAL_LOCAL_DEFINES", "version_info")
load("//bazel:build.bzl", "cc_test")
load("//bazel:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cpplint()

COPTS = GLOBAL_COPTS + select({
    "@oceandoc//bazel:not_cross_compiling_on_windows": [
        "/Iexternal/double-conversion",
        "/Iexternal/folly",
        "/Iexternal/double-conversion",
        "/Iexternal/proxygen",
        "/I$(GENDIR)/external/proxygen",
        "/I$(GENDIR)/external/fizz",
        "/Iexternal/libsodium/src/libsodium/include",
        "/Iexternal/fizz",
        "/Iexternal/wangle",
        "/I$(GENDIR)/external/folly",
    ],
    "//conditions:default": [
        "-isystem external/double-conversion",
        "-isystem external/folly",
        "-isystem external/double-conversion",
        "-isystem external/proxygen",
        "-isystem $(GENDIR)/external/proxygen",
        "-isystem $(GENDIR)/external/fizz",
        "-isystem external/libsodium/src/libsodium/include",
        "-isystem external/fizz",
        "-isystem external/wangle",
        "-isystem $(GENDIR)/external/folly",
    ],
}) + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

LOCAL_DEFINES = GLOBAL_LOCAL_DEFINES + select({
    "@oceandoc//bazel:not_cross_compiling_on_windows": [
        "GLOG_NO_ABBREVIATED_SEVERITIES",
    ],
    "//conditions:default": ["BUILD_TRACING=0"],
}) + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

LINKOPTS = GLOBAL_LINKOPTS + select({
    "@oceandoc//bazel:not_cross_compiling_on_windows": [],
    "//conditions:default": [],
}) + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

cc_library(
    name = "grpc_clients",
    hdrs = [
        "grpc_file_client.h",
        "grpc_repo_client.h",
        #"grpc_server_client.h",
        "grpc_user_client.h",
    ],
    copts = COPTS,
    linkopts = LINKOPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        "//src/async_grpc",
        "//src/common:blocking_queue",
        "//src/common:defs",
        "//src/proto:cc_grpc_service",
        "//src/util",
        "//src/util:thread_pool",
        "@openssl//:ssl",
    ],
)

cc_test(
    name = "grpc_file_client_test",
    srcs = ["grpc_file_client_test.cc"],
    copts = COPTS,
    data = ["//conf:client_base_config.json"],
    local_defines = LOCAL_DEFINES,
    deps = [":grpc_clients"],
)

cc_test(
    name = "grpc_repo_client_test",
    srcs = ["grpc_repo_client_test.cc"],
    copts = COPTS,
    data = ["//conf:client_base_config.json"],
    local_defines = LOCAL_DEFINES,
    deps = [":grpc_clients"],
)

#cc_test(
#name = "grpc_server_client_test",
#srcs = ["grpc_server_client_test.cc"],
#copts = COPTS,
#data = ["//conf:base_config.json"],
#local_defines = LOCAL_DEFINES,
#deps = [":grpc_clients"],
#)

cc_test(
    name = "grpc_user_client_test",
    srcs = ["grpc_user_client_test.cc"],
    copts = COPTS,
    data = ["//conf:client_base_config.json"],
    local_defines = LOCAL_DEFINES,
    deps = [":grpc_clients"],
)
