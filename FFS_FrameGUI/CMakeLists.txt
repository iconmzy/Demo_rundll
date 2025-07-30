cmake_minimum_required(VERSION 3.15)
project(FrameGui LANGUAGES CXX)

#add_definitions(-FRAMEGUI_EXPORTS) 


# 设置输出目录
#[[
grammar 设置目录需要在 add_library/add_executable之前，否则会使用当时的输出目录设置
CMAKE_RUNTIME_OUTPUT_DIRECTORY
设置可执行文件（.exe、.out）和 DLL（.dll）的输出目录

CMAKE_LIBRARY_OUTPUT_DIRECTORY
设置共享库（.so、.dylib）的输出目录

CMAKE_ARCHIVE_OUTPUT_DIRECTORY
设置静态库（.lib、.a）的输出目录

${CMAKE_BINARY_DIR} 表示构建目录（Build Directory）

]]
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)



# 显式列出 ImGui 源文件
set(IMGUI_SOURCES
    ImGui/imgui.cpp
    ImGui/imgui_demo.cpp
    ImGui/imgui_draw.cpp
    ImGui/imgui_tables.cpp
    ImGui/imgui_widgets.cpp
    ImGui/imgui_impl_win32.cpp
    ImGui/imgui_impl_dx11.cpp
)


# 创建 DLL，需要定义在下面的宏之前
add_library(FrameImGuiDLL SHARED 
    runFrameGUI.cpp
    ${IMGUI_SOURCES}
)




# 定义 宏，用于导出DLL
target_compile_definitions(FrameImGuiDLL
    PRIVATE FRAMEGUI_EXPORTS
)





# find_package(DirectX REQUIRED)

# 添加 ImGui 源文件
#file(GLOB IMGUI_SOURCES 
  #  "ImGui/*.cpp"
   # "ImGui/imgui_impl_win32.cpp" 
   # "ImGui/imgui_impl_dx11.cpp" 
#)



# 包含目录
target_include_directories(FrameImGuiDLL PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui
)

# 链接库
target_link_libraries(FrameImGuiDLL 
    ${DirectX_LIBRARIES}
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
)