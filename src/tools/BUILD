load("//bazel:build.bzl", "cc_test")
load("//bazel:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cpplint()

cc_binary(
    name = "scan_status_prettier",
    srcs = ["scan_status_prettier.cc"],
    deps = [
        "//src/util",
        "//src/util:scan_manager",
    ],
)

cc_binary(
    name = "scan_status_validator",
    srcs = ["scan_status_validator.cc"],
    deps = [
        "//src/util",
        "//src/util:scan_manager",
    ],
)

cc_binary(
    name = "scan",
    srcs = ["scan.cc"],
    deps = [
        "//src/util",
        "//src/util:scan_manager",
    ],
)
