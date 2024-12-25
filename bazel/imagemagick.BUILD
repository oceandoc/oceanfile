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
        "-Iexternal/Magick++/lib/Magick++",
        "/I$(GENDIR)/external/Magick++/lib/Magick++",
        "/I$(GENDIR)/external/MagickCore",
    ],
})

LOCAL_DEFINES = GLOBAL_LOCAL_DEFINES + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

sh_binary(
    name = "configure_sh",
    srcs = ["configure"],
)

genrule(
    name = "generate_config",
    srcs = ["@imagemagick//:MagickCore"],
    outs = [
        "MagickCore/magick-baseconfig.h",
    ],
    cmd = "echo 'Running configure...'; " +
          "cp $(location @imagemagick//:MagickCore)/MagickCore.h . && " +
          "$(locations :configure_sh) && " +
          "echo 'Configure executed successfully.'",
    tools = [":configure_sh"],
)

#genrule(
#    name = "generate_config",
#    srcs = [],
#    outs = ["magick-baseconfig.h"],
#    cmd = "$(location :configure)",
#    message = "Generating magick-baseconfig.h",
#    tools = [":configure"],
#)

cc_library(
    name = "imagemagick",
    srcs = glob(
        [
            "Magick++/lib/**/*.cpp",
            "MagickCore/**/*.c",
            "MagickWand/**/*.c",
        ],
        exclude = ["MagickWand/tests/**/*.c"],
    ) + select({
        "@oceandoc//bazel:linux_x86_64": [
        ],
        "@oceandoc//bazel:osx_x86_64": [
        ],
        "@oceandoc//bazel:windows_x86_64": [
        ],
        "@platforms//cpu:aarch64": [],
        "//conditions:default": [],
    }) + [
    ],
    hdrs = [
        "Magick++/lib/Magick++/Include.h",
        "MagickCore/magick-baseconfig.h",
        "MagickCore/magick-config.h",
    ],
    copts = COPTS,
    includes = ["include"],
    local_defines = LOCAL_DEFINES,
    visibility = ["//visibility:public"],
    deps = [":generate_config"],
)

write_file(
    name = "imagemagick_config_h_in",
    out = "imagemagick_config.h.in",
    content = [],
)

template_rule(
    name = "imagemagick_config_h",
    src = ":imagemagick_config_h_in",
    out = "imagemagick_config.h",
    substitutions = select({
        "@platforms//os:linux": {
        },
        "@platforms//os:osx": {
        },
        "@platforms//os:windows": {
        },
        "//conditions:default": {},
    }) | select({
        "@oceandoc//bazel:redhat": {
        },
        "//conditions:default": {},
    }),
)
