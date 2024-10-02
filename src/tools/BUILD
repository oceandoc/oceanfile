load("//bazel:build.bzl", "cc_test")
load("//bazel:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cpplint()

cc_binary(
    name = "validate_file",
    srcs = ["validate_file.cc"],
    deps = ["//src/util"],
)
