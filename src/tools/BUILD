load("//bazel:build.bzl", "cc_test")
load("//bazel:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cpplint()

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
    deps = [
        "@boost//:algorithm",
        "@com_github_glog_glog//:glog",
    ],
)

cc_binary(
    name = "conf_gen",
    srcs = ["conf_gen.cc"],
    deps = ["//src/tools:common"],
)

cc_binary(
    name = "conf_diff",
    srcs = ["conf_diff.cc"],
    deps = ["//src/tools:common"],
)

cc_binary(
    name = "conf_replace",
    srcs = ["conf_replace.cc"],
    deps = ["//src/tools:common"],
)
