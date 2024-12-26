load("@oceandoc//bazel:common.bzl", "GLOBAL_COPTS", "GLOBAL_LINKOPTS", "GLOBAL_LOCAL_DEFINES", "version_info")
load("//bazel:build.bzl", "cc_test")
load("//bazel:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cpplint()

COPTS = GLOBAL_COPTS + select({
    "@platforms//os:windows": [
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

#cc_binary(
#name = "scan_status_prettier",
#srcs = ["scan_status_prettier.cc"],
#copts = COPTS,
#linkopts = LINKOPTS,
#local_defines = LOCAL_DEFINES,
#deps = [
#"//src/impl:scan_manager",
#"//src/util",
#],
#)

#cc_binary(
#name = "scan_status_validator",
#srcs = ["scan_status_validator.cc"],
#copts = COPTS,
#linkopts = LINKOPTS,
#local_defines = LOCAL_DEFINES,
#deps = [
#"//src/impl:scan_manager",
#"//src/util",
#],
#)

#cc_binary(
#name = "scan",
#srcs = ["scan.cc"],
#copts = COPTS,
#linkopts = LINKOPTS,
#local_defines = LOCAL_DEFINES,
#deps = [
#"//src/impl:scan_manager",
#"//src/util",
#],
#)

cc_binary(
    name = "user_op",
    srcs = ["user_op.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    local_defines = LOCAL_DEFINES,
    deps = [
        "//src/impl:user_manager",
        "//src/util",
        "//src/util:config_manager",
    ],
)
