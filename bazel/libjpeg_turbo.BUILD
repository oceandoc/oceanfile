load("@bazel_skylib//lib:selects.bzl", "selects")

config_setting(
    name = "mac_arm64",
    constraint_values = [
        "@platforms//cpu:arm64",
        "@platforms//os:macos",
    ],
)

selects.config_setting_group(
    name = "arm_or_arm64",
    match_any = [
        "@platforms//cpu:arm",
        "@platforms//cpu:arm64",
    ],
)

JPEGTURBO_SRCS = [
    "src/jaricom.c",
    "src/jcapimin.c",
    "src/jcapistd.c",
    "src/jcarith.c",
    "src/jccoefct.c",
    "src/jccolor.c",
    "src/jcdctmgr.c",
    "src/jchuff.c",
    "src/jchuff.h",
    "src/jcinit.c",
    "src/jcmainct.c",
    "src/jcmarker.c",
    "src/jcmaster.c",
    "src/jcomapi.c",
    "src/jconfigint.h",
    "src/jcparam.c",
    "src/jcphuff.c",
    "src/jcprepct.c",
    "src/jcsample.c",
    "src/jdapimin.c",
    "src/jdapistd.c",
    "src/jdarith.c",
    "src/jdcoefct.c",
    "src/jdcoefct.h",
    "src/jdcolor.c",
    "src/jdct.h",
    "src/jddctmgr.c",
    "src/jdhuff.c",
    "src/jdhuff.h",
    "src/jdinput.c",
    "src/jdmainct.c",
    "src/jdmainct.h",
    "src/jdmarker.c",
    "src/jdmaster.c",
    "src/jdmaster.h",
    "src/jdmerge.c",
    "src/jdmerge.h",
    "src/jdphuff.c",
    "src/jdpostct.c",
    "src/jdsample.c",
    "src/jdsample.h",
    "src/jerror.c",
    "src/jfdctflt.c",
    "src/jfdctfst.c",
    "src/jfdctint.c",
    "src/jidctflt.c",
    "src/jidctfst.c",
    "src/jidctint.c",
    "src/jidctred.c",
    "src/jinclude.h",
    "src/jmemmgr.c",
    "src/jmemnobs.c",
    "src/jmemsys.h",
    "src/jpeg_nbits_table.c",
    "src/jpeg_nbits_table.h",
    "src/jpegcomp.h",
    "src/jpegint.h",
    "src/jquant1.c",
    "src/jquant2.c",
    "src/jsimd.h",
    "src/jsimddct.h",
    "src/jutils.c",
    "src/jversion.h",
] + select({
    ":arm_or_arm64": [
        "simd/arm/jccolor-neon.c",
        "simd/arm/jcgray-neon.c",
        "simd/arm/jchuff.h",
        "simd/arm/jcphuff-neon.c",
        "simd/arm/jcsample-neon.c",
        "simd/arm/jdcolor-neon.c",
        "simd/arm/jdmerge-neon.c",
        "simd/arm/jdsample-neon.c",
        "simd/arm/jfdctfst-neon.c",
        "simd/arm/jfdctint-neon.c",
        "simd/arm/jidctfst-neon.c",
        "simd/arm/jidctint-neon.c",
        "simd/arm/jidctred-neon.c",
        "simd/arm/jquanti-neon.c",
        "simd/arm/neon-compat.h",
        "simd/jsimd.h",
    ],
    "//conditions:default": ["jsimd_none.c"],
}) + select({
    "@platforms//cpu:arm": [
        "simd/arm/aarch32/jchuff-neon.c",
        "simd/arm/aarch32/jsimd.c",
    ],
    "@platforms//cpu:arm64": [
        "simd/arm/aarch64/jchuff-neon.c",
        "simd/arm/aarch64/jsimd.c",
        "simd/arm/align.h",
    ],
    "//conditions:default": [],
})

JPEGTURBO_TEXT_HDRS = [
    "jccolext.c",
    "jdmrgext.c",
    "jdcolext.c",
    "jdcol565.c",
    "jdmrg565.c",
    "jstdhuff.c",
] + select({
    ":arm_or_arm64": [
        "simd/arm/jdmrgext-neon.c",
        "simd/arm/jcgryext-neon.c",
        "simd/arm/jdcolext-neon.c",
    ],
    "//conditions:default": [],
}) + select({
    "@platforms//cpu:arm": [
        "simd/arm/aarch32/jccolext-neon.c",
    ],
    "@platforms//cpu:arm64": [
        "simd/arm/aarch64/jccolext-neon.c",
    ],
    "//conditions:default": [],
})

JPEGTURBO_DEFINES = [
    # Add support for arithmetic encoding (C_) and decoding (D_).
    # This matches Android. Note that such JPEGs are likely rare, given lack of
    # support by major browsers.
    "C_ARITH_CODING_SUPPORTED=1",
    "D_ARITH_CODING_SUPPORTED=1",
] + select({
    ":arm_or_arm64": ["NEON_INTRINSICS"],
    "//conditions:default": [],
}) + select({
    # Cuts a 64K table.
    "//conditions:default": ["USE_CLZ_INTRINSIC"],
    ":mac_arm64": [],  # disabled on M1 macs already
    "@platforms//os:windows": [],
})

cc_library(
    name = "libjpeg_turbo",
    srcs = JPEGTURBO_SRCS,
    hdrs = [
        "jconfig.h",
        "jerror.h",
        "jmorecfg.h",
        "jpeglib.h",
        "jpeglibmangler.h",
    ],
    copts = [
        # There are some #include "neon-compat.h" etc that need this search path
        "-Iexternal/libjpeg_turbo/simd/arm/",
    ],
    local_defines = JPEGTURBO_DEFINES,
    textual_hdrs = JPEGTURBO_TEXT_HDRS,
    visibility = ["//visibility:public"],
)
