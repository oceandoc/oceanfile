# This file will be copied into //third_party/externals/libpng via the new_local_repository
# rule in WORKSPACE.bazel, so all files should be relative to that path.
# We define this here because the emscripten toolchain calls the cpu wasm, whereas the
# bazelbuild/platforms call it wasm32. https://github.com/emscripten-core/emsdk/issues/919
config_setting(
    name = "cpu_wasm",
    values = {
        "cpu": "wasm",
    },
)

LIBPNG_SRCS = [
    "png.c",
    "pngconf.h",
    "pngdebug.h",
    "pngerror.c",
    "pngget.c",
    "pnginfo.h",
    "pngmem.c",
    "pngpread.c",
    "pngpriv.h",
    "pngread.c",
    "pngrio.c",
    "pngrtran.c",
    "pngrutil.c",
    "pngset.c",
    "pngstruct.h",
    "pngtrans.c",
    "pngwio.c",
    "pngwrite.c",
    "pngwtran.c",
    "pngwutil.c",
    "pnglibconf.h",
] + select({
    "@platforms//cpu:x86_64": [
        "intel/filter_sse2_intrinsics.c",
        "intel/intel_init.c",
    ],
    "@platforms//cpu:arm64": [
        "arm/arm_init.c",
        "arm/filter_neon_intrinsics.c",
        "arm/palette_neon_intrinsics.c",
    ],
    ":cpu_wasm": [],
    "//conditions:default": [],
})

PNG_DEFINES = ["PNG_SET_OPTION_SUPPORTED"] + select({
    "@platforms//cpu:x86_64": ["PNG_INTEL_SSE"],
    "//conditions:default": [],
})

cc_library(
    name = "libpng",
    srcs = LIBPNG_SRCS,
    hdrs = [
        "png.h",
        "pngconf.h",
        "pnglibconf.h",
    ],
	copts = select({
		"@platforms//os:windows": [],
		"//conditions:default": [
			"-Wno-unused-but-set-variable",
			"-Wno-macro-redefined",
		],
	}),
    includes = [
        ".",
    ],
    local_defines = PNG_DEFINES,
    textual_hdrs = ["scripts/pnglibconf.h.prebuilt"],
    visibility = ["//visibility:public"],
    deps = ["@zlib"],
)

# Creates a file called pnglibconf.h that includes the default png settings with one
# modification, undefining PNG_READ_OPT_PLTE_SUPPORTED.
genrule(
    name = "create_skia_pnglibconf.h",
    outs = ["pnglibconf.h"],
    cmd = "echo '#include \"scripts/pnglibconf.h.prebuilt\"\n#undef PNG_READ_OPT_PLTE_SUPPORTED' > $@",
    cmd_bat = "echo #include \"scripts/pnglibconf.h.prebuilt\" > $@ && echo #undef PNG_READ_OPT_PLTE_SUPPORTED >> $@",
)
