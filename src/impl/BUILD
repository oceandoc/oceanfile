load("@oceandoc//bazel:common.bzl", "GLOBAL_COPTS", "GLOBAL_LOCAL_DEFINES")
load("//bazel:build.bzl", "cc_test")
load("//bazel:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cpplint()

COPTS = GLOBAL_COPTS + select({
    "@platforms//os:windows": [
        "/Iexternal/double-conversion",
        "/Iexternal/folly",
        "/Iexternal/libsodium/src/libsodium/include",
        "/I$(GENDIR)/external/folly",
    ],
    "//conditions:default": [
        "-isystem external/double-conversion",
        "-isystem external/folly",
        "-isystem external/libsodium/src/libsodium/include",
        "-isystem $(GENDIR)/external/folly",
    ],
}) + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

LOCAL_DEFINES = GLOBAL_LOCAL_DEFINES

cc_library(
    name = "repo_manager",
    srcs = ["repo_manager.cc"],
    hdrs = ["repo_manager.h"],
    copts = COPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        ":scan_manager",
        "//src/proto:cc_config",
        "//src/proto:cc_data",
        "//src/util",
        "@folly",
    ],
)

cc_test(
    name = "repo_manager_test",
    srcs = ["repo_manager_test.cc"],
    copts = COPTS,
    data = ["//conf:client_base_config.json"],
    local_defines = LOCAL_DEFINES,
    deps = [":repo_manager"],
)

cc_library(
    name = "scan_manager",
    srcs = ["scan_manager.cc"],
    hdrs = ["scan_manager.h"],
    copts = COPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        "//src/common:blocking_queue",
        "//src/common:defs",
        "//src/proto:cc_data",
        "//src/util",
        "//src/util:thread_pool",
        "@folly",
    ],
)

cc_library(
    name = "sync_manager",
    srcs = ["sync_manager.cc"],
    hdrs = ["sync_manager.h"],
    copts = COPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        ":scan_manager",
        "//src/client/grpc_client:grpc_clients",
        "//src/common:defs",
        "//src/proto:cc_data",
        "//src/proto:cc_grpc_service",
        "//src/util",
        "//src/util:thread_pool",
        "@com_github_grpc_grpc//:grpc++",
        "@folly",
    ],
)

cc_library(
    name = "receive_queue_manager",
    srcs = ["receive_queue_manager.cc"],
    hdrs = ["receive_queue_manager.h"],
    copts = COPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        ":repo_manager",
        "//src/client/grpc_client:grpc_clients",
        "//src/common:defs",
        "//src/proto:cc_data",
        "//src/proto:cc_grpc_service",
        "//src/util",
        "//src/util:thread_pool",
        "@com_github_grpc_grpc//:grpc++",
        "@folly",
    ],
)

cc_library(
    name = "user_manager",
    srcs = ["user_manager.cc"],
    hdrs = ["user_manager.h"],
    copts = COPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        ":session_manager",
        "//src/common:defs",
        "//src/proto:cc_data",
        "//src/proto:cc_grpc_service",
        "//src/util",
        "//src/util:sqlite_manager",
        "@folly",
        "@sqlite",
    ],
)

cc_library(
    name = "session_manager",
    srcs = ["session_manager.cc"],
    hdrs = ["session_manager.h"],
    copts = COPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        "//src/common:defs",
        "//src/util",
        "@folly",
    ],
)

cc_test(
    name = "sync_manager_test",
    srcs = ["sync_manager_test.cc"],
    copts = COPTS,
    data = ["//conf:client_base_config.json"],
    local_defines = LOCAL_DEFINES,
    deps = [":sync_manager"],
)

cc_test(
    name = "scan_manager_test",
    srcs = ["scan_manager_test.cc"],
    copts = COPTS,
    data = ["//conf:client_base_config.json"],
    local_defines = LOCAL_DEFINES,
    deps = [":scan_manager"],
)

cc_test(
    name = "user_manager_test",
    srcs = ["user_manager_test.cc"],
    copts = COPTS,
    data = ["//conf:client_base_config.json"],
    local_defines = LOCAL_DEFINES,
    deps = [":user_manager"],
)
