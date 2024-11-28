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

LOCAL_DEFINES = GLOBAL_LOCAL_DEFINES + ["BUILD_TRACING=0"]

cc_binary(
    name = "debug",
    srcs = ["debug.cc"],
    copts = COPTS,
    linkopts = ["-static"],
    linkstatic = True,
    local_defines = LOCAL_DEFINES,
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
    ],
)

cc_library(
    name = "test_util",
    hdrs = ["test_util.h"],
    copts = COPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        "//src/util",
    ],
)

cc_library(
    name = "test_main",
    srcs = ["test_main.cc"],
    copts = COPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_googletest//:gtest",
        "@folly",
    ],
)
