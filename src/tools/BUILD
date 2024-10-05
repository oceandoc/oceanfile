load("//bazel:build.bzl", "cc_test")
load("//bazel:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cpplint()

cc_binary(
    name = "pretty_scan_status",
    srcs = ["pretty_scan_status.cc"],
    deps = [
        "//src/util",
        "//src/util:scan_manager",
    ],
)
