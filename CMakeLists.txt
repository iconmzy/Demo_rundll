cmake_minimum_required(VERSION 3.15)
project(MyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(temp_rundll main.cpp)

# 指定 MyLibrary 的路径
add_subdirectory(FFS_FrameGUI)

# 链接 MyLibrary
target_link_libraries(temp_rundll PRIVATE FrameImGuiDLL)

# 设置可执行文件的运行目录，以便找到 DLL
set_target_properties(temp_rundll PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
