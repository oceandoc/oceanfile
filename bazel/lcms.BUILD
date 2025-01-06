load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@oceandoc//bazel:common.bzl", "GLOBAL_COPTS", "GLOBAL_DEFINES", "GLOBAL_LOCAL_DEFINES", "template_rule")

package(default_visibility = ["//visibility:public"])

COPTS = GLOBAL_COPTS + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [
        "-std=c11",
        "-O3",
        "-fPIC",
    ],
})

LOCAL_DEFINES = GLOBAL_LOCAL_DEFINES + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

cc_library(
    name = "lcms",
    srcs = glob(
        [
            "src/**/*.c",
        ],
        exclude = [
        ],
    ) + [
    ],
    hdrs = glob([
        "src/**/*.h",
        "include/**/*.h",
    ]),
    copts = COPTS + [
        "-Iexternal/lcms",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [],
)
