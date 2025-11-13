cmake_minimum_required(VERSION 3.16)
project(MyLibrary VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the source files
# (You can list them manually or use GLOB to include all .cpp files)
file(GLOB SOURCES "src/*.cpp")

# Create a library (choose STATIC or SHARED)
add_library(Engine STATIC ${SOURCES})
# add_library(Engine SHARED ${SOURCES}) # <-- Uncomment for shared library (.so/.dll/.dylib)

# Specify include directories
target_include_directories(Engine PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/include
${CMAKE_CURRENT_SOURCE_DIR}/vendor/EnTT
)

# Optional: Add compile options
target_compile_options(Engine PRIVATE -Wall -Wextra -Wpedantic)

# Optional: If you want to install the library
install(TARGETS Engine
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
