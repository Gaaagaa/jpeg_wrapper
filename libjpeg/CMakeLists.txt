
# ====================================================================
# libjpeg

include_directories(include)
aux_source_directory(src LIBJPEG_SRC_LIST)

add_library(libjpeg ${LIBJPEG_SRC_LIST})

if (WIN32)
    set_target_properties(libjpeg PROPERTIES OUTPUT_NAME "libjpeg")
else ()
    set_target_properties(libjpeg PROPERTIES OUTPUT_NAME "jpeg")
endif ()

# ====================================================================

