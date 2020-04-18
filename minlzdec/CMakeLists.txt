add_executable (minlzdec "minlzdec.c")

target_include_directories(minlzdec PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(minlzdec LINK_PUBLIC minlzlib)

if(MSVC)
    set(CMAKE_C_STANDARD_LIBRARIES "")
    set_property(TARGET minlzdec PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    string(REGEX REPLACE "/W[1-3]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/WX /Ox /Ob2 /Oi /Ot /Oy /GF /Gy /MT /Zi /permissive-")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/OPT:ICF /OPT:REF /DEBUG /EMITPOGOPHASEINFO /NOVCFEATURE /NOCOFFGRPINFO /PDBALTPATH:minlzdec.pdb")
else()
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Ofast -Wall -Werror")
endif()
