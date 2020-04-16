cmake_minimum_required (VERSION 3.8)

if(!MSVC)
    add_compile_options(-Wno-undefined-inline)
endif()

add_library (minlzlib STATIC "inputbuf.c" "dictbuf.c" "lzma2dec.c" "lzmadec.c" "rangedec.c" "xzstream.c" "lzmadec.h" "xzstream.h" "minlzlib.h")
set_target_properties(minlzlib PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
target_include_directories (minlzlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /Ob2 /Oi /Ot /Oy /GF /GL /Gy /MD /Zi /DNDEBUG")
endif()
