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
    name = "openjpeg",
    srcs = glob(
        [
            "src/lib/openjp2/*.c",
            "src/lib/openjpip/*.c",
        ],
        exclude = [
            "src/lib/openjp2/test_sparse_array.c",
            "src/lib/openjp2/bench_dwt.c",
        ],
    ) + [
        ":opj_config_h",
        ":opj_config_private_h",
    ],
    hdrs = glob([
        "src/**/*.h",
        "src/lib/openjp2/*.h",
        "src/lib/openjpip/*.h",
    ]),
    copts = COPTS + [
        "-Iexternal/openjpeg/src/lib/openjp2",
        "-Iexternal/openjpeg/src/lib/openjpip",
        "-Iexternal/openjpeg/src/lib",
        "-Iexternal/openjpeg/src",
        "-Iexternal/openjpeg",
    ],
    includes = [
        "src",
        "src/lib",
        "src/lib/openjp2",
        "src/lib/openjpip",
    ],
    visibility = ["//visibility:public"],
    deps = [],
)

write_file(
    name = "opj_config_private_h_in",
    out = "opj_config_private.h.in",
    content = [
        "/* create opj_config_private.h for CMake */",
        "",
        "#define OPJ_PACKAGE_VERSION \"2.5.3\"",
        "",
        "/* Not used by openjp2*/",
        "/*#define HAVE_MEMORY_H 1*/",
        "/*#define HAVE_STDLIB_H 1*/",
        "/*#define HAVE_STRINGS_H 1*/",
        "/*#define HAVE_STRING_H 1*/",
        "/*#define HAVE_SYS_STAT_H 1*/",
        "/*#define HAVE_SYS_TYPES_H 1 */",
        "/*#define HAVE_UNISTD_H 1*/",
        "/*#define HAVE_INTTYPES_H 1 */",
        "/*#define HAVE_STDINT_H 1 */",
        "",
        "/* #undef _LARGEFILE_SOURCE */",
        "/* #undef _LARGE_FILES */",
        "/* #undef _FILE_OFFSET_BITS */",
        "#define OPJ_HAVE_FSEEKO ON",
        "",
        "/* find whether or not have <malloc.h> */",
        "#define OPJ_HAVE_MALLOC_H",
        "/* check if function `aligned_alloc` exists */",
        "/* #undef OPJ_HAVE_ALIGNED_ALLOC */",
        "/* check if function `_aligned_malloc` exists */",
        "/* #undef OPJ_HAVE__ALIGNED_MALLOC */",
        "/* check if function `memalign` exists */",
        "#define OPJ_HAVE_MEMALIGN",
        "/* check if function `posix_memalign` exists */",
        "#define OPJ_HAVE_POSIX_MEMALIGN",
        "",
        "#if !defined(_POSIX_C_SOURCE)",
        "#if defined(OPJ_HAVE_FSEEKO) || defined(OPJ_HAVE_POSIX_MEMALIGN)",
        "/* Get declarations of fseeko, ftello, posix_memalign. */",
        "#define _POSIX_C_SOURCE 200112L",
        "#endif",
        "#endif",
        "",
        "/* Byte order.  */",
        "/* All compilers that support Mac OS X define either __BIG_ENDIAN__ or",
        "__LITTLE_ENDIAN__ to match the endianness of the architecture being",
        "compiled for. This is not necessarily the same as the architecture of the",
        "machine doing the building. In order to support Universal Binaries on",
        "Mac OS X, we prefer those defines to decide the endianness.",
        "On other platforms we use the result of the TRY_RUN. */",
        "#if !defined(__APPLE__)",
        "/* #undef OPJ_BIG_ENDIAN */",
        "#elif defined(__BIG_ENDIAN__)",
        "# define OPJ_BIG_ENDIAN",
        "#endif",
    ],
)

template_rule(
    name = "opj_config_private_h",
    src = ":opj_config_private_h_in",
    out = "src/lib/openjp2/opj_config_private.h",
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

write_file(
    name = "opj_config_h_in",
    out = "opj_config.h.in",
    content = [
        "#ifndef OPJ_CONFIG_H_INCLUDED",
        "#define OPJ_CONFIG_H_INCLUDED",
        "",
        "/* create opj_config.h for CMake */",
        "",
        "/*--------------------------------------------------------------------------*/",
        "/* OpenJPEG Versioning                                                      */",
        "",
        "/* Version number. */",
        "#define OPJ_VERSION_MAJOR 2",
        "#define OPJ_VERSION_MINOR 5",
        "#define OPJ_VERSION_BUILD 3",
        "",
        "#endif",
    ],
)

template_rule(
    name = "opj_config_h",
    src = ":opj_config_h_in",
    out = "src/lib/openjp2/opj_config.h",
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
