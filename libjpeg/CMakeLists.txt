
# ====================================================================
# libjpeg

include_directories(inc)
aux_source_directory(src LIBJPEG_SRC_LIST)

add_library(libjpeg ${LIBJPEG_SRC_LIST})
set_target_properties(libjpeg PROPERTIES OUTPUT_NAME "jpeg")

# ====================================================================

