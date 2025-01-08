load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@oceandoc//bazel:common.bzl", "GLOBAL_COPTS", "GLOBAL_DEFINES", "GLOBAL_LOCAL_DEFINES", "template_rule")

package(default_visibility = ["//visibility:public"])

COPTS = GLOBAL_COPTS + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [
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
    name = "libheif",
    srcs = glob(
        ["libheif/**/*.cc"],
        exclude = [
            "libheif/plugins/*.cc",
            "libheif/plugins/*.h",
            "libheif/plugins_windows.h",
            "libheif/plugins_windows.cc",
        ],
    ),
    hdrs = glob([
        "libheif/**/*.h",
        "libheif/*.h",
    ]),
    copts = ["-std=c++20"],
    includes = [
        "libheif",
        "libheif/api",
    ],
    visibility = ["//visibility:public"],
    deps = ["@openjpeg"],
)
