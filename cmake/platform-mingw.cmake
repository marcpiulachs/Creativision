# MinGW Platform Def from Example
# Use cmake -DCMAKE_TOOLCHAIN_FILE=
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_RC_COMPILER windres)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
