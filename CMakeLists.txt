set( CMAKE_VERBOSE_MAKEFILE on )
# Which compilers to use
#option(USE_CLANG "build application with clang" ON)
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -v")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fms-extensions")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
#Suppress -fms-extensions anonymous struct warnings - http://stackoverflow.com/questions/15505023/suppress-anonymous-structs-warning-with-clang-fms-extensions-doesnt-work
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-gnu")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-microsoft")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DORB_KHASH_TYPES_OFF")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers")

#include_directories(${CMAKE_SOURCE_DIR}/)
#include_directories(${CMAKE_SOURCE_DIR}/c)

#target_link_libraries(libtest ${PROJECT_LINK_LIBS} )
#target_link_libraries(wa.exe PRIVATE "/usr/lib/gcc/i686-w64-mingw32/4.9-win32/libgcc.a")
#target_link_libraries(wa.exe PRIVATE "/usr/lib/gcc/i686-w64-mingw32/4.9-win32/libgcc_eh.a")
link_directories(/usr/lib/gcc/i686-w64-mingw32/4.9-win32)

#FIND_LIBRARY(LIBGCC libgcc)
#file(GLOB Sources "*.cpp")
add_library(orb STATIC orb.c glew/glew.c)
#set_target_properties(wa PROPERTIES OUTPUT_NAME arc)
target_link_libraries(orb gcc)
target_link_libraries(orb gcc_eh)
target_link_libraries(orb /usr/local/lib/libSDL2main.a)
target_link_libraries(orb /usr/local/lib/libSDL2.a)