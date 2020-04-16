add_executable (minlzdec "minlzdec.c")

target_include_directories(minlzdec PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(minlzdec LINK_PUBLIC minlzlib)

if(MSVC)
    set(CMAKE_C_STANDARD_LIBRARIES "")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Ox /Ob2 /Oi /Ot /Oy /GF /GL /Gy /MT /Zi /DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/LTCG /INCREMENTAL:NO /OPT:ICF /OPT:REF /DEBUG")
endif()
