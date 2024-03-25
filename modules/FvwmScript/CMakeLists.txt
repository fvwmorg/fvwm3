# Declare the minimum required CMake version
# Name the project
project(FvwmScript)

add_subdirectory(${PROJECT_SOURCE_DIR}/Scripts)
add_subdirectory(${PROJECT_SOURCE_DIR}/Widgets)

#Remove header from SRC_FILES regex:
#' src/[^\n ]*\.h'
# Define the build target for the executable
add_executable(${PROJECT_NAME} FvwmScript.c Instructions.c libyywrap.c scanner.c script.c)
target_link_libraries(${PROJECT_NAME} Widgets)
