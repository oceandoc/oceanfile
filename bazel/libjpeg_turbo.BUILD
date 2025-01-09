load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:write_file.bzl", "write_file")
load("@oceandoc//bazel:common.bzl", "GLOBAL_COPTS", "GLOBAL_LINKOPTS", "GLOBAL_LOCAL_DEFINES", "template_rule")

package(default_visibility = ["//visibility:public"])

COPTS = GLOBAL_COPTS + select({
    "@platforms//os:windows": [],
    "//conditions:default": [],
}) + select({
    "@platforms//os:linux": [],
    "@platforms//os:osx": [],
    "@platforms//os:windows": [],
    "//conditions:default": [],
})

LOCAL_DEFINES = GLOBAL_LOCAL_DEFINES + select({
    "@platforms//os:windows": [],
    "//conditions:default": [],
}) + select({
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
    name = "libjpeg_turbo",
    srcs = [
        "src/jaricom.c",
        "src/jcapimin.c",
        "src/jcapistd.c",
        "src/jcarith.c",
        "src/jccoefct.c",
        "src/jccolor.c",
        "src/jcdctmgr.c",
        "src/jcdiffct.c",
        "src/jchuff.c",
        "src/jcicc.c",
        "src/jcinit.c",
        "src/jclhuff.c",
        "src/jclossls.c",
        "src/jcmainct.c",
        "src/jcmarker.c",
        "src/jcmaster.c",
        "src/jcomapi.c",
        "src/jcparam.c",
        "src/jcphuff.c",
        "src/jcprepct.c",
        "src/jcsample.c",
        "src/jctrans.c",
        "src/jdapimin.c",
        "src/jdapistd.c",
        "src/jdarith.c",
        "src/jdatadst.c",
        "src/jdatadst-tj.c",
        "src/jdatasrc.c",
        "src/jdatasrc-tj.c",
        "src/jdcoefct.c",
        "src/jdcolor.c",
        "src/jddctmgr.c",
        "src/jddiffct.c",
        "src/jdhuff.c",
        "src/jdicc.c",
        "src/jdinput.c",
        "src/jdlhuff.c",
        "src/jdlossls.c",
        "src/jdmainct.c",
        "src/jdmarker.c",
        "src/jdmaster.c",
        "src/jdmerge.c",
        "src/jdphuff.c",
        "src/jdpostct.c",
        "src/jdsample.c",
        "src/jdtrans.c",
        "src/jerror.c",
        "src/jfdctflt.c",
        "src/jfdctfst.c",
        "src/jfdctint.c",
        "src/jidctflt.c",
        "src/jidctfst.c",
        "src/jidctint.c",
        "src/jidctred.c",
        "src/jmemmgr.c",
        "src/jmemnobs.c",
        "src/jpeg_nbits.c",
        "src/jquant1.c",
        "src/jquant2.c",
        "src/jutils.c",
        "src/rdbmp.c",
        "src/rdppm.c",
        "src/transupp.c",
        "src/turbojpeg.c",
        "src/wrbmp.c",
        "src/wrppm.c",
        ":jconfig_h",
        ":jconfigint_h",
    ] + select({
        "@platforms//cpu:aarch64": glob([
            "simd/arm/*.c",
            "simd/arm/*.h",
            "simd/arm/aarch64/*.c",
            "simd/arm/aarch64/*.h",
            "simd/arm/aarch64/jsimd_neon.S",
        ]),
        "//conditions:default": [],
    }),
    hdrs = glob(["src/*.h"]) + [
        "src/jccolext.c",
        "src/jdcol565.c",
        "src/jdcolext.c",
        "src/jdmrg565.c",
        "src/jdmrgext.c",
        "src/jstdhuff.c",
        "src/turbojpeg-mp.c",
        ":jversion_h",
    ],
    copts = COPTS + [
        "-I$(GENDIR)/external/libjpeg_turbo",
        "-Iexternal/libjpeg_turbo/src",
    ],
    linkopts = LINKOPTS,
    local_defines = LOCAL_DEFINES,
)

write_file(
    name = "jconfig_h_in",
    out = "jconfig.h.in",
    content = [
        "#define JPEG_LIB_VERSION  62",
        "#define LIBJPEG_TURBO_VERSION  3.1.1",
        "#define LIBJPEG_TURBO_VERSION_NUMBER  3001001",
        "#define C_ARITH_CODING_SUPPORTED 1",
        "#define D_ARITH_CODING_SUPPORTED 1",
        "#define MEM_SRCDST_SUPPORTED  1",
        "#define WITH_SIMD 1",
        "",
        "#ifndef BITS_IN_JSAMPLE",
        "#define BITS_IN_JSAMPLE  8",
        "#endif",
        "",
        "#ifdef _WIN32",
        "#undef RIGHT_SHIFT_IS_UNSIGNED",
        "#ifndef __RPCNDR_H__",
        "typedef unsigned char boolean;",
        "#endif",
        "#define HAVE_BOOLEAN",
        "",
        "#if !(defined(_BASETSD_H_) || defined(_BASETSD_H))",
        "typedef short INT16;",
        "typedef signed int INT32;",
        "#endif",
        "#define XMD_H",
        "",
        "#else",
        "/* #undef RIGHT_SHIFT_IS_UNSIGNED */",
        "",
        "#endif",
    ],
)

template_rule(
    name = "jconfig_h",
    src = ":jconfig_h_in",
    out = "jconfig.h",
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
    name = "jconfigint_h_in",
    out = "jconfigint.h.in",
    content = [
        "#define BUILD  \"20250109\"",
        "#define HIDDEN  __attribute__((visibility(\"hidden\")))",
        "#undef inline",
        "#define INLINE  __inline__ __attribute__((always_inline))",
        "#define THREAD_LOCAL  __thread",
        "#define PACKAGE_NAME  \"libjpeg-turbo\"",
        "#define VERSION  \"3.1.1\"",
        "#define SIZEOF_SIZE_T  8",
        "#define HAVE_BUILTIN_CTZL",
        "/* #undef HAVE_INTRIN_H */",
        "",
        "#if defined(_MSC_VER) && defined(HAVE_INTRIN_H)",
        "#if (SIZEOF_SIZE_T == 8)",
        "#define HAVE_BITSCANFORWARD64",
        "#elif (SIZEOF_SIZE_T == 4)",
        "#define HAVE_BITSCANFORWARD",
        "#endif",
        "#endif",
        "",
        "#if defined(__has_attribute)",
        "#if __has_attribute(fallthrough)",
        "#define FALLTHROUGH  __attribute__((fallthrough));",
        "#else",
        "#define FALLTHROUGH",
        "#endif",
        "#else",
        "#define FALLTHROUGH",
        "#endif",
        "",
        "#ifndef BITS_IN_JSAMPLE",
        "#define BITS_IN_JSAMPLE  8      /* use 8 or 12 */",
        "#endif",
        "",
        "#undef C_ARITH_CODING_SUPPORTED",
        "#undef D_ARITH_CODING_SUPPORTED",
        "#undef WITH_SIMD",
        "",
        "#if BITS_IN_JSAMPLE == 8",
        "#define C_ARITH_CODING_SUPPORTED 1",
        "#define D_ARITH_CODING_SUPPORTED 1",
        "#define WITH_SIMD 1",
        "",
        "#endif",
    ],
)

template_rule(
    name = "jconfigint_h",
    src = ":jconfigint_h_in",
    out = "jconfigint.h",
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
    name = "jversion_h_in",
    out = "jversion.h.in",
    content = [
        "#if JPEG_LIB_VERSION >= 80",
        "#define JVERSION        \"8d  15-Jan-2012\"",
        "#elif JPEG_LIB_VERSION >= 70",
        "#define JVERSION        \"7  27-Jun-2009\"",
        "#else",
        "#define JVERSION        \"6b  27-Mar-1998\"",
        "#endif",
        "#define JCOPYRIGHT1 \"\"",
        "#define JCOPYRIGHT2 \"\"",
        "#define JCOPYRIGHT_SHORT \"\"",
    ],
)

template_rule(
    name = "jversion_h",
    src = ":jversion_h_in",
    out = "jversion.h",
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
