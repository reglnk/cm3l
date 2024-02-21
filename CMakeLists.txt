 
cmake_minimum_required(VERSION 3.17)

project(Compose VERSION 0.1.0 LANGUAGES C)

set(CM3L_INTERNAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
set(CM3L_RUNTIME_OUTPUT_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "Put binary in this folder")

# The include dirs used by cm3d and subprojects
set(CM3L_INTERNAL_INCDIRS "" CACHE INTERNAL "")

list(APPEND CM3L_INTERNAL_INCDIRS
	"${CM3L_INTERNAL_ROOT}/include"
)

set (
CM3L_SRC
	"source/cm3l/Lib/Vector.c"
	"source/cm3l/Lib/SLList.c"
	"source/cm3l/Lexer.c"
	"source/cm3l/Composer.c"
)
add_library(cm3l STATIC ${CM3L_SRC})
set_target_properties(cm3l PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CM3L_RUNTIME_OUTPUT_DIR})
target_include_directories(cm3l PUBLIC ${CM3L_INTERNAL_INCDIRS})

set (
INTERPRETER_SRC
	"source/main.c"
)
add_executable(interpreter ${INTERPRETER_SRC})
set_target_properties(interpreter PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CM3L_RUNTIME_OUTPUT_DIR}
	RUNTIME_OUTPUT_NAME "cm3lang"
)
target_include_directories(interpreter PUBLIC ${CM3L_INTERNAL_INCDIRS})
target_link_libraries(interpreter PUBLIC "$<TARGET_FILE:cm3l>")

add_dependencies(interpreter cm3l)
