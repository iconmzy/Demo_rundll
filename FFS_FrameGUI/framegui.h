
//防止头文件被多次包含（避免重复定义、编译错误） 等价于#ifndef #define #endif 
#pragma once


// DLL 导出/导入控制
/*
当前项目的CMakelist会定义FRAMEGUI_EXPORTS，告诉编译器exportdll
当其他项目通过find_package()或target_link_libraries()链接这个 DLL 时，不会定义FRAMEGUI_EXPORTS，因此头文件中的FRAMEGUI_API会自动变为__declspec(dllimport)
*/
#ifdef FRAMEGUI_EXPORTS
#define FRAMEGUI_API __declspec(dllexport)
#else
#define FRAMEGUI_API __declspec(dllimport)
#endif




#include "ImGui/imconfig.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imstb_rectpack.h"
#include "ImGui/imstb_textedit.h"
#include "ImGui/imstb_truetype.h"
#include "ImGui/implot.h"
#include "ImGui/implot_internal.h"
#include "ImGui/implot3d.h"
#include "ImGui/implot3d_internal.h"

#include <d3d11.h>


#pragma comment(lib,"d3d11.lib")


extern "C" {
	FRAMEGUI_API int draw_gui();
}