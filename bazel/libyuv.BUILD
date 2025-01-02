cc_library(
    name = "libyuv",
    srcs = glob(
        [
            "include/libyuv/*.h",
            "source/*.cc",
        ],
        exclude = [""],
    ) + [
    ],
    hdrs = [
        "include/libyuv.h",
    ],
    includes = [
        "include",
    ],
    local_defines = [
        "AVIF_CODEC_LIBGAV1=1",
        "AVIF_LIBYUV_ENABLED=1",
    ],
    visibility = ["//visibility:public"],
)
