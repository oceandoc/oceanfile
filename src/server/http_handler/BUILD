load("@oceandoc//bazel:common.bzl", "GLOBAL_COPTS", "GLOBAL_LINKOPTS", "GLOBAL_LOCAL_DEFINES", "version_info")
load("//bazel:build.bzl", "cc_test")
load("//bazel:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cpplint()

COPTS = GLOBAL_COPTS + select({
    "@platforms//os:windows": [
        "/Iexternal/double-conversion",
        "/Iexternal/folly",
        "/I$(GENDIR)/external/folly",
        "/Iexternal/proxygen",
        "/I$(GENDIR)/external/proxygen",
        "/Iexternal/wangle",
    ],
    "//conditions:default": [
        "-isystem external/double-conversion",
        "-isystem external/folly",
        "-isystem $(GENDIR)/external/folly",
        "-isystem external/proxygen",
        "-isystem $(GENDIR)/external/proxygen",
        "-isystem external/wangle",
    ],
}) + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

LOCAL_DEFINES = GLOBAL_LOCAL_DEFINES + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

LINKOPTS = GLOBAL_LINKOPTS + select({
    "@platforms//os:windows": [],
    "//conditions:default": [],
}) + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

cc_library(
    name = "http_handler",
    srcs = [],
    hdrs = [
        "file_handler.h",
        "file_json_handler.h",
        "http_handler_factory.h",
        "repo_handler.h",
        "server_handler.h",
        "user_handler.h",
        "util.h",
    ],
    copts = COPTS,
    linkopts = LINKOPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        "//src/impl:repo_manager",
        "//src/server/handler_proxy",
        "//src/util",
        "//src/util:config_manager",
        "//src/util:timer",
        "@multipart-parser",
        "@proxygen",
    ],
)

cc_test(
    name = "util_test",
    srcs = ["util_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    local_defines = LOCAL_DEFINES,
    deps = [":http_handler"],
)
